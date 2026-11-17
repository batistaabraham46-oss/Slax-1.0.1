#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

// Black + maroon theme -------------------------------------------------------
namespace UI
{
inline const juce::Colour bg        { 0xff0a0a0b };
inline const juce::Colour bgLow     { 0xff0f0c0d };
inline const juce::Colour panel     { 0xff131316 };
inline const juce::Colour panelEdge { 0xff222228 };
inline const juce::Colour knobBody  { 0xff1a1a1e };
inline const juce::Colour track     { 0xff2a2a30 };
inline const juce::Colour maroon    { 0xff8a1224 };
inline const juce::Colour maroonHi  { 0xffc01a33 };
inline const juce::Colour text      { 0xffe9e9ec };
inline const juce::Colour textDim   { 0xff85858f };

inline juce::Font font (float size, bool bold = false, float kerning = 0.0f)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain)
                           .withKerningFactor (kerning));
}
} // namespace UI

//==============================================================================
class UMLookAndFeel : public juce::LookAndFeel_V4
{
public:
    UMLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId,       UI::text);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId,  UI::maroon);
        setColour (juce::CaretComponent::caretColourId,     UI::maroonHi);

        setColour (juce::TextButton::buttonColourId,   UI::panel);
        setColour (juce::TextButton::buttonOnColourId, UI::maroon);
        setColour (juce::TextButton::textColourOffId,  UI::text);
        setColour (juce::TextButton::textColourOnId,   UI::text);

        setColour (juce::ComboBox::backgroundColourId, UI::knobBody);
        setColour (juce::ComboBox::textColourId,       UI::text);
        setColour (juce::ComboBox::outlineColourId,    UI::panelEdge);
        setColour (juce::ComboBox::arrowColourId,      UI::maroonHi);

        setColour (juce::PopupMenu::backgroundColourId,            UI::panel);
        setColour (juce::PopupMenu::textColourId,                  UI::text);
        setColour (juce::PopupMenu::headerTextColourId,            UI::maroonHi);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, UI::maroon);
        setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override { return UI::font (14.0f, true); }
    juce::Font getComboBoxFont (juce::ComboBox&) override          { return UI::font (13.5f, true); }
    juce::Font getPopupMenuFont() override                         { return UI::font (14.5f); }
    juce::Font getLabelFont (juce::Label&) override                { return UI::font (12.0f, true, 0.08f); }
    juce::Font getSliderPopupFont (juce::Slider&) override         { return UI::font (13.0f); }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                               bool highlighted, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        auto fill = UI::panel;
        if (highlighted) fill = fill.brighter (0.08f);
        if (down)        fill = UI::maroon.withAlpha (0.6f);

        g.setColour (fill);
        g.fillRoundedRectangle (r, 9.0f);
        g.setColour (highlighted ? UI::maroon : UI::panelEdge);
        g.drawRoundedRectangle (r, 9.0f, 1.0f);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
        const bool hot = box.isMouseOver (true) || isButtonDown;

        g.setColour (UI::knobBody);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (hot ? UI::maroon : UI::panelEdge);
        g.drawRoundedRectangle (r, 8.0f, 1.0f);

        // little down arrow
        const float cx = (float) width - 15.0f, cy = (float) height * 0.5f;
        juce::Path arrow;
        arrow.addTriangle (cx - 4.5f, cy - 2.0f, cx + 4.5f, cy - 2.0f, cx, cy + 3.0f);
        g.setColour (UI::maroonHi);
        g.fillPath (arrow);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float pos, float startAngle, float endAngle, juce::Slider& s) override
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
        const float d = juce::jmin (bounds.getWidth(), bounds.getHeight());
        auto r = juce::Rectangle<float> (d, d).withCentre (bounds.getCentre());
        const auto c = r.getCentre();

        const float trackW = 4.5f;
        const float arcR   = d * 0.5f - trackW * 0.5f;
        const float angle  = startAngle + pos * (endAngle - startAngle);

        // Full track
        juce::Path trackPath;
        trackPath.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (UI::track);
        g.strokePath (trackPath, juce::PathStrokeType (trackW, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

        // Value arc (starts at the centre for bipolar controls such as EQ gains)
        const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
        const float from = bipolar ? startAngle + (float) s.valueToProportionOfLength (0.0) * (endAngle - startAngle)
                                   : startAngle;
        const float a0 = juce::jmin (from, angle);
        const float a1 = juce::jmax (from, angle);

        if (a1 - a0 > 0.001f)
        {
            juce::Path valuePath;
            valuePath.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, a0, a1, true);

            // soft glow, then the crisp arc
            g.setColour (UI::maroonHi.withAlpha (s.isEnabled() ? 0.22f : 0.08f));
            g.strokePath (valuePath, juce::PathStrokeType (trackW + 5.0f, juce::PathStrokeType::curved,
                                                           juce::PathStrokeType::rounded));
            g.setColour (s.isMouseOverOrDragging() ? UI::maroonHi : UI::maroon);
            g.strokePath (valuePath, juce::PathStrokeType (trackW, juce::PathStrokeType::curved,
                                                           juce::PathStrokeType::rounded));
        }

        // Knob body
        auto body = r.reduced (trackW + 7.0f);
        g.setColour (UI::knobBody);
        g.fillEllipse (body);
        g.setColour (UI::panelEdge);
        g.drawEllipse (body, 1.0f);

        // Pointer
        juce::Path p;
        const float len = body.getWidth() * 0.5f;
        p.addRoundedRectangle (-1.6f, -len + 5.0f, 3.2f, len * 0.38f, 1.6f);
        p.applyTransform (juce::AffineTransform::rotation (angle).translated (c.x, c.y));
        g.setColour (UI::text);
        g.fillPath (p);
    }

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
    {
        g.fillAll (UI::panel);
        g.setColour (UI::panelEdge);
        g.drawRect (0, 0, width, height, 1);
    }
};

//==============================================================================
// Output meter: a full panel with two vertical bars and a peak read-out.
class LevelMeter : public juce::Component, private juce::Timer
{
public:
    explicit LevelMeter (MasterProcessor& p) : proc (p) { startTimerHz (30); }
    ~LevelMeter() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (UI::panel);
        g.fillRoundedRectangle (b, 14.0f);
        g.setColour (UI::panelEdge);
        g.drawRoundedRectangle (b, 14.0f, 1.0f);

        // Title (same style as the other panels)
        g.setColour (UI::maroonHi);
        g.fillEllipse (b.getX() + 18.0f, b.getY() + 17.0f, 6.0f, 6.0f);
        g.setColour (UI::textDim);
        g.setFont (UI::font (11.5f, true, 0.25f));
        g.drawText ("OUTPUT", (int) b.getX() + 32, (int) b.getY() + 10, 160, 20, juce::Justification::centredLeft);

        auto inner = b.reduced (18.0f, 0.0f);
        inner.removeFromTop (48.0f);
        auto readout = inner.removeFromBottom (50.0f);
        inner.removeFromBottom (4.0f);

        // dB scale on the left
        auto scale = inner.removeFromLeft (36.0f);
        g.setFont (UI::font (10.5f, true));
        for (float db : { 0.0f, -6.0f, -12.0f, -24.0f, -48.0f })
        {
            const float y = inner.getBottom() - toProp (db) * inner.getHeight();
            g.setColour (UI::textDim);
            g.drawText (juce::String ((int) db), (int) scale.getX(), (int) (y - 8.0f), (int) scale.getWidth() - 6, 16,
                        juce::Justification::centredRight);
            g.setColour (UI::panelEdge);
            g.drawHorizontalLine ((int) y, inner.getX(), inner.getRight());
        }

        // Two bars, centred
        const float barW = 34.0f, gap = 16.0f;
        const float x0 = inner.getCentreX() - (barW * 2.0f + gap) * 0.5f;
        drawBar (g, { x0, inner.getY(), barW, inner.getHeight() }, dbL);
        drawBar (g, { x0 + barW + gap, inner.getY(), barW, inner.getHeight() }, dbR);

        // Peak read-out
        g.setColour (UI::textDim);
        g.setFont (UI::font (10.5f, true, 0.2f));
        g.drawText ("PEAK", readout.removeFromTop (16.0f), juce::Justification::centred);
        g.setColour (peakHoldDb > -0.5f ? UI::maroonHi : UI::text);
        g.setFont (UI::font (15.0f, true));
        g.drawText (peakHoldDb <= -90.0f ? juce::String ("-inf") : juce::String (peakHoldDb, 1) + " dB",
                    readout, juce::Justification::centred);
    }

private:
    static float toProp (float db) { return juce::jmap (juce::jlimit (-60.0f, 0.0f, db), -60.0f, 0.0f, 0.0f, 1.0f); }

    static void drawBar (juce::Graphics& g, juce::Rectangle<float> r, float db)
    {
        g.setColour (juce::Colour (0xff1c1c21));
        g.fillRoundedRectangle (r, 6.0f);

        if (db > -59.0f)
        {
            const float h = juce::jmax (6.0f, r.getHeight() * toProp (db));
            auto fill = r.withTop (r.getBottom() - h);
            g.setGradientFill (juce::ColourGradient (UI::maroon.darker (0.4f), 0.0f, r.getBottom(),
                                                     UI::maroonHi, 0.0f, r.getY(), false));
            g.fillRoundedRectangle (fill, 6.0f);
        }
    }

    void timerCallback() override
    {
        const float pl = proc.peakL.exchange (0.0f);
        const float pr = proc.peakR.exchange (0.0f);

        const float newL = juce::Decibels::gainToDecibels (pl, -90.0f);
        const float newR = juce::Decibels::gainToDecibels (pr, -90.0f);

        dbL = juce::jmax (newL, dbL - 1.6f);   // ~48 dB/s fall-off
        dbR = juce::jmax (newR, dbR - 1.6f);

        const float newPeak = juce::jmax (newL, newR);
        if (newPeak >= peakHoldDb) { peakHoldDb = newPeak; holdFrames = 90; }
        else if (--holdFrames <= 0) peakHoldDb = juce::jmax (newPeak, peakHoldDb - 0.8f);

        repaint();
    }

    MasterProcessor& proc;
    float dbL = -90.0f, dbR = -90.0f, peakHoldDb = -90.0f;
    int holdFrames = 0;
};
