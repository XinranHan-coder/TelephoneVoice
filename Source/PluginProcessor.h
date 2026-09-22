/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>

class TelephoneVoiceAudioProcessor : public juce::AudioProcessor
{
public:
    TelephoneVoiceAudioProcessor();
    ~TelephoneVoiceAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    float getInputLevel(int channel) const noexcept;
    float getOutputLevel(int channel) const noexcept;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>> highPass, lowPass;

    double currentSampleRate = 44100.0;
    float  lastHpFreq = 0.0f;
    float  lastLpFreq = 0.0f;

    std::atomic<float> inputLevelL{ 0.0f };
    std::atomic<float> inputLevelR{ 0.0f };
    std::atomic<float> outputLevelL{ 0.0f };
    std::atomic<float> outputLevelR{ 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TelephoneVoiceAudioProcessor)
};