# Slax (v1.0.0)

A mastering plugin (VST3 + Standalone, plus AU on macOS) with one-click master presets
for underground and DMV sounds. Black and maroon UI, 12 knobs, built with JUCE.

## What's in it

Signal chain, in order:

1. **Input** trim
2. **Tone**: Low Cut (HPF), Low shelf (100 Hz), Mid bell (2.5 kHz), High shelf (9 kHz)
3. **Comp**: stereo-linked glue compressor with soft knee and auto makeup
4. **Width**: mid/side stereo width (0-200 %)
5. **Mix**: dry/wet of the processing above
6. **Filter (FX)**: low-pass / high-pass / band-pass with Cutoff, Reso and Amount (smoothed, so sweeping is click-free)
7. **Gate (FX)**: tempo-synced volume chop, like Gross Beat. Rate (1/4 to 1/32, plus triplets), Depth, and Smooth (square to soft edges). Follows the host tempo; in the standalone app it free-runs at 120 BPM.
8. **Loudness**: pushes up to +12 dB into the limiter
9. **Ceiling**: look-ahead limiter (1.5 ms) with a hard clamp, so it never goes over
10. **Output**: final trim (0 to -12 dB)

## Presets

Two per artist (**Balanced** and **Pushed**), grouped by scene, plus a few utility presets:

- **Underground**: Ayerp, Che, Rexv2, Luhpray, Maajins
- **DMV**: Noid4l, Slimegetem, Dragnutz, Killjae
- **FX**: Muffled, Low-pass Sweep Start, High-pass Build, Telephone, and gate presets (1/4 pump, 1/8 chop, 1/16 stutter, trance gate, filtered gate)
- **Utility**: Init, Gentle Glue, Loudness Only, Dark & Heavy, Bright & Airy

Important: these were built by hand as starting points for each scene's general sound.
They were not measured or analyzed from any artist's real masters. Load one, then A/B it
against a reference track and adjust to taste.

To edit or add presets, open `Source/Presets.h`. Each preset is one line of 12 numbers
(the format is documented at the top of the file).

## Building

You need **CMake 3.22+**, **git**, and a C++ compiler:

- Windows: Visual Studio 2022 (with "Desktop development with C++")
- macOS: Xcode command line tools
- Linux: gcc/clang plus the JUCE Linux dependencies
  (https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md)

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The first configure downloads JUCE (needs internet). Your plugin ends up in:

```
build/Slax_artefacts/Release/VST3/Slax.vst3
build/Slax_artefacts/Release/Standalone/   (a test app, no DAW needed)
```

Copy the `.vst3` folder into your VST3 folder and rescan in your DAW:

- Windows: `C:\Program Files\Common Files\VST3\`
- macOS: `~/Library/Audio/Plug-Ins/VST3/`
- Linux: `~/.vst3/`

## Notes

- Reports 66 samples (~1.5 ms) of latency at 44.1 kHz for the limiter look-ahead; hosts compensate automatically.
- JUCE is free for personal and open-source use. If you plan to sell the plugin, check JUCE's licensing tiers.
- Ideas for next steps: bass-mono below ~120 Hz, a real multiband stage, an A/B compare button.

## Building without installing anything (GitHub Actions)

1. Make a free account at github.com and create a new repository (any name, Private is fine).
2. Click "uploading an existing file" and drag in all the files from this folder
   (CMakeLists.txt, README.md and the .cpp/.h files). Commit.
3. Add the build recipe: Add file > Create new file. For the name, type exactly
   `.github/workflows/build.yml` (typing the slashes makes the folders). Paste in the
   contents of `build-recipe.txt` from this folder, then Commit.
4. Open the Actions tab. The build starts by itself (about 10 to 15 minutes).
5. When it finishes, open the run and download **Slax-Windows** from the Artifacts section at the bottom.
6. Unzip it, copy `Slax.vst3` from the VST3 folder into `C:\Program Files\Common Files\VST3\`, then rescan plugins in FL Studio.
