/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class NeumorphicLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeumorphicLookAndFeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
        float sliderPosProportional, float rotaryStartAngle,
        float rotaryEndAngle, juce::Slider&) override;

    juce::Colour base{ 0xffe0e0e0 };
    juce::Colour dark{ 0xffa8a8a8 };
    juce::Colour light{ 0xffffffff };
    juce::Colour accent{ 0xff6a8caf };   // 低饱和度蓝
};

class LevelMeter : public juce::Component, private juce::Timer
{
public:
    LevelMeter(TelephoneVoiceAudioProcessor& p, bool isInput, bool ticksOnLeftSide);
    ~LevelMeter() override;

    void paint(juce::Graphics&) override;
    juce::Rectangle<float> getBarArea() const;

private:
    void timerCallback() override;

    TelephoneVoiceAudioProcessor& processor;
    bool inputMeter;
    bool ticksOnLeft;

    float smoothedL = 0.0f;
    float smoothedR = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};

class TelephoneVoiceAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    TelephoneVoiceAudioProcessorEditor(TelephoneVoiceAudioProcessor&);
    ~TelephoneVoiceAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    TelephoneVoiceAudioProcessor& audioProcessor;
    NeumorphicLookAndFeel neumorphicLnf;

    juce::Slider inputSlider, modulationSlider, outputSlider;
    juce::Label  inputLabel, modulationLabel, outputLabel;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> inputAttachment, modulationAttachment, outputAttachment;

    LevelMeter inputMeter{ audioProcessor, true,  false };
    LevelMeter outputMeter{ audioProcessor, false, true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TelephoneVoiceAudioProcessorEditor)
};