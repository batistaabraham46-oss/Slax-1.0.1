#include "PluginEditor.h"

namespace
{
constexpr int kWidth = 880, kHeight = 680;
constexpr int kMargin = 20, kHeaderH = 84, kGap = 14, kPanelTitleH = 38;
}

MasterEditor::MasterEditor (MasterProcessor& p)
    : AudioProcessorEditor (&p), proc (p), meter (p)
{
    setLookAndFeel (&laf);

    for (int i = 0; i < UM::kNumParams; ++i)
    {
        auto& k = controls[(size_t) i];
        auto* param = proc.apvts.getParameter (UM::kParamIds[i]);
        auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param);
        k.isChoice = choice != nullptr;

        if (k.isChoice)
        {
            k.combo.addItemList (choice->choices, 1);
            k.combo.setJustificationType (juce::Justification::centred);
            addAndMakeVisible (k.combo);
            k.comboAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
                proc.apvts, UM::kParamIds[i], k.combo);
        }
        else
        {
            k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 20);
            k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                          juce::MathConstants<float>::pi * 2.8f, true);
            addAndMakeVisible (k.slider);

            k.sliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                proc.apvts, UM::kParamIds[i], k.slider);

            if (param != nullptr)
                k.slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
        }

        k.label.setText (juce::String (UM::kParamNames[i]).toUpperCase(), juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setColour (juce::Label::textColourId, UI::textDim);
        k.label.setFont (UI::font (11.0f, true, 0.18f));
        k.label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (k.label);
    }

    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (+1); };
    presetButton.onClick = [this] { showPresetMenu(); };
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);
    addAndMakeVisible (presetButton);
    addAndMakeVisible (meter);

    refreshPresetName();
    setSize (kWidth, kHeight);
}

MasterEditor::~MasterEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
void MasterEditor::refreshPresetName()
{
    const int idx = proc.getCurrentPreset();
    const auto& presets = UM::getPresets();

    if (juce::isPositiveAndBelow (idx, (int) presets.size()))
        presetButton.setButtonText (presets[(size_t) idx].name);
    else
        presetButton.setButtonText ("Select a preset");
}

void MasterEditor::loadPreset (int index)
{
    proc.applyPreset (index);
    refreshPresetName();
}

void MasterEditor::stepPreset (int delta)
{
    const int count = (int) UM::getPresets().size();
    const int cur   = proc.getCurrentPreset();
    const int next  = cur < 0 ? (delta > 0 ? 0 : count - 1)
                              : (cur + delta + count) % count;
    loadPreset (next);
}

void MasterEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    const auto& presets = UM::getPresets();
    const int current = proc.getCurrentPreset();

    juce::String lastGroup;
    for (int i = 0; i < (int) presets.size(); ++i)
    {
        const auto& pr = presets[(size_t) i];
        if (pr.group != lastGroup)
        {
            if (lastGroup.isNotEmpty())
                menu.addSeparator();
            menu.addSectionHeader (pr.group.toUpperCase());
            lastGroup = pr.group;
        }
        menu.addItem (i + 1, pr.name, true, i == current);
    }

    juce::Component::SafePointer<MasterEditor> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options()
                            .withTargetComponent (&presetButton)
                            .withMinimumWidth (presetButton.getWidth()),
                        [safe] (int result)
                        {
                            if (safe != nullptr && result > 0)
                                safe->loadPreset (result - 1);
                        });
}

//==============================================================================
void MasterEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (UI::bg, 0.0f, 0.0f, UI::bgLow, 0.0f, (float) getHeight(), false));
    g.fillAll();

    // Title
    g.setColour (UI::maroon);
    g.fillRoundedRectangle (24.0f, 24.0f, 5.0f, 34.0f, 2.5f);

    g.setColour (UI::text);
    g.setFont (UI::font (23.0f, true, 0.12f));
    g.drawText ("SLAX", 40, 20, 330, 28, juce::Justification::centredLeft);

    g.setColour (UI::textDim);
    g.setFont (UI::font (11.0f, true, 0.2f));
    g.drawText ("MASTERING PRESETS  |  v1.0.0", 40, 46, 330, 16, juce::Justification::centredLeft);

    // Header divider (maroon fading out to the right)
    g.setGradientFill (juce::ColourGradient (UI::maroon, (float) kMargin, 0.0f,
                                             UI::maroon.withAlpha (0.0f), (float) (getWidth() - kMargin), 0.0f, false));
    g.fillRect ((float) kMargin, (float) kHeaderH - 6.0f, (float) (getWidth() - 2 * kMargin), 1.5f);

    // Panels
    for (int i = 0; i < 5; ++i)
    {
        auto r = panelBounds[(size_t) i].toFloat();
        g.setColour (UI::panel);
        g.fillRoundedRectangle (r, 14.0f);
        g.setColour (UI::panelEdge);
        g.drawRoundedRectangle (r.reduced (0.5f), 14.0f, 1.0f);

        g.setColour (UI::maroonHi);
        g.fillEllipse (r.getX() + 18.0f, r.getY() + 17.0f, 6.0f, 6.0f);

        g.setColour (UI::textDim);
        g.setFont (UI::font (11.5f, true, 0.25f));
        g.drawText (UM::kPanels[i].title, (int) r.getX() + 32, (int) r.getY() + 10, 160, 20,
                    juce::Justification::centredLeft);
    }
}

void MasterEditor::resized()
{
    auto area = getLocalBounds();

    // Header: [<] [ preset name ] [>] on the right
    auto header = area.removeFromTop (kHeaderH).reduced (kMargin, 0);
    auto presetRow = header.removeFromRight (400).withTrimmedTop (22).withHeight (38);
    nextButton.setBounds (presetRow.removeFromRight (42));
    presetRow.removeFromRight (8);
    prevButton.setBounds (presetRow.removeFromLeft (42));
    presetRow.removeFromLeft (8);
    presetButton.setBounds (presetRow);

    // Panels: 3 columns x 2 rows. Slots 0-4 are control panels, slot 5 is the meter.
    auto body = area.reduced (kMargin, 0).withTrimmedBottom (kMargin);
    const int panelW = (body.getWidth() - 2 * kGap) / 3;
    const int panelH = (body.getHeight() - kGap) / 2;

    auto slotRect = [&] (int slot)
    {
        return juce::Rectangle<int> (body.getX() + (slot % 3) * (panelW + kGap),
                                     body.getY() + (slot / 3) * (panelH + kGap), panelW, panelH);
    };

    for (int p = 0; p < 5; ++p)
    {
        const auto& def = UM::kPanels[p];
        panelBounds[(size_t) p] = slotRect (p);

        auto grid = panelBounds[(size_t) p].withTrimmedTop (kPanelTitleH).reduced (8, 4);
        const int cellW = grid.getWidth() / 2;
        const int cellH = grid.getHeight() / 2;

        for (int k = 0; k < def.count; ++k)
        {
            auto& ctl = controls[(size_t) (def.first + k)];
            juce::Rectangle<int> cell (grid.getX() + (k % 2) * cellW,
                                       grid.getY() + (k / 2) * cellH, cellW, cellH);

            ctl.label.setBounds (cell.removeFromBottom (18));
            if (ctl.isChoice)
                ctl.combo.setBounds (cell.withSizeKeepingCentre (cellW - 28, 32));
            else
                ctl.slider.setBounds (cell);
        }
    }

    meter.setBounds (slotRect (5));
}
