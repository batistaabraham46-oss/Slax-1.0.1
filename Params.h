#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace UM
{
inline constexpr int kNumParams = 18;

// The order here matters: the preset tables and the editor's panels both follow it.
enum ParamIndex
{
    // TONE
    pLowCut = 0, pLow, pMid, pHigh,
    // DYNAMICS
    pComp, pWidth, pMix,
    // LEVEL
    pIn, pLoud, pCeil, pOut,
    // FILTER (FX)
    pFType, pFCut, pFReso, pFMix,
    // GATE (FX, tempo-synced chop like Gross Beat)
    pGRate, pGDepth, pGSmooth
};

inline constexpr const char* kParamIds[kNumParams] =
    { "lowcut", "low", "mid", "high",
      "comp", "width", "mix",
      "in", "loud", "ceil", "out",
      "ftype", "fcut", "freso", "fmix",
      "grate", "gdepth", "gsmooth" };

inline constexpr const char* kParamNames[kNumParams] =
    { "Low Cut", "Low", "Mid", "High",
      "Comp", "Width", "Mix",
      "Input", "Loudness", "Ceiling", "Output",
      "Type", "Cutoff", "Reso", "Amount",
      "Rate", "Depth", "Smooth" };

// Which parameters live in which panel (first index + how many)
struct PanelDef { const char* title; int first; int count; };
inline constexpr PanelDef kPanels[5] =
{
    { "TONE",     pLowCut, 4 },
    { "DYNAMICS", pComp,   3 },
    { "LEVEL",    pIn,     4 },
    { "FILTER",   pFType,  4 },
    { "GATE",     pGRate,  3 }
};

inline constexpr const char* kFilterTypes[3] = { "Low-pass", "High-pass", "Band-pass" };

inline constexpr const char* kGateRates[6] = { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" };
inline constexpr double kGateCyclesPerBeat[6] = { 1.0, 2.0, 3.0, 4.0, 6.0, 8.0 };

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using FP     = juce::AudioParameterFloat;
    using Choice = juce::AudioParameterChoice;
    using Attr   = juce::AudioParameterFloatAttributes;
    using Range  = juce::NormalisableRange<float>;
    using PID    = juce::ParameterID;

    auto dbText  = [] (float v, int) { return juce::String (v, 1) + " dB"; };
    auto pctText = [] (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; };
    auto hzText  = [] (float v, int)
    {
        return v <= 20.5f ? juce::String ("Off") : juce::String (juce::roundToInt (v)) + " Hz";
    };
    auto cutText = [] (float v, int)
    {
        return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
    };

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // TONE
    layout.add (std::make_unique<FP> (PID { "lowcut", 1 }, "Low Cut", Range (20.0f, 300.0f, 1.0f, 0.4f), 20.0f,
                                      Attr().withStringFromValueFunction (hzText)));
    layout.add (std::make_unique<FP> (PID { "low", 1 },  "Low",  Range (-9.0f, 9.0f, 0.1f), 0.0f,
                                      Attr().withStringFromValueFunction (dbText)));
    layout.add (std::make_unique<FP> (PID { "mid", 1 },  "Mid",  Range (-6.0f, 6.0f, 0.1f), 0.0f,
                                      Attr().withStringFromValueFunction (dbText)));
    layout.add (std::make_unique<FP> (PID { "high", 1 }, "High", Range (-9.0f, 9.0f, 0.1f), 0.0f,
                                      Attr().withStringFromValueFunction (dbText)));

    // DYNAMICS
    layout.add (std::make_unique<FP> (PID { "comp", 1 },  "Comp",  Range (0.0f, 100.0f, 1.0f), 0.0f,
                                      Attr().withStringFromValueFunction (pctText)));
    layout.add (std::make_unique<FP> (PID { "width", 1 }, "Width", Range (0.0f, 200.0f, 1.0f), 100.0f,
                                      Attr().withStringFromValueFunction (pctText)));
    layout.add (std::make_unique<FP> (PID { "mix", 1 },   "Mix",   Range (0.0f, 100.0f, 1.0f), 100.0f,
                                      Attr().withStringFromValueFunction (pctText)));

    // LEVEL
    layout.add (std::make_unique<FP> (PID { "in", 1 },   "Input",    Range (-12.0f, 12.0f, 0.1f), 0.0f,
                                      Attr().withStringFromValueFunction (dbText)));
    layout.add (std::make_unique<FP> (PID { "loud", 1 }, "Loudness", Range (0.0f, 100.0f, 1.0f), 0.0f,
                                      Attr().withStringFromValueFunction (pctText)));
    layout.add (std::make_unique<FP> (PID { "ceil", 1 }, "Ceiling",  Range (-6.0f, 0.0f, 0.1f), -0.3f,
                                      Attr().withStringFromValueFunction (dbText)));
    layout.add (std::make_unique<FP> (PID { "out", 1 },  "Output",   Range (-12.0f, 0.0f, 0.1f), 0.0f,
                                      Attr().withStringFromValueFunction (dbText)));

    // FILTER
    layout.add (std::make_unique<Choice> (PID { "ftype", 1 }, "Filter Type",
                                          juce::StringArray { kFilterTypes[0], kFilterTypes[1], kFilterTypes[2] }, 0));
    layout.add (std::make_unique<FP> (PID { "fcut", 1 }, "Filter Cutoff", Range (20.0f, 20000.0f, 1.0f, 0.2f), 20000.0f,
                                      Attr().withStringFromValueFunction (cutText)));
    layout.add (std::make_unique<FP> (PID { "freso", 1 }, "Filter Reso", Range (0.0f, 100.0f, 1.0f), 0.0f,
                                      Attr().withStringFromValueFunction (pctText)));
    layout.add (std::make_unique<FP> (PID { "fmix", 1 }, "Filter Amount", Range (0.0f, 100.0f, 1.0f), 0.0f,
                                      Attr().withStringFromValueFunction (pctText)));

    // GATE
    layout.add (std::make_unique<Choice> (PID { "grate", 1 }, "Gate Rate",
                                          juce::StringArray { kGateRates[0], kGateRates[1], kGateRates[2],
                                                              kGateRates[3], kGateRates[4], kGateRates[5] }, 3));
    layout.add (std::make_unique<FP> (PID { "gdepth", 1 }, "Gate Depth", Range (0.0f, 100.0f, 1.0f), 0.0f,
                                      Attr().withStringFromValueFunction (pctText)));
    layout.add (std::make_unique<FP> (PID { "gsmooth", 1 }, "Gate Smooth", Range (0.0f, 100.0f, 1.0f), 30.0f,
                                      Attr().withStringFromValueFunction (pctText)));

    return layout;
}
} // namespace UM
