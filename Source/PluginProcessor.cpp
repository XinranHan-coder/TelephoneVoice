/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

TelephoneVoiceAudioProcessor::TelephoneVoiceAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

TelephoneVoiceAudioProcessor::~TelephoneVoiceAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout
TelephoneVoiceAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "input", "Input", -24.0f, 24.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "modulation", "Modulation", 0.0f, 1.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "output", "Output", -24.0f, 6.0f, 0.0f));

    return { params.begin(), params.end() };
}

void TelephoneVoiceAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    lastHpFreq = 0.0f;
    lastLpFreq = 0.0f;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = (juce::uint32)getTotalNumOutputChannels();

    highPass.prepare(spec);
    lowPass.prepare(spec);
    highPass.reset();
    lowPass.reset();

    const float sr = (float)sampleRate;
    const float hpInit = 20.0f;
    const float lpInit = sr * 0.45f;

    *highPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, hpInit, 0.707f);
    *lowPass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, lpInit, 0.707f);
    lastHpFreq = hpInit;
    lastLpFreq = lpInit;
}

void TelephoneVoiceAudioProcessor::releaseResources() {}

float TelephoneVoiceAudioProcessor::getInputLevel(int channel) const noexcept
{
    return channel == 0 ? inputLevelL.load() : inputLevelR.load();
}

float TelephoneVoiceAudioProcessor::getOutputLevel(int channel) const noexcept
{
    return channel == 0 ? outputLevelL.load() : outputLevelR.load();
}

void TelephoneVoiceAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int i = getTotalNumInputChannels(); i < numChannels; ++i)
        buffer.clear(i, 0, numSamples);

    const float inputGainDb = apvts.getRawParameterValue("input")->load();
    const float mod = apvts.getRawParameterValue("modulation")->load();
    const float outputDb = apvts.getRawParameterValue("output")->load();

    const float inputGain = juce::Decibels::decibelsToGain(inputGainDb);
    const float outputGain = juce::Decibels::decibelsToGain(outputDb);

    // Apply input gain
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* d = buffer.getWritePointer(ch);
        for (int s = 0; s < numSamples; ++s)
            d[s] *= inputGain;
    }

    // Measure input peak
    float peakL = numChannels > 0 ? buffer.getMagnitude(0, 0, numSamples) : 0.0f;
    float peakR = numChannels > 1 ? buffer.getMagnitude(1, 0, numSamples) : peakL;
    inputLevelL.store(peakL);
    inputLevelR.store(peakR);

    // ---- Modulation mapping (perceptually linear) ----
// HP: 20 -> 600 Hz, LP: lpMax -> 1800 Hz, both on exponential curves.
    const float hpFreq = 20.0f * std::pow(30.0f, mod);            // 20 -> 600
    const float lpMax = (float)currentSampleRate * 0.45f;
    const float lpFreq = lpMax * std::pow(1800.0f / lpMax, mod);   // lpMax -> 1800

    if (std::abs(hpFreq - lastHpFreq) > 0.5f)
    {
        *highPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
            currentSampleRate, hpFreq, 0.707f);
        lastHpFreq = hpFreq;
    }
    if (std::abs(lpFreq - lastLpFreq) > 0.5f)
    {
        *lowPass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
            currentSampleRate, lpFreq, 0.707f);
        lastLpFreq = lpFreq;
    }

    // Telephone band-pass
    juce::dsp::AudioBlock<float> block(buffer);
    highPass.process(juce::dsp::ProcessContextReplacing<float>(block));
    lowPass.process(juce::dsp::ProcessContextReplacing<float>(block));

    // ---- Clip-like saturation, blended with mod ----
    // drive: exponential 1 -> 13
    // compGain: partial compensation for peak squashing so it doesn't feel quieter
    const float drive = std::pow(13.0f, mod);
    const float compGain = 1.0f / std::sqrt(drive);
    const float wet = mod;

    float outPeakL = 0.0f, outPeakR = 0.0f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* d = buffer.getWritePointer(ch);
        for (int s = 0; s < numSamples; ++s)
        {
            float x = d[s];

            if (mod > 0.0005f)
            {
                const float shaped = std::tanh(x * drive) * compGain;
                x = x * (1.0f - wet) + shaped * wet;
            }

            x *= outputGain;
            d[s] = x;

            if (ch == 0)      outPeakL = juce::jmax(outPeakL, std::abs(x));
            else if (ch == 1) outPeakR = juce::jmax(outPeakR, std::abs(x));
        }
    }

    outputLevelL.store(outPeakL);
    outputLevelR.store(numChannels > 1 ? outPeakR : outPeakL);
}
juce::AudioProcessorEditor* TelephoneVoiceAudioProcessor::createEditor()
{
    return new TelephoneVoiceAudioProcessorEditor(*this);
}

void TelephoneVoiceAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void TelephoneVoiceAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TelephoneVoiceAudioProcessor();
}