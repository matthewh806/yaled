#include "RetroLookAndFeel.h"

#include "BinaryData.h"

#include <cmath>

RetroLookAndFeel::RetroLookAndFeel()
    : typeface (juce::Typeface::createSystemTypefaceFor (BinaryData::PressStart2PRegular_ttf,
                                                         BinaryData::PressStart2PRegular_ttfSize))
{
    setDefaultSansSerifTypeface (typeface);

    setColour (juce::PopupMenu::backgroundColourId, retro::background);
    setColour (juce::PopupMenu::textColourId, retro::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, retro::accent);
    setColour (juce::PopupMenu::highlightedTextColourId, retro::background);

    setColour (juce::ComboBox::textColourId, retro::text);
    setColour (juce::Label::textColourId, retro::text);

    setColour (juce::Slider::textBoxTextColourId, retro::text);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, retro::accent);

    // Shown while typing a value into a knob's readout.
    setColour (juce::TextEditor::backgroundColourId, retro::background);
    setColour (juce::TextEditor::textColourId, retro::text);
    setColour (juce::TextEditor::highlightColourId, retro::accent);
    setColour (juce::TextEditor::highlightedTextColourId, retro::background);
    setColour (juce::TextEditor::outlineColourId, retro::border);
    setColour (juce::TextEditor::focusedOutlineColourId, retro::accent);
    setColour (juce::CaretComponent::caretColourId, retro::accent);
}

juce::Font RetroLookAndFeel::pixelFont (float height) const
{
    return juce::Font (juce::FontOptions (typeface).withHeight (height));
}

void RetroLookAndFeel::drawPixelBox (juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour fill, juce::Colour borderColour)
{
    constexpr int p = retro::pixel;

    // Two overlapping rectangles leave the four corner pixels notched out. Then the same
    // shape, one pixel in, in the fill colour: that is the border.
    g.setColour (borderColour);
    g.fillRect (bounds.getX() + p, bounds.getY(), bounds.getWidth() - 2 * p, bounds.getHeight());
    g.fillRect (bounds.getX(), bounds.getY() + p, bounds.getWidth(), bounds.getHeight() - 2 * p);

    const auto inner = bounds.reduced (p);
    g.setColour (fill);
    g.fillRect (inner.getX() + p, inner.getY(), inner.getWidth() - 2 * p, inner.getHeight());
    g.fillRect (inner.getX(), inner.getY() + p, inner.getWidth(), inner.getHeight() - 2 * p);
}

void RetroLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                         float startAngle, float endAngle, juce::Slider&)
{
    constexpr int p = retro::pixel;
    constexpr int ledRingSpace = 4 * p; // room outside the knob body for the ring of LEDs

    // Snap the centre to the pixel grid so every block lines up with the rest of the UI.
    const auto centre = juce::Rectangle<int> (x, y, width, height).getCentre();
    const int cx = (centre.x / p) * p;
    const int cy = (centre.y / p) * p;
    const float radius = static_cast<float> (std::min (width, height) / 2 - ledRingSpace);
    const int reach = static_cast<int> (radius / p) + 1;

    auto fillCell = [&] (int i, int j, juce::Colour colour)
    {
        g.setColour (colour);
        g.fillRect (cx + i * p, cy + j * p, p, p);
    };

    // The knob body: a disc built from square cells, with the outermost ring highlighted.
    for (int j = -reach; j < reach; ++j)
    {
        for (int i = -reach; i < reach; ++i)
        {
            const float dx = (static_cast<float> (i) + 0.5f) * p;
            const float dy = (static_cast<float> (j) + 0.5f) * p;
            const float distance = std::sqrt (dx * dx + dy * dy);

            if (distance <= radius)
                fillCell (i, j, distance > radius - p ? retro::borderBright : retro::panel);
        }
    }

    // The pointer, drawn as a row of blocks from near the centre to the rim.
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const float dirX = std::sin (angle);
    const float dirY = -std::cos (angle);

    for (float t = radius * 0.2f; t < radius - p; t += p * 0.5f)
        fillCell (static_cast<int> (std::floor (dirX * t / p)), static_cast<int> (std::floor (dirY * t / p)), retro::accent);

    // The LED ring: lit up to the current value.
    constexpr int numLeds = 11;
    const float ledRadius = radius + 2.5f * p;

    for (int k = 0; k < numLeds; ++k)
    {
        const float fraction = static_cast<float> (k) / static_cast<float> (numLeds - 1);
        const float ledAngle = startAngle + fraction * (endAngle - startAngle);
        const int i = static_cast<int> (std::floor ((std::sin (ledAngle) * ledRadius - p) / p));
        const int j = static_cast<int> (std::floor ((-std::cos (ledAngle) * ledRadius - p) / p));

        g.setColour (sliderPos + 0.0001f >= fraction ? retro::accent : retro::border);
        g.fillRect (cx + i * p, cy + j * p, 2 * p, 2 * p);
    }
}

void RetroLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    constexpr int p = retro::pixel;
    const auto bounds = button.getLocalBounds();
    const bool isOn = button.getToggleState();

    drawPixelBox (g, bounds,
                  shouldDrawButtonAsDown ? retro::border : retro::panel,
                  shouldDrawButtonAsHighlighted || button.hasKeyboardFocus (true) ? retro::accent : retro::borderBright);

    // An LED that lights up when the button is on.
    const auto led = juce::Rectangle<int> (bounds.getX() + 3 * p, bounds.getCentreY() - 2 * p, 4 * p, 4 * p);
    g.setColour (retro::border);
    g.fillRect (led);
    g.setColour (isOn ? retro::on : retro::background);
    g.fillRect (led.reduced (p / 2 + 1));

    g.setColour (isOn ? retro::text : retro::dimText);
    g.setFont (pixelFont (16.0f));
    g.drawText (button.getButtonText(), bounds.withTrimmedLeft (9 * p), juce::Justification::centredLeft, false);
}

void RetroLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                     int, int, int, int, juce::ComboBox& box)
{
    constexpr int p = retro::pixel;

    drawPixelBox (g, { 0, 0, width, height },
                  isButtonDown ? retro::border : retro::panel,
                  box.hasKeyboardFocus (true) || box.isPopupActive() ? retro::accent : retro::borderBright);

    // A down arrow: three rows of blocks narrowing to a point.
    const int arrowX = width - 7 * p;
    const int arrowY = height / 2 - p;
    g.setColour (retro::accent);
    g.fillRect (arrowX, arrowY, 5 * p, p);
    g.fillRect (arrowX + p, arrowY + p, 3 * p, p);
    g.fillRect (arrowX + 2 * p, arrowY + 2 * p, p, p);
}

void RetroLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    constexpr int p = retro::pixel;
    label.setBounds (3 * p, 0, box.getWidth() - 11 * p, box.getHeight());
    label.setFont (getComboBoxFont (box));
}

juce::Font RetroLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return pixelFont (16.0f);
}

juce::Font RetroLookAndFeel::getPopupMenuFont()
{
    return pixelFont (16.0f);
}

juce::Font RetroLookAndFeel::getLabelFont (juce::Label& label)
{
    return pixelFont (label.getFont().getHeight());
}

juce::Label* RetroLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (pixelFont (16.0f));
    label->setJustificationType (juce::Justification::centred);
    return label;
}
