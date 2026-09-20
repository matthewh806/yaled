#include "PluginEditor.h"

namespace
{
    // All positions are multiples of retro::pixel so the layout sits on the pixel grid.
    constexpr int windowWidth = 480;
    constexpr int windowHeight = 328;
    constexpr int margin = 24;

    constexpr int titleY = 28;
    constexpr int topDividerY = 92;
    constexpr int bottomDividerY = 244;

    constexpr int columnWidth = (windowWidth - 2 * margin) / 3;
    constexpr int knobCaptionY = 104;
    constexpr int knobY = 116;
    constexpr int knobHeight = 116; // the knob itself, plus its value readout underneath
    constexpr int readoutHeight = 24;
    constexpr float dimmedAlpha = 0.35f; // a control that is not in use right now

    constexpr int bottomCaptionY = 256;
    constexpr int bottomControlY = 272;
    constexpr int bottomControlHeight = 32;
    constexpr int syncButtonX = margin;
    constexpr int syncButtonWidth = 160;
    constexpr int divisionBoxX = 208;
    constexpr int divisionBoxWidth = 128;

    juce::Rectangle<int> knobColumn (int index)
    {
        return { margin + index * columnWidth, knobY, columnWidth, knobHeight };
    }

    void drawDashedLine (juce::Graphics& g, int y)
    {
        g.setColour (retro::border);
        for (int x = margin; x < windowWidth - margin; x += 2 * retro::pixel)
            g.fillRect (x, y, retro::pixel, retro::pixel);
    }
}

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&lookAndFeel);

    configureKnob (chunkLengthSlider, "chunkLengthMs");
    configureKnob (mixSlider, "mix");
    configureKnob (feedbackSlider, "feedback");

    tempoSyncButton.setComponentID ("tempoSync");
    addAndMakeVisible (tempoSyncButton);

    divisionBox.setComponentID ("tempoSyncDivision");
    if (auto* division = dynamic_cast<juce::AudioParameterChoice*> (processorRef.apvts.getParameter ("tempoSyncDivision")))
        divisionBox.addItemList (division->choices, 1);
    addAndMakeVisible (divisionBox);

    chunkLengthAttachment = std::make_unique<SliderAttachment> (processorRef.apvts, "chunkLengthMs", chunkLengthSlider);
    mixAttachment = std::make_unique<SliderAttachment> (processorRef.apvts, "mix", mixSlider);
    feedbackAttachment = std::make_unique<SliderAttachment> (processorRef.apvts, "feedback", feedbackSlider);
    tempoSyncAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, "tempoSync", tempoSyncButton);
    divisionAttachment = std::make_unique<ComboBoxAttachment> (processorRef.apvts, "tempoSyncDivision", divisionBox);

    // The attachments install their own readout text, so ours goes on afterwards.
    formatKnobReadout (chunkLengthSlider, " MS");
    formatKnobReadout (mixSlider, " %");
    formatKnobReadout (feedbackSlider, " %");

    // With Tempo Sync on, the division decides the Chunk Length and the free knob is ignored.
    tempoSyncButton.onStateChange = [this] { updateTempoSyncDimming(); };
    updateTempoSyncDimming();

    setWantsKeyboardFocus (true);
    setSize (windowWidth, windowHeight);
}

PluginEditor::~PluginEditor()
{
    setLookAndFeel (nullptr);
}

void PluginEditor::configureKnob (juce::Slider& slider, const juce::String& parameterId)
{
    slider.setComponentID (parameterId);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, columnWidth - 2 * retro::pixel, readoutHeight);
    addAndMakeVisible (slider);
}

void PluginEditor::formatKnobReadout (juce::Slider& slider, const juce::String& suffix)
{
    slider.textFromValueFunction = [suffix] (double value) { return juce::String (juce::roundToInt (value)) + suffix; };
    slider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue(); };
    slider.updateText();
}

void PluginEditor::updateTempoSyncDimming()
{
    const bool synced = tempoSyncButton.getToggleState();
    chunkLengthSlider.setAlpha (synced ? dimmedAlpha : 1.0f);
    divisionBox.setAlpha (synced ? 1.0f : dimmedAlpha);
}

//==============================================================================
void PluginEditor::paint (juce::Graphics& g)
{
    constexpr int p = retro::pixel;

    g.fillAll (retro::background);
    RetroLookAndFeel::drawPixelBox (g, getLocalBounds().reduced (2 * p), retro::background, retro::borderBright);

    // The name, with a drop shadow.
    const juce::Rectangle<int> title (margin + p, titleY, 320, 32);
    g.setFont (lookAndFeel.pixelFont (retro::titleFontHeight));
    g.setColour (retro::border);
    g.drawText ("YALED", title.translated (p, p), juce::Justification::topLeft, false);
    g.setColour (retro::accent);
    g.drawText ("YALED", title, juce::Justification::topLeft, false);

    g.setFont (lookAndFeel.pixelFont (retro::captionFontHeight));
    g.setColour (retro::cyan);
    g.drawText ("REVERSE DELAY", margin + p, titleY + 44, 200, 8, juce::Justification::topLeft, false);

    g.setColour (retro::dimText);
    g.drawText (juce::String ("V") + VERSION, windowWidth - margin - 120, titleY, 120, 8, juce::Justification::topRight, false);

    drawDashedLine (g, topDividerY);
    drawDashedLine (g, bottomDividerY);

    // Captions above each control.
    g.setColour (retro::text);
    const char* knobCaptions[] = { "CHUNK LENGTH", "MIX", "FEEDBACK" };
    for (int i = 0; i < 3; ++i)
        g.drawText (knobCaptions[i], margin + i * columnWidth, knobCaptionY, columnWidth, 8, juce::Justification::centred, false);

    g.drawText ("TEMPO SYNC", syncButtonX, bottomCaptionY, syncButtonWidth, 8, juce::Justification::centredLeft, false);
    g.drawText ("DIVISION", divisionBoxX, bottomCaptionY, divisionBoxWidth, 8, juce::Justification::centredLeft, false);
}

void PluginEditor::paintOverChildren (juce::Graphics& g)
{
    // Faint scanlines over everything, like an old CRT.
    g.setColour (juce::Colours::black.withAlpha (0.12f));
    for (int y = 0; y < getHeight(); y += retro::pixel)
        g.fillRect (0, y, getWidth(), 1);
}

void PluginEditor::resized()
{
    chunkLengthSlider.setBounds (knobColumn (0));
    mixSlider.setBounds (knobColumn (1));
    feedbackSlider.setBounds (knobColumn (2));

    tempoSyncButton.setBounds (syncButtonX, bottomControlY, syncButtonWidth, bottomControlHeight);
    divisionBox.setBounds (divisionBoxX, bottomControlY, divisionBoxWidth, bottomControlHeight);
}

bool PluginEditor::keyPressed (const juce::KeyPress& key)
{
    // Developer aid: Cmd/Ctrl+Shift+I opens the Melatonin Inspector.
    if (key == juce::KeyPress ('i', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0))
    {
        if (inspector == nullptr)
        {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }

        inspector->setVisible (true);
        return true;
    }

    return false;
}
