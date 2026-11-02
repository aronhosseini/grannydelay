#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <vector>

// Modulation routing: 4 sources (LFO1, LFO2, ENV1, ENV2) x 24 target parameters.
// Amounts are -1..1 in the target's normalised (0..1) range. Lock-free: written by the GUI, read by the audio thread.
class ModMatrix
{
public:
    static constexpr int numSources = 4;
    static constexpr int numTargets = 25;

    enum Target
    {
        tDelayTime = 0, tFeedback, tMix, tGrainSize, tGrainDensity, tPitch, tSpread, tMotion,
        tSaturation, tStretch, tPosition, tDistance, tFilterCutoff, tFilterRes, tFilterGain,
        tChordTime, tChordDecay, tChordGain, tChordMix, tReverbDecay, tReverbMix,
        tReverbHighDamp, tReverbLowDamp, tReverbPreDecay, tPan
    };

    struct Route { int source; const char* target; float amount; };

    ModMatrix();

    static const char* targetId (int target);
    static int indexOf (const juce::String& paramId);   // -1 if the parameter is not modulatable
    static const char* sourceName (int source);

    float getAmount (int source, int target) const noexcept;
    void setAmount (int source, int target, float amount) noexcept;
    void clear() noexcept;
    void applyRoutes (const std::vector<Route>& routes);

    void toXml (juce::XmlElement& parent) const;
    void fromXml (const juce::XmlElement* element);

    // written by the audio thread, read by the GUI for meters / live markers
    std::array<std::atomic<float>, numSources> sourceValue;
    std::array<std::atomic<float>, numTargets> liveNorm;

private:
    std::array<std::atomic<float>, numSources * numTargets> amounts;
};
