#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr float kLowShelfHz  = 100.0f;
constexpr float kMidHz       = 2500.0f;
constexpr float kHighShelfHz = 9000.0f;

inline float dbToGain (float db) noexcept { return juce::Decibels::decibelsToGain (db); }
} // namespace

//==============================================================================
MasterProcessor::MasterProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", UM::createLayout())
{
    for (int i = 0; i < UM::kNumParams; ++i)
        raw[(size_t) i] = apvts.getRawParameterValue (UM::kParamIds[i]);
}

bool MasterProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
void MasterProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    hpf.prepare (spec);
    lowShelf.prepare (spec);
    midPeak.prepare (spec);
    highShelf.prepare (spec);
    hpf.reset(); lowShelf.reset(); midPeak.reset(); highShelf.reset();

    dry.setSize (2, juce::jmax (1, samplesPerBlock));
    fxBuf.setSize (2, juce::jmax (1, samplesPerBlock));

    svf.prepare (spec);
    svf.reset();
    cutSm.reset (sr, 0.03);
    cutSm.setCurrentAndTargetValue (20000.0f);
    fmixSm.reset (sr, 0.02);
    fmixSm.setCurrentAndTargetValue (0.0f);
    gateGain = 1.0f;
    freePpq = 0.0;

    // Compressor ballistics: fixed 15 ms attack / 150 ms release (glue-style).
    compAttack  = std::exp (-1.0f / (0.015f * (float) sr));
    compRelease = std::exp (-1.0f / (0.150f * (float) sr));
    grEnv = 0.0f;

    // Limiter: 1.5 ms look-ahead, ~0.5 ms attack, 80 ms release.
    lookahead = juce::jmax (1, (int) std::lround (0.0015 * sr));
    delayL.assign ((size_t) lookahead + 1, 0.0f);
    delayR.assign ((size_t) lookahead + 1, 0.0f);
    delayPos = 0;
    limiterGain = 1.0f;
    limAttack  = 1.0f - std::exp (-1.0f / (0.0005f * (float) sr));
    limRelease = 1.0f - std::exp (-1.0f / (0.080f * (float) sr));
    setLatencySamples (lookahead);

    // Force the filter coefficients to be rebuilt.
    lastLowCut = lastLow = lastMid = lastHigh = -9999.0f;
    updateFilters();
}

void MasterProcessor::updateFilters()
{
    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    const float lc   = getParam (UM::pLowCut);
    const float low  = getParam (UM::pLow);
    const float mid  = getParam (UM::pMid);
    const float high = getParam (UM::pHigh);

    hpfActive = lc > 20.5f;

    if (lc != lastLowCut)
    {
        *hpf.state = *Coeffs::makeHighPass (sr, juce::jmax (20.0f, lc), 0.707f);
        lastLowCut = lc;
    }
    if (low != lastLow)
    {
        *lowShelf.state = *Coeffs::makeLowShelf (sr, kLowShelfHz, 0.707f, dbToGain (low));
        lastLow = low;
    }
    if (mid != lastMid)
    {
        *midPeak.state = *Coeffs::makePeakFilter (sr, kMidHz, 0.8f, dbToGain (mid));
        lastMid = mid;
    }
    if (high != lastHigh)
    {
        *highShelf.state = *Coeffs::makeHighShelf (sr, kHighShelfHz, 0.707f, dbToGain (high));
        lastHigh = high;
    }
}

//==============================================================================
void MasterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n == 0)
        return;

    const float comp   = getParam (UM::pComp)  * 0.01f;
    const float width  = getParam (UM::pWidth) * 0.01f;
    const float mix    = getParam (UM::pMix)   * 0.01f;
    const float inGain = dbToGain (getParam (UM::pIn));
    const float loudDb = getParam (UM::pLoud) * 0.12f;          // 0..100 % -> 0..+12 dB into the limiter
    const float ceilLin = dbToGain (getParam (UM::pCeil));
    const float outGain = dbToGain (getParam (UM::pOut));

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    // 1) Input trim ------------------------------------------------------------
    buffer.applyGain (inGain);

    // Keep a copy of the trimmed input for the Mix control.
    if (dry.getNumSamples() < n)
        dry.setSize (2, n, false, false, true);
    dry.copyFrom (0, 0, buffer, 0, 0, n);
    dry.copyFrom (1, 0, buffer, 1, 0, n);

    // 2) Tone: low cut -> low shelf -> mid bell -> high shelf --------------------
    updateFilters();
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        if (hpfActive) hpf.process (ctx);
        lowShelf.process (ctx);
        midPeak.process (ctx);
        highShelf.process (ctx);
    }

    // 3) Glue compressor (stereo linked, soft knee, auto makeup) -------------------
    if (comp > 0.001f)
    {
        const float thr    = -4.0f - 20.0f * comp;
        const float ratio  = 1.5f + 3.0f * comp;
        const float knee   = 8.0f;
        const float slope  = 1.0f - 1.0f / ratio;
        const float makeup = -thr * slope * 0.4f;

        for (int i = 0; i < n; ++i)
        {
            const float peak = juce::jmax (std::abs (L[i]), std::abs (R[i]));
            const float x    = juce::Decibels::gainToDecibels (peak, -100.0f);
            const float d    = x - thr;

            float gr = 0.0f;
            if (2.0f * d >= -knee)
            {
                if (2.0f * std::abs (d) <= knee)
                {
                    const float t = d + knee * 0.5f;
                    gr = slope * t * t / (2.0f * knee);
                }
                else
                    gr = d * slope;
            }

            const float coef = gr > grEnv ? compAttack : compRelease;
            grEnv = coef * grEnv + (1.0f - coef) * gr;

            const float g = dbToGain (makeup - grEnv);
            L[i] *= g;
            R[i] *= g;
        }
    }
    else
        grEnv = 0.0f;

    // 4) Stereo width (mid/side) ---------------------------------------------------
    if (std::abs (width - 1.0f) > 0.001f)
    {
        for (int i = 0; i < n; ++i)
        {
            const float m = 0.5f * (L[i] + R[i]);
            const float s = 0.5f * (L[i] - R[i]) * width;
            L[i] = m + s;
            R[i] = m - s;
        }
    }

    // 5) Dry/wet mix ---------------------------------------------------------------
    if (mix < 0.999f)
    {
        const float* dL = dry.getReadPointer (0);
        const float* dR = dry.getReadPointer (1);
        for (int i = 0; i < n; ++i)
        {
            L[i] = L[i] * mix + dL[i] * (1.0f - mix);
            R[i] = R[i] * mix + dR[i] * (1.0f - mix);
        }
    }

    // 6) FX filter (low/high/band-pass with smoothed cutoff) --------------------------
    {
        const float fmixTarget = getParam (UM::pFMix) * 0.01f;
        const float cutoff = juce::jlimit (20.0f, 0.45f * (float) sr, getParam (UM::pFCut));
        cutSm.setTargetValue (cutoff);
        fmixSm.setTargetValue (fmixTarget);

        if (fmixTarget > 0.001f || fmixSm.isSmoothing())
        {
            const float reso = getParam (UM::pFReso) * 0.01f;
            svf.setResonance (0.707f + reso * reso * 9.0f);

            switch ((int) getParam (UM::pFType))
            {
                case 1:  svf.setType (juce::dsp::StateVariableTPTFilterType::highpass); break;
                case 2:  svf.setType (juce::dsp::StateVariableTPTFilterType::bandpass); break;
                default: svf.setType (juce::dsp::StateVariableTPTFilterType::lowpass);  break;
            }

            if (fxBuf.getNumSamples() < n)
                fxBuf.setSize (2, n, false, false, true);
            fxBuf.copyFrom (0, 0, buffer, 0, 0, n);
            fxBuf.copyFrom (1, 0, buffer, 1, 0, n);

            juce::dsp::AudioBlock<float> fxBlock (fxBuf.getArrayOfWritePointers(), 2, (size_t) n);
            constexpr int chunk = 16;
            for (int s = 0; s < n; s += chunk)
            {
                const int len = juce::jmin (chunk, n - s);
                svf.setCutoffFrequency (cutSm.skip (len));
                auto sub = fxBlock.getSubBlock ((size_t) s, (size_t) len);
                juce::dsp::ProcessContextReplacing<float> fxCtx (sub);
                svf.process (fxCtx);
            }

            const float* fL = fxBuf.getReadPointer (0);
            const float* fR = fxBuf.getReadPointer (1);
            for (int i = 0; i < n; ++i)
            {
                const float a = fmixSm.getNextValue();
                L[i] += (fL[i] - L[i]) * a;
                R[i] += (fR[i] - R[i]) * a;
            }
        }
        else
        {
            cutSm.skip (n);
            svf.reset();
        }
    }

    // 7) FX gate: tempo-synced chop (Gross Beat style) ----------------------------------
    {
        const float depth = getParam (UM::pGDepth) * 0.01f;

        if (depth > 0.001f || gateGain < 0.999f)
        {
            double bpm = 120.0;
            double ppq0 = freePpq;

            if (auto* ph = getPlayHead())
            {
                if (auto pos = ph->getPosition())
                {
                    if (auto b = pos->getBpm())
                        if (*b > 20.0) bpm = *b;

                    if (pos->getIsPlaying())
                        if (auto q = pos->getPpqPosition())
                            ppq0 = *q;
                }
            }

            const double perSample = bpm / (60.0 * sr);   // beats per sample
            freePpq = ppq0 + perSample * n;                // keeps running when the host is stopped

            const int rate = juce::jlimit (0, 5, (int) getParam (UM::pGRate));
            const double cycles = UM::kGateCyclesPerBeat[rate];

            const float smooth = getParam (UM::pGSmooth) * 0.01f;
            const float halfCycleSec = (float) (0.5 * 60.0 / (bpm * cycles));
            const float tc = juce::jmap (smooth, 0.0004f, 0.45f * halfCycleSec);
            const float coef = std::exp (-1.0f / (tc * (float) sr));

            for (int i = 0; i < n; ++i)
            {
                const double x = (ppq0 + perSample * i) * cycles;
                const double phase = x - std::floor (x);
                const float target = phase < 0.5 ? 1.0f : 1.0f - depth;

                gateGain = coef * gateGain + (1.0f - coef) * target;
                L[i] *= gateGain;
                R[i] *= gateGain;
            }
        }
        else
        {
            gateGain = 1.0f;
        }
    }

    // 8) Loudness push into the look-ahead limiter ---------------------------------
    // The limiter is the last stage, so the ceiling is a true ceiling: the final
    // clamp catches anything the look-ahead gain smoothing lets through.
    buffer.applyGain (dbToGain (loudDb));

    const int size = lookahead + 1;
    for (int i = 0; i < n; ++i)
    {
        const float inL = L[i], inR = R[i];

        delayL[(size_t) delayPos] = inL;
        delayR[(size_t) delayPos] = inR;
        delayPos = (delayPos + 1) % size;

        const float peak   = juce::jmax (std::abs (inL), std::abs (inR));
        const float target = peak > ceilLin ? ceilLin / peak : 1.0f;
        const float coef   = target < limiterGain ? limAttack : limRelease;
        limiterGain += (target - limiterGain) * coef;

        // delayPos now points at the oldest sample (written `lookahead` samples ago)
        L[i] = juce::jlimit (-ceilLin, ceilLin, delayL[(size_t) delayPos] * limiterGain);
        R[i] = juce::jlimit (-ceilLin, ceilLin, delayR[(size_t) delayPos] * limiterGain);
    }

    // 9) Output trim (negative only, so the ceiling can never be exceeded) -----------
    buffer.applyGain (outGain);

    // Meters
    const float pl = buffer.getMagnitude (0, 0, n);
    const float pr = buffer.getMagnitude (1, 0, n);
    peakL.store (juce::jmax (peakL.load(), pl));
    peakR.store (juce::jmax (peakR.load(), pr));
}

//==============================================================================
void MasterProcessor::applyPreset (int index)
{
    const auto& presets = UM::getPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size()))
        return;

    const auto& preset = presets[(size_t) index];
    for (int i = 0; i < UM::kNumParams; ++i)
        if (auto* p = apvts.getParameter (UM::kParamIds[i]))
            p->setValueNotifyingHost (p->convertTo0to1 (preset.v[(size_t) i]));

    apvts.state.setProperty ("preset", index, nullptr);
}

int MasterProcessor::getCurrentPreset() const
{
    return (int) apvts.state.getProperty ("preset", -1);
}

//==============================================================================
juce::AudioProcessorEditor* MasterProcessor::createEditor()
{
    return new MasterEditor (*this);
}

void MasterProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void MasterProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MasterProcessor();
}
