#pragma once

#include "Params.h"
#include "Presets.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class MasterProcessor : public juce::AudioProcessor
{
public:
    MasterProcessor();
    ~MasterProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Slax"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Preset handling (index into UM::getPresets(), -1 = none / custom)
    void applyPreset (int index);
    int  getCurrentPreset() const;

    juce::AudioProcessorValueTreeState apvts;

    // Peak values since the editor last read them (editor calls exchange(0)).
    std::atomic<float> peakL { 0.0f }, peakR { 0.0f };

private:
    using Duplicator = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                      juce::dsp::IIR::Coefficients<float>>;

    void updateFilters();
    float getParam (int index) const noexcept { return raw[(size_t) index]->load(); }

    std::array<std::atomic<float>*, UM::kNumParams> raw {};

    // Tone
    Duplicator hpf, lowShelf, midPeak, highShelf;
    bool hpfActive = false;
    float lastLowCut = -9999.0f, lastLow = -9999.0f, lastMid = -9999.0f, lastHigh = -9999.0f;

    // Compressor state (stereo linked)
    float grEnv = 0.0f, compAttack = 0.0f, compRelease = 0.0f;

    // Look-ahead limiter state
    std::vector<float> delayL, delayR;
    int delayPos = 0, lookahead = 1;
    float limiterGain = 1.0f, limAttack = 0.0f, limRelease = 0.0f;

    // FX: filter
    juce::dsp::StateVariableTPTFilter<float> svf;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> cutSm;
    juce::SmoothedValue<float> fmixSm;
    juce::AudioBuffer<float> fxBuf;

    // FX: gate
    float gateGain = 1.0f;
    double freePpq = 0.0;

    juce::AudioBuffer<float> dry;
    double sr = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterProcessor)
};
