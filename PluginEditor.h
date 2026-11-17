#pragma once

#include "PluginProcessor.h"
#include "UI.h"

class MasterEditor : public juce::AudioProcessorEditor
{
public:
    explicit MasterEditor (MasterProcessor&);
    ~MasterEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // One control = a knob (slider) or a dropdown (for choice parameters), plus its label.
    struct Control
    {
        bool isChoice = false;
        juce::Slider   slider;
        juce::ComboBox combo;
        juce::Label    label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   sliderAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAttachment;
    };

    void showPresetMenu();
    void stepPreset (int delta);
    void loadPreset (int index);
    void refreshPresetName();

    MasterProcessor& proc;
    UMLookAndFeel laf;

    std::array<Control, UM::kNumParams> controls;
    juce::TextButton prevButton { "<" }, nextButton { ">" }, presetButton;
    LevelMeter meter;

    std::array<juce::Rectangle<int>, 5> panelBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterEditor)
};
