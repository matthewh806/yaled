#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include "ui/RetroLookAndFeel.h"

//==============================================================================
// A rudimentary custom editor: the plugin name and one control for every parameter,
// drawn in a retro 8-bit style. Each control's component ID is its parameter ID.
class PluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void configureKnob (juce::Slider&, const juce::String& parameterId);
    void formatKnobReadout (juce::Slider&, const juce::String& suffix);
    void updateTempoSyncDimming();

    PluginProcessor& processorRef;
    RetroLookAndFeel lookAndFeel;

    juce::Slider chunkLengthSlider, mixSlider, feedbackSlider;
    juce::ToggleButton tempoSyncButton { "SYNC" };
    juce::ComboBox divisionBox;

    // Declared after the controls so they are destroyed first.
    std::unique_ptr<SliderAttachment> chunkLengthAttachment, mixAttachment, feedbackAttachment;
    std::unique_ptr<ButtonAttachment> tempoSyncAttachment;
    std::unique_ptr<ComboBoxAttachment> divisionAttachment;

    std::unique_ptr<melatonin::Inspector> inspector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
