#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include "../GranularEngine.h"

// Software-rendered 3D view of the live grain cloud.
//   X axis = stereo position, Y axis = grain window amplitude, Z axis = how far back in time the grain reads.
// Runs from the moment the plugin opens: with no audio engine activity it simulates the cloud from the current
// settings; with sound the grains get bigger, taller and brighter.
class GranularVisualizer  : public juce::Component,
                            private juce::Timer
{
public:
    struct VizParams
    {
        float pitch = 0.0f, spread = 0.5f, reverbMix = 0.25f, motion = 0.2f, saturation = 0.0f, distance = 0.0f;
        float grainSizeMs = 120.0f, density = 25.0f, delayMs = 350.0f;
        bool  freeze = false;
    };

    explicit GranularVisualizer (const GranularEngine& engineToWatch);
    ~GranularVisualizer() override;

    void paint (juce::Graphics&) override;

    void setParameters (const VizParams& p) { params = p; }

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    struct V3 { float x = 0, y = 0, z = 0; };
    struct Projected { juce::Point<float> p; float scale = 1.0f; float zc = 1.0f; };

    static constexpr int trailLength = 12;

    struct Slot
    {
        bool  active = false;
        float depth = 0, env = 0, pan = 0, age = 0;
        float trailAlpha = 0.0f;
        int   trailCount = 0;
        std::array<V3, trailLength> trail {};
        V3    pos;
    };

    struct SynthGrain { bool on = false; float age = 0, len = 1, depth = 0, pan = 0; };

    Projected project (float x, float y, float z) const;
    juce::Colour colourFor (float depth, float alpha) const;

    const GranularEngine& engine;
    VizParams params;
    std::array<Slot, GranularEngine::maxGrains> slots;
    std::array<SynthGrain, GranularEngine::maxGrains> synth;

    juce::Random random;
    float time = 0.0f, scrollTime = 0.0f;
    float wetLevel = 0.0f, inLevel = 0.0f, energy = 0.5f;
    float synthAccum = 0.0f;
    int   silentTicks = 100;
    int   activeCount = 0;
    bool  idle = true;

    // camera
    float dragYaw = 0.0f, dragPitch = 0.0f, startYaw = 0.0f, startPitch = 0.0f;
    float yaw = 0.0f, camPitch = 0.5f;
    juce::Point<float> centre;
    float focal = 300.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularVisualizer)
};
