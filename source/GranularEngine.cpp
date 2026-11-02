#include "GranularEngine.h"
#include <algorithm>
#include <cmath>

namespace
{
    const int kChordIntervals[10][4] =
    {
        { 0, 4, 7, 12 }, { 0, 3, 7, 12 }, { 0, 4, 7, 11 }, { 0, 3, 7, 10 }, { 0, 4, 7, 10 },
        { 0, 2, 7, 12 }, { 0, 5, 7, 12 }, { 0, 3, 6, 9 },  { 0, 4, 8, 12 }, { 0, 4, 7, 14 }
    };

    struct ReverbTypeInfo { float base, range, width, dampMul; };
    const ReverbTypeInfo kReverbTypes[5] =
    {
        { 0.15f, 0.35f, 0.70f, 1.2f },   // Room
        { 0.35f, 0.45f, 1.00f, 0.7f },   // Plate
        { 0.50f, 0.40f, 1.00f, 1.0f },   // Hall
        { 0.70f, 0.28f, 1.00f, 0.9f },   // Cathedral
        { 0.80f, 0.20f, 1.00f, 0.6f }    // Space
    };
}

GranularEngine::GranularEngine()
{
    for (auto& v : vizData) v.store (0.0f, std::memory_order_relaxed);

    window.resize (windowSize + 1);
    for (int i = 0; i <= windowSize; ++i)   // Hann window
        window[(size_t) i] = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) windowSize));
}

void GranularEngine::prepare (double sr, int maxBlockSize, int channels)
{
    sampleRate = sr;
    maxBlock   = juce::jmax (1, maxBlockSize);
    numChannels = juce::jlimit (1, maxChannels, channels);

    delaySize = (int) (sampleRate * 8.0) + 8;
    chordHistSize = (int) (sampleRate * 0.7) + 8;
    chordCombSize = (int) (sampleRate / 25.0) + 8;
    preSize = (int) (sampleRate * 0.25) + 8;

    for (size_t c = 0; c < (size_t) maxChannels; ++c)
    {
        delayBuffer[c].assign ((size_t) delaySize, 0.0f);
        wetBuffer[c].assign ((size_t) maxBlock, 0.0f);
        reverbTmp[c].assign ((size_t) maxBlock, 0.0f);
        chordHist[c].assign ((size_t) chordHistSize, 0.0f);
        preDelay[c].assign ((size_t) preSize, 0.0f);
        for (auto& v : chordComb[c]) v.assign ((size_t) chordCombSize, 0.0f);
    }

    reverb.setSampleRate (sampleRate);

    smoothMix.reset (sampleRate, 0.03);
    smoothFeedback.reset (sampleRate, 0.03);

    reset();
}

void GranularEngine::reset()
{
    for (auto& b : delayBuffer) std::fill (b.begin(), b.end(), 0.0f);
    for (auto& b : chordHist)   std::fill (b.begin(), b.end(), 0.0f);
    for (auto& b : preDelay)    std::fill (b.begin(), b.end(), 0.0f);
    for (auto& ch : chordComb)
        for (auto& v : ch) std::fill (v.begin(), v.end(), 0.0f);

    for (auto& g : grains) g.active = false;

    for (auto& st : filterState) for (auto& s : st) s = {};
    for (auto& a : chordLp) for (auto& v : a) v = 0.0f;
    for (auto& v : distanceLp) v = 0.0f;
    for (auto& v : revHp) v = 0.0f;

    writePos = 0;
    chordHistPos = chordPos = prePos = 0;
    grainCountdown = 0.0;
    stretchOffset = 0.0;
    reverb.reset();
}

float GranularEngine::readInterpolated (const std::vector<float>& buf, double pos) const noexcept
{
    int i0 = (int) pos;
    const float frac = (float) (pos - (double) i0);
    if (i0 >= delaySize) i0 -= delaySize;
    int i1 = i0 + 1;
    if (i1 >= delaySize) i1 = 0;
    return buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * frac;
}

float GranularEngine::readFrac (const std::vector<float>& buf, int size, int pos, float delay) noexcept
{
    float rp = (float) pos - delay;
    while (rp < 0.0f) rp += (float) size;
    int i0 = (int) rp;
    const float f = rp - (float) i0;
    if (i0 >= size) i0 -= size;
    int i1 = i0 + 1;
    if (i1 >= size) i1 = 0;
    return buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * f;
}

GranularEngine::Coeffs GranularEngine::makeCoeffs (int type, float f, float q, float gainDb) const noexcept
{
    const float w0 = juce::MathConstants<float>::twoPi * f / (float) sampleRate;
    const float cw = std::cos (w0), sw = std::sin (w0);
    const float alpha = sw / (2.0f * q);
    const float A = std::pow (10.0f, gainDb / 40.0f);

    float b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;

    switch (type)
    {
        case 0: b0 = (1 - cw) * 0.5f; b1 = 1 - cw; b2 = b0; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;   // low-pass
        case 2: b0 = (1 + cw) * 0.5f; b1 = -(1 + cw); b2 = b0; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break; // high-pass
        case 4: b0 = alpha; b1 = 0; b2 = -alpha; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;               // band-pass
        case 5: b0 = 1; b1 = -2 * cw; b2 = 1; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;                 // notch
        case 6: b0 = 1 + alpha * A; b1 = -2 * cw; b2 = 1 - alpha * A;                                                // peak
                a0 = 1 + alpha / A; a1 = -2 * cw; a2 = 1 - alpha / A; break;
        case 9: b0 = 1 - alpha; b1 = -2 * cw; b2 = 1 + alpha; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break; // all-pass
        case 7: case 8:                                                                                              // shelves
        {
            const float beta = 2.0f * std::sqrt (A) * (sw * 0.5f * std::sqrt (2.0f));
            if (type == 7)
            {
                b0 = A * ((A + 1) - (A - 1) * cw + beta);
                b1 = 2 * A * ((A - 1) - (A + 1) * cw);
                b2 = A * ((A + 1) - (A - 1) * cw - beta);
                a0 = (A + 1) + (A - 1) * cw + beta;
                a1 = -2 * ((A - 1) + (A + 1) * cw);
                a2 = (A + 1) + (A - 1) * cw - beta;
            }
            else
            {
                b0 = A * ((A + 1) + (A - 1) * cw + beta);
                b1 = -2 * A * ((A - 1) + (A + 1) * cw);
                b2 = A * ((A + 1) + (A - 1) * cw - beta);
                a0 = (A + 1) - (A - 1) * cw + beta;
                a1 = 2 * ((A - 1) - (A + 1) * cw);
                a2 = (A + 1) - (A - 1) * cw - beta;
            }
            break;
        }
        default: break;
    }

    Coeffs c;
    c.b0 = b0 / a0; c.b1 = b1 / a0; c.b2 = b2 / a0; c.a1 = a1 / a0; c.a2 = a2 / a0;
    return c;
}

void GranularEngine::spawnGrain (const Params& p)
{
    Grain* g = nullptr;
    for (auto& candidate : grains)
        if (! candidate.active) { g = &candidate; break; }
    if (g == nullptr)
        return;

    const int length = juce::jmax (16, (int) (p.grainSizeMs * 0.001 * sampleRate));
    double speed = std::pow (2.0, (double) p.pitchSemitones / 12.0);
    speed *= 1.0 + (double) (random.nextFloat() * 2.0f - 1.0f) * (double) p.motion * 0.02;   // motion: slow per-grain detune drift

    // where in the stored audio the grain starts: delay + position offset + time-stretch scan offset
    double delay = (double) p.delayTimeMs * 0.001 * sampleRate;
    delay += (double) p.position * 1.5 * sampleRate;
    if (std::abs (p.stretch - 1.0f) > 0.01f)
        delay += stretchOffset;
    delay += (double) (random.nextFloat() * 2.0f - 1.0f) * (double) p.motion * 0.5 * (double) length;   // motion: scatter

    const double minDelay = 1.0 + (double) length * juce::jmax (0.0, speed - 1.0);
    const double maxDelay = (double) delaySize - 4.0 - (double) length * juce::jmax (1.0, 1.0 / speed);
    delay = juce::jlimit (minDelay, juce::jmax (minDelay, maxDelay), delay);

    double pos = (double) writePos - delay;
    while (pos < 0.0) pos += (double) delaySize;

    g->panStart = (random.nextFloat() * 2.0f - 1.0f) * p.spread;
    g->panEnd   = juce::jlimit (-1.0f, 1.0f, g->panStart + (random.nextFloat() * 2.0f - 1.0f) * p.motion);   // motion: pan travel
    g->panNow   = g->panStart;
    g->readPos = pos;
    g->speed   = speed;
    g->age     = 0;
    g->length  = length;
    g->active  = true;
}

void GranularEngine::process (juce::AudioBuffer<float>& buffer, const Params& p)
{
    const int total = buffer.getNumSamples();
    const int chans = juce::jmin (buffer.getNumChannels(), numChannels);
    if (chans <= 0 || delaySize == 0)
        return;

    smoothMix.setTargetValue (juce::jlimit (0.0f, 1.0f, p.mix));
    smoothFeedback.setTargetValue (juce::jlimit (0.0f, 0.95f, p.feedback));

    int done = 0;
    while (done < total)
    {
        const int n = juce::jmin (maxBlock, total - done);
        float* ptrs[maxChannels] = {};
        for (int c = 0; c < chans; ++c)
            ptrs[c] = buffer.getWritePointer (c) + done;
        processChunk (ptrs, n, p);
        done += n;
    }
}

void GranularEngine::processChunk (float* const* io, int numSamples, const Params& p)
{
    const int chans = numChannels;
    const float sr = (float) sampleRate;
    const double interval = sampleRate / (double) juce::jmax (0.1f, p.grainDensity);

    // input level for the visualiser
    {
        float peak = 0.0f;
        for (int c = 0; c < chans; ++c)
            for (int s = 0; s < numSamples; ++s)
                peak = juce::jmax (peak, std::abs (io[c][s]));
        vizInLevel.store (juce::jmin (1.0f, peak), std::memory_order_relaxed);
    }

    const float overlap = p.grainDensity * p.grainSizeMs * 0.001f;
    currentNorm = 1.0f / juce::jmax (1.0f, overlap * 0.5f);

    // saturation
    const float satAmt = juce::jlimit (0.0f, 1.0f, p.saturation);
    const bool useSat = satAmt > 0.001f;
    const float drive = 1.0f + satAmt * 9.0f;
    const float driveComp = 1.0f / std::sqrt (drive);

    // distance: quieter + darker
    const float dist = juce::jlimit (0.0f, 1.0f, p.distance);
    const float distGain = 1.0f - 0.5f * dist;
    const float distCut = juce::jlimit (200.0f, sr * 0.45f, 18000.0f * std::pow (0.1f, dist));
    const float distA = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * distCut / sr);
    const bool useDist = dist > 0.001f;

    // time-stretch scan offset (read head moves at 1/stretch of real time)
    if (! p.freeze)
    {
        const float st = juce::jlimit (0.25f, 4.0f, p.stretch);
        const double range = 1.5 * sampleRate;
        stretchOffset += (double) numSamples * (1.0 - 1.0 / (double) st);
        while (stretchOffset > range) stretchOffset -= range;
        while (stretchOffset < 0.0)   stretchOffset += range;
    }

    // ---- pass 1: grains -> wetBuffer (+ feedback into the delay line)
    for (int s = 0; s < numSamples; ++s)
    {
        grainCountdown -= 1.0;
        while (grainCountdown <= 0.0)
        {
            spawnGrain (p);
            grainCountdown += interval;
        }

        float wet[maxChannels] = { 0.0f, 0.0f };

        for (auto& g : grains)
        {
            if (! g.active) continue;

            const float t = (float) g.age / (float) g.length;
            const int wi = juce::jlimit (0, windowSize, (int) (t * (float) windowSize));
            const float env = window[(size_t) wi];

            const float pan = juce::jlimit (-1.0f, 1.0f, g.panStart + (g.panEnd - g.panStart) * t + p.pan);
            g.panNow = pan;
            const float gains[2] = { juce::jmin (1.0f, 1.0f - pan), juce::jmin (1.0f, 1.0f + pan) };

            for (int c = 0; c < chans; ++c)
                wet[c] += readInterpolated (delayBuffer[(size_t) c], g.readPos) * env * gains[c];

            g.readPos += g.speed;
            if (g.readPos >= (double) delaySize) g.readPos -= (double) delaySize;
            if (++g.age >= g.length) g.active = false;
        }

        const float fb = smoothFeedback.getNextValue();
        for (int c = 0; c < chans; ++c)
        {
            float w = wet[c] * currentNorm * distGain;

            if (useSat)
                w = std::tanh (w * drive) * driveComp;

            if (useDist)
            {
                distanceLp[(size_t) c] += distA * (w - distanceLp[(size_t) c]);
                w = distanceLp[(size_t) c];
            }
            else
                distanceLp[(size_t) c] = w;

            if (! p.freeze)
                delayBuffer[(size_t) c][(size_t) writePos] = io[c][s] + std::tanh (w * fb);

            wetBuffer[(size_t) c][(size_t) s] = w;
        }

        if (! p.freeze && ++writePos >= delaySize) writePos = 0;
    }

    // ---- pass 2: filter
    {
        const float f = juce::jlimit (20.0f, sr * 0.49f, p.filterCutoff);
        const float q = juce::jlimit (0.2f, 20.0f, p.filterRes);
        const int t = juce::jlimit (0, 9, p.filterType);
        filter24 = (t == 1 || t == 3);
        const int base = (t == 1) ? 0 : (t == 3) ? 2 : t;
        filterCoeffs[0] = makeCoeffs (base, f, q, p.filterGainDb);
        if (filter24) filterCoeffs[1] = makeCoeffs (base, f, 0.7071f, p.filterGainDb);

        for (int c = 0; c < chans; ++c)
        {
            auto* data = wetBuffer[(size_t) c].data();
            for (int s = 0; s < numSamples; ++s)
            {
                float x = data[s];
                for (int stage = 0; stage < (filter24 ? 2 : 1); ++stage)
                {
                    const auto& k = filterCoeffs[stage];
                    auto& st = filterState[c][stage];
                    const float y = k.b0 * x + st.z1;
                    st.z1 = k.b1 * x - k.a1 * y + st.z2;
                    st.z2 = k.b2 * x - k.a2 * y;
                    x = y;
                }
                data[s] = x;
            }
        }
    }

    // ---- pass 3: chord resonator (4 tuned feedback combs, strummed by "time")
    const float cmix = juce::jlimit (0.0f, 1.0f, p.chordMix);
    if (cmix > 0.001f)
    {
        const int type = juce::jlimit (0, 9, p.chordType);
        const int rootMidi = 48 + juce::jlimit (0, 11, p.chordRoot);
        const float decay = juce::jmax (0.05f, p.chordDecay);
        const float gainLin = std::pow (10.0f, p.chordGainDb / 20.0f);

        float delays[chordVoices], fbs[chordVoices], norms[chordVoices];
        int taps[chordVoices];
        for (int v = 0; v < chordVoices; ++v)
        {
            const float midi = (float) (rootMidi + kChordIntervals[type][v]);
            const float freq = 440.0f * std::pow (2.0f, (midi - 69.0f) / 12.0f);
            delays[v] = juce::jlimit (4.0f, (float) (chordCombSize - 3), sr / freq);
            fbs[v] = juce::jmin (0.995f, std::pow (0.001f, delays[v] / (sr * decay)));
            norms[v] = (1.0f - fbs[v]) * 0.5f;
            taps[v] = juce::jlimit (0, chordHistSize - 2, (int) ((float) v * p.chordTimeMs * 0.001f * sr));
        }

        for (int s = 0; s < numSamples; ++s)
        {
            for (int c = 0; c < chans; ++c)
            {
                const float x = wetBuffer[(size_t) c][(size_t) s];
                chordHist[(size_t) c][(size_t) chordHistPos] = x;

                float res = 0.0f;
                for (int v = 0; v < chordVoices; ++v)
                {
                    int ti = chordHistPos - taps[v];
                    if (ti < 0) ti += chordHistSize;
                    const float xin = chordHist[(size_t) c][(size_t) ti];

                    auto& comb = chordComb[(size_t) c][(size_t) v];
                    const float r = readFrac (comb, chordCombSize, chordPos, delays[v]);
                    float& lp = chordLp[c][v];
                    lp += 0.55f * (r - lp);
                    const float y = xin + fbs[v] * lp;
                    comb[(size_t) chordPos] = y;
                    res += y * norms[v];
                }

                wetBuffer[(size_t) c][(size_t) s] = x * (1.0f - cmix) + res * gainLin * cmix;
            }

            if (++chordHistPos >= chordHistSize) chordHistPos = 0;
            if (++chordPos >= chordCombSize) chordPos = 0;
        }
    }

    // ---- pass 4: reverb (type, decay, damping, pre-decay)
    const float rmix = juce::jlimit (0.0f, 1.0f, p.reverbMix);
    if (rmix > 0.001f)
    {
        const auto& ti = kReverbTypes[juce::jlimit (0, 4, p.reverbType)];

        juce::Reverb::Parameters rp;
        rp.roomSize = juce::jlimit (0.0f, 1.0f, ti.base + juce::jlimit (0.0f, 1.0f, p.reverbDecay) * ti.range);
        rp.damping = juce::jlimit (0.0f, 1.0f, p.reverbHighDamp * ti.dampMul);
        rp.wetLevel = 0.33f;
        rp.dryLevel = 0.0f;
        rp.width = ti.width;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);

        const int pre = juce::jlimit (0, preSize - 2, (int) (p.reverbPreDecayMs * 0.001f * sr));
        for (int s = 0; s < numSamples; ++s)
        {
            for (int c = 0; c < chans; ++c)
            {
                preDelay[(size_t) c][(size_t) prePos] = wetBuffer[(size_t) c][(size_t) s];
                int ri = prePos - pre;
                if (ri < 0) ri += preSize;
                reverbTmp[(size_t) c][(size_t) s] = preDelay[(size_t) c][(size_t) ri];
            }
            if (++prePos >= preSize) prePos = 0;
        }

        if (chans == 2) reverb.processStereo (reverbTmp[0].data(), reverbTmp[1].data(), numSamples);
        else            reverb.processMono (reverbTmp[0].data(), numSamples);

        // low damping: high-pass the reverb tail
        const float hpCut = 20.0f * std::pow (30.0f, juce::jlimit (0.0f, 1.0f, p.reverbLowDamp));
        const float hpA = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * hpCut / sr);
        const float send = juce::jmin (1.0f, rmix * (1.0f + 0.6f * dist));

        for (int c = 0; c < chans; ++c)
        {
            auto* wetData = wetBuffer[(size_t) c].data();
            auto* rev = reverbTmp[(size_t) c].data();
            for (int s = 0; s < numSamples; ++s)
            {
                revHp[c] += hpA * (rev[s] - revHp[c]);
                wetData[s] = wetData[s] * (1.0f - 0.5f * send) + (rev[s] - revHp[c]) * send;
            }
        }
    }

    // ---- publish the grain cloud for the visualiser
    {
        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            peak = juce::jmax (peak, std::abs (wetBuffer[0][(size_t) i]));
        vizWetLevel.store (juce::jmin (1.0f, peak), std::memory_order_relaxed);

        for (int i = 0; i < maxGrains; ++i)
        {
            const auto& g = grains[(size_t) i];
            const size_t b = (size_t) i * 5;

            if (! g.active)
            {
                vizData[b].store (0.0f, std::memory_order_relaxed);
                continue;
            }

            double d = (double) writePos - g.readPos;
            if (d < 0.0) d += (double) delaySize;
            const float depth = juce::jlimit (0.0f, 1.0f, (float) (d / (sampleRate * 4.0)));
            const float t = (float) g.age / (float) g.length;
            const int wi = juce::jlimit (0, windowSize, (int) (t * (float) windowSize));

            vizData[b + 0].store (1.0f, std::memory_order_relaxed);
            vizData[b + 1].store (depth, std::memory_order_relaxed);
            vizData[b + 2].store (window[(size_t) wi], std::memory_order_relaxed);
            vizData[b + 3].store (g.panNow, std::memory_order_relaxed);
            vizData[b + 4].store (t, std::memory_order_relaxed);
        }
    }

    // ---- final dry/wet mix
    for (int s = 0; s < numSamples; ++s)
    {
        const float m = smoothMix.getNextValue();
        for (int c = 0; c < chans; ++c)
            io[c][s] = io[c][s] * (1.0f - m) + wetBuffer[(size_t) c][(size_t) s] * m;
    }
}

void GranularEngine::copyVisualData (std::array<VizGrain, maxGrains>& out, float& wetLevel, float& inputLevel) const
{
    for (int i = 0; i < maxGrains; ++i)
    {
        const size_t b = (size_t) i * 5;
        auto& o = out[(size_t) i];
        o.active = vizData[b + 0].load (std::memory_order_relaxed);
        o.depth  = vizData[b + 1].load (std::memory_order_relaxed);
        o.env    = vizData[b + 2].load (std::memory_order_relaxed);
        o.pan    = vizData[b + 3].load (std::memory_order_relaxed);
        o.age    = vizData[b + 4].load (std::memory_order_relaxed);
    }
    wetLevel   = vizWetLevel.load (std::memory_order_relaxed);
    inputLevel = vizInLevel.load (std::memory_order_relaxed);
}
