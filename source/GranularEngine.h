#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <vector>

// Real-time safe granular delay engine (no allocation in process()).
// Signal flow: grains (motion / saturation / distance) -> filter -> chord resonator -> reverb -> dry/wet mix
class GranularEngine
{
public:
    static constexpr int maxGrains = 64;

    // snapshot of one grain for the UI (written by the audio thread, read by the GUI thread)
    struct VizGrain
    {
        float active = 0.0f;
        float depth  = 0.0f;   // 0 = now, 1 = oldest audio
        float env    = 0.0f;   // current window value 0..1
        float pan    = 0.0f;   // -1..1
        float age    = 0.0f;   // 0..1 through the grain
    };

    struct Params
    {
        // time
        float delayTimeMs = 350.0f, feedback = 0.35f, mix = 0.5f;
        // grains
        float grainSizeMs = 120.0f, grainDensity = 25.0f, pitchSemitones = 0.0f, spread = 0.5f;
        float motion = 0.2f, saturation = 0.0f, stretch = 1.0f, position = 0.0f, distance = 0.0f;
        float pan = 0.0f;   // -1..1: shifts the whole grain cloud left / right
        bool  freeze = false;
        // filter
        int   filterType = 0;
        float filterCutoff = 20000.0f, filterRes = 0.7f, filterGainDb = 0.0f;
        // chord resonator
        int   chordType = 0, chordRoot = 0;
        float chordTimeMs = 20.0f, chordDecay = 1.0f, chordGainDb = -6.0f, chordMix = 0.0f;
        // reverb
        int   reverbType = 2;
        float reverbDecay = 0.5f, reverbMix = 0.25f, reverbHighDamp = 0.5f, reverbLowDamp = 0.2f, reverbPreDecayMs = 10.0f;
    };

    GranularEngine();

    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const Params& params);

    // thread-safe (lock-free) copy of the current grain cloud, for visualisation
    void copyVisualData (std::array<VizGrain, maxGrains>& out, float& wetLevel, float& inputLevel) const;

private:
    static constexpr int maxChannels = 2;
    static constexpr int windowSize  = 2048;
    static constexpr int chordVoices = 4;

    struct Grain
    {
        bool   active = false;
        double readPos = 0.0;
        double speed = 1.0;
        int    age = 0;
        int    length = 1;
        float  panStart = 0.0f, panEnd = 0.0f, panNow = 0.0f;
    };

    struct Coeffs { float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f; };
    struct BiquadState { float z1 = 0.0f, z2 = 0.0f; };

    void processChunk (float* const* channelData, int numSamples, const Params& p);
    void spawnGrain (const Params& p);
    float readInterpolated (const std::vector<float>& buf, double pos) const noexcept;
    Coeffs makeCoeffs (int baseType, float freq, float q, float gainDb) const noexcept;
    static float readFrac (const std::vector<float>& buf, int size, int pos, float delay) noexcept;

    double sampleRate = 44100.0;
    int numChannels = 2;
    int maxBlock = 512;

    // grain engine
    std::array<std::vector<float>, maxChannels> delayBuffer;
    int delaySize = 0;
    int writePos = 0;
    std::array<Grain, maxGrains> grains;
    std::vector<float> window;
    double grainCountdown = 0.0;
    double stretchOffset = 0.0;
    float currentNorm = 1.0f;
    std::array<float, maxChannels> distanceLp {};

    std::array<std::vector<float>, maxChannels> wetBuffer;

    // filter
    Coeffs filterCoeffs[2];
    bool filter24 = false;
    BiquadState filterState[maxChannels][2];

    // chord resonator
    std::array<std::vector<float>, maxChannels> chordHist;
    std::array<std::array<std::vector<float>, chordVoices>, maxChannels> chordComb;
    float chordLp[maxChannels][chordVoices] {};
    int chordHistSize = 0, chordCombSize = 0, chordHistPos = 0, chordPos = 0;

    // reverb
    juce::Reverb reverb;
    std::array<std::vector<float>, maxChannels> preDelay;
    std::array<std::vector<float>, maxChannels> reverbTmp;
    int preSize = 0, prePos = 0;
    float revHp[maxChannels] {};

    juce::SmoothedValue<float> smoothMix, smoothFeedback;
    juce::Random random;

    std::array<std::atomic<float>, (size_t) maxGrains * 5> vizData;
    std::atomic<float> vizWetLevel { 0.0f };
    std::atomic<float> vizInLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularEngine)
};
