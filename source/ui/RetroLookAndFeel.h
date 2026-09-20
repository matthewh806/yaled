#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace retro
{
    // The whole UI is drawn from this small palette.
    inline const juce::Colour background   { 0xff1a1c2c };
    inline const juce::Colour panel        { 0xff333c57 };
    inline const juce::Colour border       { 0xff566c86 };
    inline const juce::Colour borderBright { 0xff94b0c2 };
    inline const juce::Colour accent       { 0xffffcd75 };
    inline const juce::Colour cyan         { 0xff73eff7 };
    inline const juce::Colour on           { 0xff38b764 };
    inline const juce::Colour text         { 0xfff4f4f4 };
    inline const juce::Colour dimText      { 0xff566c86 };

    // Press Start 2P is designed on an 8px grid: keep text heights to multiples of 8 for crisp glyphs.
    constexpr float captionFontHeight = 8.0f;
    constexpr float bodyFontHeight = 16.0f;
    constexpr float titleFontHeight = 32.0f;

    // One "art pixel". Everything is drawn as blocks of this size so the UI reads as low-res.
    constexpr int pixel = 4;
}

// 8-bit look for the plugin's controls: chunky pixel-grid knobs with an LED ring, notched
// panel corners, and the Press Start 2P typeface (SIL Open Font License, see licenses/).
class RetroLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RetroLookAndFeel();

    juce::Font pixelFont (float height) const;

    static void drawPixelBox (juce::Graphics&, juce::Rectangle<int> bounds, juce::Colour fill, juce::Colour border);

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPosProportional,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                       int buttonW, int buttonH, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;

private:
    juce::Typeface::Ptr typeface;
};
