#pragma once

#include "Params.h"
#include <array>
#include <vector>

// -----------------------------------------------------------------------------
// PRESET BANK
//
// Each artist gets two presets ("Balanced" and "Pushed"). These are hand-built
// starting points for the general sound of each scene (wide/bright for the
// underground side, tight/mid-forward for the DMV side). They were NOT measured
// from any artist's actual masters, so treat them as starting points and A/B
// against a reference track.
//
// The artist presets are intentionally moderate on loudness (the Loudness
// knob is what pushes level, so crank it yourself if you want it hotter).
//
// To add or tweak a preset, edit the tables below. The base() helper takes:
//   lowcut Hz, low dB, mid dB, high dB,
//   comp %, width %, mix %,
//   input dB, loudness %, ceiling dB, output dB
// and leaves the Filter and Gate effects switched off.
// -----------------------------------------------------------------------------
namespace UM
{
struct Preset
{
    juce::String group;
    juce::String name;
    std::array<float, kNumParams> v;
};

inline const std::vector<Preset>& getPresets()
{
    static const std::vector<Preset> list = []
    {
        using A = std::array<float, kNumParams>;
        std::vector<Preset> p;

        auto base = [] (float lowcut, float low, float mid, float high,
                        float comp, float width, float mix,
                        float in, float loud, float ceil, float out) -> A
        {
            A v {};
            v[pLowCut] = lowcut; v[pLow] = low; v[pMid] = mid; v[pHigh] = high;
            v[pComp] = comp; v[pWidth] = width; v[pMix] = mix;
            v[pIn] = in; v[pLoud] = loud; v[pCeil] = ceil; v[pOut] = out;
            // FX off by default
            v[pFType] = 0.0f; v[pFCut] = 20000.0f; v[pFReso] = 0.0f; v[pFMix] = 0.0f;
            v[pGRate] = 3.0f; v[pGDepth] = 0.0f; v[pGSmooth] = 30.0f;
            return v;
        };

        // "<artist> - Balanced" plus a "<artist> - Pushed" variant
        // (a bit more compression and loudness).
        auto addStyle = [&p] (const char* group, const char* artist, const A& v)
        {
            p.push_back ({ group, juce::String (artist) + " - Balanced", v });

            A hard = v;
            hard[pComp] = juce::jmin (100.0f, v[pComp] + 8.0f);
            hard[pLoud] = juce::jmin (100.0f, v[pLoud] + 10.0f);
            p.push_back ({ group, juce::String (artist) + " - Pushed", hard });
        };

        // ------------------------------ UNDERGROUND ------------------------------
        //                       lowcut  low    mid    high  comp width mix  in  loud  ceil   out
        addStyle ("Underground", "Ayerp",
                  base (          28,    2.0f, -1.0f,  3.5f,  30,  140, 100,  0,  45, -0.3f, 0));
        addStyle ("Underground", "Che",
                  base (          25,    3.0f,  0.0f,  2.0f,  35,  118, 100,  0,  50, -0.3f, 0));
        addStyle ("Underground", "Rexv2",
                  base (          30,    2.0f,  0.5f,  2.5f,  40,  125, 100,  0,  55, -0.3f, 0));
        addStyle ("Underground", "Luhpray",
                  base (          25,    1.0f,  0.5f,  1.5f,  25,  120, 100,  0,  35, -0.3f, 0));
        addStyle ("Underground", "Maajins",
                  base (          22,    3.5f, -0.5f, -0.5f,  35,  108, 100,  0,  45, -0.3f, 0));

        // ---------------------------------- DMV ----------------------------------
        addStyle ("DMV", "Noid4l",
                  base (          30,    1.5f,  1.5f,  1.5f,  40,  100, 100,  0,  50, -0.3f, 0));
        addStyle ("DMV", "Slimegetem",
                  base (          30,    1.0f,  2.0f,  1.0f,  40,   95, 100,  0,  55, -0.3f, 0));
        addStyle ("DMV", "Dragnutz",
                  base (          28,    3.0f,  1.0f,  0.5f,  40,  100, 100,  0,  55, -0.3f, 0));
        addStyle ("DMV", "Killjae",
                  base (          28,    1.5f,  0.5f,  2.5f,  35,  110, 100,  0,  50, -0.3f, 0));

        // ----------------------------------- FX ------------------------------------
        // Filters and Gross Beat-style gates on an otherwise neutral master.
        auto addFx = [&p, &base] (const char* name,
                                  float ftype, float fcut, float freso, float fmix,
                                  float grate, float gdepth, float gsmooth)
        {
            A v = base (20, 0, 0, 0, 0, 100, 100, 0, 0, -0.3f, 0);
            v[pFType] = ftype; v[pFCut] = fcut; v[pFReso] = freso; v[pFMix] = fmix;
            v[pGRate] = grate; v[pGDepth] = gdepth; v[pGSmooth] = gsmooth;
            p.push_back ({ "FX", name, v });
        };

        //                           type  cutoff reso  amt  rate depth smooth
        addFx ("Muffled (Low-pass 400)",   0,   400,   10, 100,   3,   0,   30);
        addFx ("Low-pass Sweep Start",     0,   700,   25, 100,   3,   0,   30);
        addFx ("High-pass Build",          1,   600,   30, 100,   3,   0,   30);
        addFx ("Telephone",                2,  1500,   20, 100,   3,   0,   30);
        addFx ("Gate 1/4 Pump",            0, 20000,    0,   0,   0,  70,   50);
        addFx ("Gate 1/8 Chop",            0, 20000,    0,   0,   1, 100,   25);
        addFx ("Gate 1/16 Stutter",        0, 20000,    0,   0,   3, 100,   15);
        addFx ("Trance Gate 1/16T",        0, 20000,    0,   0,   4,  85,   40);
        addFx ("Filtered Gate 1/8",        0,  1200,   35, 100,   1,  80,   30);

        // --------------------------------- UTILITY --------------------------------
        p.push_back ({ "Utility", "Init",          base (20, 0.0f,  0.0f,  0.0f,  0, 100, 100, 0,  0, -0.3f, 0) });
        p.push_back ({ "Utility", "Gentle Glue",   base (20, 0.0f,  0.0f,  0.0f, 25, 100, 100, 0, 15, -0.3f, 0) });
        p.push_back ({ "Utility", "Loudness Only", base (20, 0.0f,  0.0f,  0.0f,  0, 100, 100, 0, 50, -0.3f, 0) });
        p.push_back ({ "Utility", "Dark & Heavy",  base (25, 3.5f, -0.5f, -3.0f, 35, 105, 100, 0, 45, -0.3f, 0) });
        p.push_back ({ "Utility", "Bright & Airy", base (25, 0.0f,  0.0f,  4.0f, 25, 125, 100, 0, 40, -0.3f, 0) });

        return p;
    }();

    return list;
}
} // namespace UM
