#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    using Range = juce::NormalisableRange<float>;
    using ToText = std::function<juce::String (float, int)>;
    using FromText = std::function<float (const juce::String&)>;

    // LFO sync lengths in beats (4/4): 16 bar ... 1/64
    const float kLfoBeats[17] = { 64.0f, 32.0f, 16.0f, 8.0f, 4.0f, 2.0f, 1.5f, 1.0f, 2.0f / 3.0f,
                                  0.75f, 0.5f, 1.0f / 3.0f, 0.375f, 0.25f, 1.0f / 6.0f, 0.125f, 0.0625f };

    const float kSyncBeats[11] = { 0.125f, 1.0f / 6.0f, 0.25f, 1.0f / 3.0f, 0.5f, 2.0f / 3.0f, 0.75f, 1.0f, 1.5f, 2.0f, 4.0f };

    std::unique_ptr<juce::AudioParameterFloat> makeFloat (const char* id, const juce::String& name,
                                                          Range range, float def, ToText toText, FromText fromText)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, def,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction (std::move (toText))
                .withValueFromStringFunction (std::move (fromText)));
    }

    std::unique_ptr<juce::AudioParameterChoice> makeChoice (const char* id, const juce::String& name,
                                                            const juce::StringArray& items, int def)
    {
        return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, def);
    }

    std::unique_ptr<juce::AudioParameterBool> makeBool (const char* id, const juce::String& name, bool def)
    {
        return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def);
    }

    ToText unitText (const juce::String& suffix, int decimals)
    {
        return [suffix, decimals] (float v, int) { return juce::String (v, decimals) + suffix; };
    }

    ToText percentText()
    {
        return [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; };
    }

    ToText hzText()
    {
        return [] (float v, int) { return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz"; };
    }

    FromText plainParse()   { return [] (const juce::String& t) { return t.getFloatValue(); }; }
    FromText percentParse() { return [] (const juce::String& t) { return t.getFloatValue() / 100.0f; }; }
    FromText hzParse()      { return [] (const juce::String& t) { return t.containsIgnoreCase ("k") ? t.getFloatValue() * 1000.0f : t.getFloatValue(); }; }

    Range skewedAround (float lo, float hi, float interval, float centre)
    {
        Range r (lo, hi, interval);
        r.setSkewForCentre (centre);
        return r;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout GrannyDelayAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ---- TIME
    layout.add (makeFloat ("delayTime", "Delay Time", Range (0.0f, 2000.0f, 0.1f), 350.0f, unitText (" ms", 1), plainParse()));
    layout.add (makeBool  ("sync", "Sync", false));
    layout.add (makeChoice ("syncDiv", "Sync Division",
                            { "1/32", "1/16T", "1/16", "1/8T", "1/8", "1/4T", "1/8.", "1/4", "1/4.", "1/2", "1/1" }, 7));
    layout.add (makeFloat ("feedback", "Feedback", Range (0.0f, 0.95f, 0.01f), 0.35f, percentText(), percentParse()));
    layout.add (makeFloat ("mix",      "Mix",      Range (0.0f, 1.0f, 0.01f),  0.5f,  percentText(), percentParse()));

    // ---- GRAINS
    layout.add (makeFloat ("grainSize",    "Grain Size",    Range (5.0f, 500.0f, 0.1f, 0.4f), 120.0f, unitText (" ms", 1), plainParse()));
    layout.add (makeFloat ("grainDensity", "Grain Density", Range (1.0f, 100.0f, 0.1f, 0.5f), 25.0f,  unitText (" /s", 1),  plainParse()));
    layout.add (makeFloat ("pitch", "Pitch", Range (-12.0f, 12.0f, 0.1f), 0.0f,
                           [] (float v, int) { return juce::String (v > 0.0f ? "+" : "") + juce::String (v, 1) + " st"; }, plainParse()));
    layout.add (makeFloat ("spread",     "Spread",     Range (0.0f, 1.0f, 0.01f), 0.5f, percentText(), percentParse()));
    layout.add (makeFloat ("motion",     "Motion",     Range (0.0f, 1.0f, 0.01f), 0.2f, percentText(), percentParse()));
    layout.add (makeFloat ("saturation", "Saturation", Range (0.0f, 1.0f, 0.01f), 0.0f, percentText(), percentParse()));
    layout.add (makeBool  ("freeze", "Freeze", false));
    layout.add (makeFloat ("stretch",  "Stretch",  skewedAround (0.25f, 4.0f, 0.01f, 1.0f), 1.0f, unitText (" x", 2), plainParse()));
    layout.add (makeFloat ("position", "Position", Range (0.0f, 1.0f, 0.01f), 0.0f, percentText(), percentParse()));
    layout.add (makeFloat ("distance", "Distance", Range (0.0f, 1.0f, 0.01f), 0.0f, percentText(), percentParse()));
    layout.add (makeFloat ("pan", "Pan", Range (-1.0f, 1.0f, 0.01f), 0.0f,
                           [] (float v, int)
                           {
                               const int pc = juce::roundToInt (std::abs (v) * 100.0f);
                               return pc == 0 ? juce::String ("C") : juce::String (v < 0.0f ? "L " : "R ") + juce::String (pc);
                           }, plainParse()));

    // ---- FILTER
    layout.add (makeChoice ("filterType", "Filter Type",
                            { "LP 12", "LP 24", "HP 12", "HP 24", "Band", "Notch", "Peak", "Low Shelf", "High Shelf", "All-pass" }, 0));
    layout.add (makeFloat ("filterCutoff", "Cutoff",    skewedAround (20.0f, 20000.0f, 1.0f, 1000.0f), 20000.0f, hzText(), hzParse()));
    layout.add (makeFloat ("filterRes",    "Resonance", skewedAround (0.3f, 12.0f, 0.01f, 1.5f), 0.7f, unitText (" Q", 2), plainParse()));
    layout.add (makeFloat ("filterGain",   "Gain",      Range (-18.0f, 18.0f, 0.1f), 0.0f, unitText (" dB", 1), plainParse()));

    // ---- CHORD RESONATOR
    layout.add (makeChoice ("chordType", "Chord",
                            { "Major", "Minor", "Maj 7", "Min 7", "Dom 7", "Sus 2", "Sus 4", "Dim", "Aug", "Maj 9" }, 0));
    layout.add (makeChoice ("chordRoot", "Root",
                            { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
    layout.add (makeFloat ("chordTime",  "Time",  Range (0.0f, 200.0f, 0.1f), 20.0f, unitText (" ms", 0), plainParse()));
    layout.add (makeFloat ("chordDecay", "Decay", Range (0.05f, 5.0f, 0.01f, 0.5f), 1.0f, unitText (" s", 2), plainParse()));
    layout.add (makeFloat ("chordGain",  "Gain",  Range (-24.0f, 6.0f, 0.1f), -6.0f, unitText (" dB", 1), plainParse()));
    layout.add (makeFloat ("chordMix",   "Mix",   Range (0.0f, 1.0f, 0.01f), 0.0f, percentText(), percentParse()));

    // ---- REVERB
    layout.add (makeChoice ("reverbType", "Reverb Type", { "Room", "Plate", "Hall", "Cathedral", "Space" }, 2));
    layout.add (makeFloat ("reverbDecay",    "Decay",     Range (0.0f, 1.0f, 0.01f), 0.5f, percentText(), percentParse()));
    layout.add (makeFloat ("space",          "Mix",       Range (0.0f, 1.0f, 0.01f), 0.25f, percentText(), percentParse()));
    layout.add (makeFloat ("reverbHighDamp", "High Damp", Range (0.0f, 1.0f, 0.01f), 0.5f, percentText(), percentParse()));
    layout.add (makeFloat ("reverbLowDamp",  "Low Damp",  Range (0.0f, 1.0f, 0.01f), 0.2f, percentText(), percentParse()));
    layout.add (makeFloat ("reverbPreDecay", "Pre-Decay", Range (0.0f, 200.0f, 0.1f), 10.0f, unitText (" ms", 0), plainParse()));

    // ---- MODULATION: 2 LFOs, 2 envelope followers
    const juce::StringArray shapes { "Sine", "Tri", "Saw", "Square", "S&H" };
    const juce::StringArray lfoDivs { "16 bar", "8 bar", "4 bar", "2 bar", "1 bar", "1/2", "1/4.", "1/4", "1/4T",
                                      "1/8.", "1/8", "1/8T", "1/16.", "1/16", "1/16T", "1/32", "1/64" };
    layout.add (makeFloat ("lfo1Rate", "LFO 1 Rate", Range (0.01f, 20.0f, 0.01f, 0.5f), 1.0f, unitText (" Hz", 2), plainParse()));
    layout.add (makeChoice ("lfo1Shape", "LFO 1 Shape", shapes, 0));
    layout.add (makeFloat ("lfo1Depth", "LFO 1 Depth", Range (0.0f, 1.0f, 0.01f), 1.0f, percentText(), percentParse()));
    layout.add (makeBool  ("lfo1Sync", "LFO 1 Sync", false));
    layout.add (makeChoice ("lfo1Div", "LFO 1 Division", lfoDivs, 7));
    layout.add (makeFloat ("lfo2Rate", "LFO 2 Rate", Range (0.01f, 20.0f, 0.01f, 0.5f), 0.3f, unitText (" Hz", 2), plainParse()));
    layout.add (makeChoice ("lfo2Shape", "LFO 2 Shape", shapes, 1));
    layout.add (makeFloat ("lfo2Depth", "LFO 2 Depth", Range (0.0f, 1.0f, 0.01f), 1.0f, percentText(), percentParse()));
    layout.add (makeBool  ("lfo2Sync", "LFO 2 Sync", false));
    layout.add (makeChoice ("lfo2Div", "LFO 2 Division", lfoDivs, 7));

    layout.add (makeFloat ("env1Attack",  "Env 1 Attack",  Range (0.1f, 500.0f, 0.1f, 0.4f), 10.0f, unitText (" ms", 1), plainParse()));
    layout.add (makeFloat ("env1Release", "Env 1 Release", Range (5.0f, 3000.0f, 1.0f, 0.4f), 200.0f, unitText (" ms", 0), plainParse()));
    layout.add (makeFloat ("env1Gain",    "Env 1 Gain",    skewedAround (0.5f, 20.0f, 0.01f, 4.0f), 4.0f, unitText (" x", 1), plainParse()));
    layout.add (makeFloat ("env2Attack",  "Env 2 Attack",  Range (0.1f, 500.0f, 0.1f, 0.4f), 50.0f, unitText (" ms", 1), plainParse()));
    layout.add (makeFloat ("env2Release", "Env 2 Release", Range (5.0f, 3000.0f, 1.0f, 0.4f), 800.0f, unitText (" ms", 0), plainParse()));
    layout.add (makeFloat ("env2Gain",    "Env 2 Gain",    skewedAround (0.5f, 20.0f, 0.01f, 4.0f), 4.0f, unitText (" x", 1), plainParse()));

    return layout;
}

GrannyDelayAudioProcessor::GrannyDelayAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                              .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                              .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "GrannyDelayState", createParameterLayout())
{
    apvts.state.setProperty ("presetName", "Init", nullptr);

    for (int t = 0; t < ModMatrix::numTargets; ++t)
    {
        targetRaw[(size_t) t]   = raw (ModMatrix::targetId (t));
        targetRange[(size_t) t] = apvts.getParameterRange (ModMatrix::targetId (t));
    }

    pSync = raw ("sync");  pSyncDiv = raw ("syncDiv");  pFreeze = raw ("freeze");
    pFilterType = raw ("filterType");  pChordType = raw ("chordType");  pChordRoot = raw ("chordRoot");
    pReverbType = raw ("reverbType");

    pLfoSync[0] = raw ("lfo1Sync");  pLfoDiv[0] = raw ("lfo1Div");
    pLfoSync[1] = raw ("lfo2Sync");  pLfoDiv[1] = raw ("lfo2Div");
    pLfoRate[0] = raw ("lfo1Rate");  pLfoShape[0] = raw ("lfo1Shape");  pLfoDepth[0] = raw ("lfo1Depth");
    pLfoRate[1] = raw ("lfo2Rate");  pLfoShape[1] = raw ("lfo2Shape");  pLfoDepth[1] = raw ("lfo2Depth");
    pEnvAtt[0] = raw ("env1Attack");  pEnvRel[0] = raw ("env1Release");  pEnvGain[0] = raw ("env1Gain");
    pEnvAtt[1] = raw ("env2Attack");  pEnvRel[1] = raw ("env2Release");  pEnvGain[1] = raw ("env2Gain");
}

void GrannyDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    granularEngine.prepare (sampleRate, samplesPerBlock, juce::jmax (1, getTotalNumOutputChannels()));
    envState[0] = envState[1] = 0.0f;
}

void GrannyDelayAudioProcessor::releaseResources() {}

bool GrannyDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

void GrannyDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, numSamples);

    const float sr = (float) juce::jmax (1.0, getSampleRate());
    const int inCh = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels());

    // ---- envelope followers (follow the input level)
    float attCoef[2], relCoef[2];
    for (int e = 0; e < 2; ++e)
    {
        attCoef[e] = 1.0f - std::exp (-1.0f / (sr * 0.001f * pEnvAtt[e]->load()));
        relCoef[e] = 1.0f - std::exp (-1.0f / (sr * 0.001f * pEnvRel[e]->load()));
    }

    const float* in[2] = { inCh > 0 ? buffer.getReadPointer (0) : nullptr, inCh > 1 ? buffer.getReadPointer (1) : nullptr };
    for (int s = 0; s < numSamples; ++s)
    {
        float x = 0.0f;
        for (int c = 0; c < inCh; ++c)
            x = juce::jmax (x, std::abs (in[c][s]));

        for (int e = 0; e < 2; ++e)
            envState[e] += (x > envState[e] ? attCoef[e] : relCoef[e]) * (x - envState[e]);
    }

    // ---- host tempo (used by the LFO sync and the delay sync)
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                bpm = *b;
    bpm = juce::jlimit (20.0, 300.0, bpm);

    // ---- LFOs (evaluated once per block)
    float src[ModMatrix::numSources];
    for (int l = 0; l < 2; ++l)
    {
        float rate = pLfoRate[l]->load();
        if (pLfoSync[l]->load() > 0.5f)
            rate = (float) (bpm / 60.0) / kLfoBeats[juce::jlimit (0, 16, (int) pLfoDiv[l]->load())];
        const int shape = (int) pLfoShape[l]->load();
        const float ph = lfoPhase[l];
        float v = 0.0f;

        switch (shape)
        {
            case 0:  v = std::sin (juce::MathConstants<float>::twoPi * ph); break;
            case 1:  v = 1.0f - 4.0f * std::abs (ph - 0.5f); break;
            case 2:  v = 2.0f * ph - 1.0f; break;
            case 3:  v = ph < 0.5f ? 1.0f : -1.0f; break;
            default: v = lfoHeld[l]; break;
        }

        src[l] = v * pLfoDepth[l]->load();

        float next = ph + rate * (float) numSamples / sr;
        if (next >= 1.0f)
        {
            next -= std::floor (next);
            lfoHeld[l] = shRandom.nextFloat() * 2.0f - 1.0f;
        }
        lfoPhase[l] = next;
    }

    for (int e = 0; e < 2; ++e)
        src[2 + e] = juce::jlimit (0.0f, 1.0f, envState[e] * pEnvGain[e]->load());

    for (int s = 0; s < ModMatrix::numSources; ++s)
        modMatrix.sourceValue[(size_t) s].store (src[s], std::memory_order_relaxed);

    // ---- apply modulation to every target (in each parameter's normalised range)
    float value[ModMatrix::numTargets];
    float baseNorm[ModMatrix::numTargets];
    float modNorm[ModMatrix::numTargets];

    for (int t = 0; t < ModMatrix::numTargets; ++t)
    {
        const auto& range = targetRange[(size_t) t];
        const float base = targetRaw[(size_t) t]->load();
        const float n0 = range.convertTo0to1 (base);

        float m = 0.0f;
        for (int s = 0; s < ModMatrix::numSources; ++s)
            m += src[s] * modMatrix.getAmount (s, t);

        const float n = juce::jlimit (0.0f, 1.0f, n0 + m);
        baseNorm[t] = n0;
        modNorm[t] = n;
        value[t] = range.convertFrom0to1 (n);
        modMatrix.liveNorm[(size_t) t].store (n, std::memory_order_relaxed);
    }

    // ---- tempo sync
    float delayMs = value[ModMatrix::tDelayTime];
    if (pSync->load() > 0.5f)
    {
        const int div = juce::jlimit (0, 10, (int) pSyncDiv->load());
        const float synced = (float) ((double) kSyncBeats[div] * 60000.0 / bpm);
        delayMs = synced * juce::jmax (0.0f, 1.0f + (modNorm[ModMatrix::tDelayTime] - baseNorm[ModMatrix::tDelayTime]));
    }

    GranularEngine::Params p;
    p.delayTimeMs    = juce::jlimit (0.0f, 6000.0f, delayMs);
    p.feedback       = value[ModMatrix::tFeedback];
    p.mix            = value[ModMatrix::tMix];
    p.grainSizeMs    = value[ModMatrix::tGrainSize];
    p.grainDensity   = value[ModMatrix::tGrainDensity];
    p.pitchSemitones = value[ModMatrix::tPitch];
    p.spread         = value[ModMatrix::tSpread];
    p.motion         = value[ModMatrix::tMotion];
    p.saturation     = value[ModMatrix::tSaturation];
    p.stretch        = value[ModMatrix::tStretch];
    p.position       = value[ModMatrix::tPosition];
    p.distance       = value[ModMatrix::tDistance];
    p.pan            = value[ModMatrix::tPan];
    p.freeze         = pFreeze->load() > 0.5f;

    p.filterType     = (int) pFilterType->load();
    p.filterCutoff   = value[ModMatrix::tFilterCutoff];
    p.filterRes      = value[ModMatrix::tFilterRes];
    p.filterGainDb   = value[ModMatrix::tFilterGain];

    p.chordType      = (int) pChordType->load();
    p.chordRoot      = (int) pChordRoot->load();
    p.chordTimeMs    = value[ModMatrix::tChordTime];
    p.chordDecay     = value[ModMatrix::tChordDecay];
    p.chordGainDb    = value[ModMatrix::tChordGain];
    p.chordMix       = value[ModMatrix::tChordMix];

    p.reverbType        = (int) pReverbType->load();
    p.reverbDecay       = value[ModMatrix::tReverbDecay];
    p.reverbMix         = value[ModMatrix::tReverbMix];
    p.reverbHighDamp    = value[ModMatrix::tReverbHighDamp];
    p.reverbLowDamp     = value[ModMatrix::tReverbLowDamp];
    p.reverbPreDecayMs  = value[ModMatrix::tReverbPreDecay];

    granularEngine.process (buffer, p);
}

juce::AudioProcessorEditor* GrannyDelayAudioProcessor::createEditor()
{
    return new GrannyDelayAudioProcessorEditor (*this);
}

void GrannyDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
    {
        xml->deleteAllChildElementsWithTagName ("ModMatrix");
        modMatrix.toXml (*xml);
        copyXmlToBinary (*xml, destData);
    }
}

void GrannyDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            modMatrix.fromXml (xml->getChildByName ("ModMatrix"));
            xml->deleteAllChildElementsWithTagName ("ModMatrix");
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GrannyDelayAudioProcessor();
}
