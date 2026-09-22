/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "PluginProcessor.h"

// ============ NeumorphicLookAndFeel ============

NeumorphicLookAndFeel::NeumorphicLookAndFeel() {}

void NeumorphicLookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider&)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(6.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    auto centre = bounds.getCentre();
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Outer raised shadow
    {
        juce::Path circle;
        circle.addEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

        juce::DropShadow darkShadow(dark.withAlpha(0.85f), 10, { 4, 4 });
        darkShadow.drawForPath(g, circle);

        juce::DropShadow lightShadow(light.withAlpha(0.9f), 10, { -4, -4 });
        lightShadow.drawForPath(g, circle);
    }

    // Main body
    {
        juce::ColourGradient grad(base.brighter(0.06f),
            centre.x - radius, centre.y - radius,
            base.darker(0.06f),
            centre.x + radius, centre.y + radius, false);
        g.setGradientFill(grad);
        g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
    }

    // Inner recessed area
    auto innerR = radius * 0.72f;
    {
        juce::Path innerCircle;
        innerCircle.addEllipse(centre.x - innerR, centre.y - innerR, innerR * 2.0f, innerR * 2.0f);

        juce::DropShadow darkShadow(dark.withAlpha(0.7f), 6, { -2, -2 });
        darkShadow.drawForPath(g, innerCircle);

        juce::DropShadow lightShadow(light.withAlpha(0.9f), 6, { 2, 2 });
        lightShadow.drawForPath(g, innerCircle);
    }
    {
        juce::ColourGradient grad(base.darker(0.05f),
            centre.x - innerR, centre.y - innerR,
            base.brighter(0.05f),
            centre.x + innerR, centre.y + innerR, false);
        g.setGradientFill(grad);
        g.fillEllipse(centre.x - innerR, centre.y - innerR, innerR * 2.0f, innerR * 2.0f);
    }

    // Recessed needle
    juce::Path needle;
    auto needleLen = radius * 0.70f;
    auto needleW = juce::jmax(2.5f, radius * 0.10f);
    needle.addRoundedRectangle(-needleW * 0.5f, -needleLen,
        needleW, needleLen,
        needleW * 0.5f);

    {
        auto shadowT = juce::AffineTransform::rotation(angle)
            .translated(centre)
            .translated(1.0f, 1.0f);
        g.setColour(dark.withAlpha(0.75f));
        g.fillPath(needle, shadowT);
    }

    {
        auto mainT = juce::AffineTransform::rotation(angle).translated(centre);
        g.setColour(accent);
        g.fillPath(needle, mainT);
    }

    {
        juce::Path hi;
        hi.startNewSubPath(-needleW * 0.5f + 0.6f, -2.0f);
        hi.lineTo(-needleW * 0.5f + 0.6f, -needleLen + 2.0f);

        auto hiT = juce::AffineTransform::rotation(angle).translated(centre);
        g.setColour(light.withAlpha(0.55f));
        g.strokePath(hi, juce::PathStrokeType(0.7f), hiT);
    }

    // Centre cap
    {
        auto capR = needleW * 1.1f;

        juce::Path cap;
        cap.addEllipse(centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f);

        juce::DropShadow dk(dark.withAlpha(0.6f), 4, { -1, -1 });
        dk.drawForPath(g, cap);

        juce::DropShadow lt(light.withAlpha(0.7f), 3, { 1, 1 });
        lt.drawForPath(g, cap);

        g.setColour(base.darker(0.02f));
        g.fillEllipse(centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f);
    }
}

// ============ LevelMeter ============

LevelMeter::LevelMeter(TelephoneVoiceAudioProcessor& p, bool isInput, bool ticksOnLeftSide)
    : processor(p), inputMeter(isInput), ticksOnLeft(ticksOnLeftSide)
{
    startTimerHz(30);
}

LevelMeter::~LevelMeter()
{
    stopTimer();
}

juce::Rectangle<float> LevelMeter::getBarArea() const
{
    auto b = getLocalBounds().toFloat();

    const float scaleW = 20.0f;   // 刻度区变窄
    const float vPad = 6.0f;

    return ticksOnLeft
        ? b.withTrimmedLeft(scaleW).withTrimmedTop(vPad).withTrimmedBottom(vPad)
        : b.withTrimmedRight(scaleW).withTrimmedTop(vPad).withTrimmedBottom(vPad);
}

void LevelMeter::timerCallback()
{
    float l = inputMeter ? processor.getInputLevel(0) : processor.getOutputLevel(0);
    float r = inputMeter ? processor.getInputLevel(1) : processor.getOutputLevel(1);

    auto smooth = [](float current, float target)
        {
            if (target > current)
                return current * 0.5f + target * 0.5f;   // 快速上升
            else
                return current * 0.95f + target * 0.05f; // 慢速回落
        };

    smoothedL = smooth(smoothedL, l);
    smoothedR = smooth(smoothedR, r);
    repaint();
}

void LevelMeter::paint(juce::Graphics& g)
{
    auto barArea = getBarArea();

    // 凹陷背景
    {
        juce::Path bg;
        bg.addRoundedRectangle(barArea, 4.0f);

        g.setColour(juce::Colour(0xffc8c8c8));
        g.fillPath(bg);

        juce::DropShadow innerDark(juce::Colour(0xff909090), 5, { -1, -1 });
        innerDark.drawForPath(g, bg);

        juce::DropShadow innerLight(juce::Colour(0xffeeeeee), 4, { 1, 1 });
        innerLight.drawForPath(g, bg);
    }

    auto inner = barArea.reduced(3.0f);
    auto lBar = inner.removeFromLeft(inner.getWidth() * 0.5f).reduced(1.0f, 0.0f);
    auto rBar = inner.reduced(1.0f, 0.0f);

    auto levelToNorm = [](float level) -> float
        {
            if (level <= 0.0001f) return 0.0f;
            float db = juce::Decibels::gainToDecibels(level, -60.0f);
            return juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
        };

    auto drawBar = [&](juce::Rectangle<float> r, float level)
        {
            float n = levelToNorm(level);
            if (n < 0.001f) return;

            float h = r.getHeight() * n;
            auto filled = r.withTop(r.getBottom() - h);

            // 单一低饱和度蓝
            g.setColour(juce::Colour(0xff6a8caf));
            g.fillRect(filled);
        };

    drawBar(lBar, smoothedL);
    drawBar(rBar, smoothedR);

    // dB 刻度
    g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));

    const float tickX = ticksOnLeft ? barArea.getX() : barArea.getRight();

    for (int db = 0; db >= -60; db -= 6)
    {
        const float norm = (float)(db + 60) / 60.0f;
        const float y = barArea.getBottom() - norm * barArea.getHeight();

        const bool  major = (db % 12 == 0);
        const float tickLen = major ? 5.0f : 3.0f;
        const float lineW = major ? 1.0f : 0.6f;

        g.setColour(juce::Colour(0xff5a5a5a));

        if (ticksOnLeft)
            g.drawLine(tickX - tickLen, y, tickX, y, lineW);
        else
            g.drawLine(tickX, y, tickX + tickLen, y, lineW);

        if (major && db >= -48)
        {
            const float textY = juce::jlimit(0.5f,
                (float)getHeight() - 10.5f,
                y - 5.0f);

            if (ticksOnLeft)
            {
                const float textW = 16.0f;
                const float textX = tickX - tickLen - 1.0f - textW;

                g.setColour(juce::Colour(0xff3a3a3a));
                g.drawText(juce::String(db),
                    juce::Rectangle<float>(textX, textY, textW, 10.0f),
                    juce::Justification::centredRight);
            }
            else
            {
                const float textX = tickX + tickLen + 1.0f;
                const float textW = 16.0f;

                g.setColour(juce::Colour(0xff3a3a3a));
                g.drawText(juce::String(db),
                    juce::Rectangle<float>(textX, textY, textW, 10.0f),
                    juce::Justification::centredLeft);
            }
        }
    }
}

// ============ Editor ============

TelephoneVoiceAudioProcessorEditor::TelephoneVoiceAudioProcessorEditor(TelephoneVoiceAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&neumorphicLnf);

    auto setupSlider = [this](juce::Slider& s,
        const juce::String& paramID,
        std::unique_ptr<Attachment>& att)
        {
            s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            s.setPopupDisplayEnabled(true, false, this);
            addAndMakeVisible(s);
            att = std::make_unique<Attachment>(audioProcessor.apvts, paramID, s);
        };

    setupSlider(inputSlider, "input", inputAttachment);
    setupSlider(modulationSlider, "modulation", modulationAttachment);
    setupSlider(outputSlider, "output", outputAttachment);

    auto setupLabel = [this](juce::Label& l, const juce::String& text, float size)
        {
            l.setText(text, juce::dontSendNotification);
            l.setJustificationType(juce::Justification::centred);
            l.setColour(juce::Label::textColourId, juce::Colour(0xff4a4a4a));
            l.setFont(juce::Font(juce::FontOptions().withHeight(size).withStyle("Bold")));
            addAndMakeVisible(l);
        };

    setupLabel(inputLabel, "INPUT", 13.0f);
    setupLabel(modulationLabel, "MODULATION", 14.0f);
    setupLabel(outputLabel, "OUTPUT", 13.0f);

    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);

    setSize(640, 420);
}

TelephoneVoiceAudioProcessorEditor::~TelephoneVoiceAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void TelephoneVoiceAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(neumorphicLnf.base);
}

void TelephoneVoiceAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);

    const int sideColW = 130;
    const int gap = 10;

    auto leftCol = area.removeFromLeft(sideColW);
    auto rightCol = area.removeFromRight(sideColW);
    area.removeFromLeft(gap);
    area.removeFromRight(gap);
    auto midCol = area;

    // Side column layout:
    //   Top    : meter (rest of the column height)
    //   Middle : small knob
    //   Bottom : label
    // Bottom-up reservation avoids the meter overlapping knob/label.
    auto layoutSide = [this](juce::Rectangle<int> col,
        LevelMeter& meter,
        juce::Slider& knob,
        juce::Label& label)
        {
            auto c = col.reduced(6, 0);

            const int labelH = 20;
            const int knobD = 66;
            const int pad = 8;
            const int meterW = 46;

            // Reserve from bottom: label, then pad, then knob, then pad.
            auto labelArea = c.removeFromBottom(labelH);
            c.removeFromBottom(pad);
            auto knobRow = c.removeFromBottom(knobD);
            c.removeFromBottom(pad);

            // Remaining c is exclusively for the meter.
            meter.setBounds(c.withSizeKeepingCentre(meterW, c.getHeight()));

            // Align knob and label to the meter's bar-area centre (excluding ticks).
            const auto meterBounds = meter.getBounds().toFloat();
            const auto barRel = meter.getBarArea();
            const float barCentreX = meterBounds.getX() + barRel.getCentreX();

            knob.setBounds(juce::Rectangle<int>(knobD, knobD)
                .withCentre({ juce::roundToInt(barCentreX),
                               knobRow.getCentreY() }));

            const int labelW = 100;
            label.setBounds(juce::Rectangle<int>(labelW, labelH)
                .withCentre({ juce::roundToInt(barCentreX),
                               labelArea.getCentreY() }));
        };

    layoutSide(leftCol, inputMeter, inputSlider, inputLabel);
    layoutSide(rightCol, outputMeter, outputSlider, outputLabel);

    // Centre: bigger Modulation knob, vertically centred, label below.
    {
        const int knobD = 200;   // was 160
        const int labelH = 24;
        const int pad = 12;

        const int totalH = knobD + pad + labelH;
        const int topY = midCol.getY() + (midCol.getHeight() - totalH) / 2;

        modulationSlider.setBounds(midCol.getX() + (midCol.getWidth() - knobD) / 2,
            topY, knobD, knobD);

        const int labelW = 160;
        modulationLabel.setBounds(juce::Rectangle<int>(labelW, labelH)
            .withCentre({ midCol.getCentreX(),
                           topY + knobD + pad + labelH / 2 }));
    }
}