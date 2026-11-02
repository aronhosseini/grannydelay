#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "GranularEngine.h"
#include "ModMatrix.h"
#include "PresetManager.h"

class GrannyDelayAudioProcessor  : public juce::AudioProcessor
{
public:
    GrannyDelayAudioProcessor();
    ~GrannyDelayAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    ModMatrix modMatrix;
    PresetManager presets { apvts, modMatrix };

    const GranularEngine& getEngine() const { return granularEngine; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::atomic<float>* raw (const char* id) const { return apvts.getRawParameterValue (id); }

    GranularEngine granularEngine;

    // modulation targets
    std::array<std::atomic<float>*, ModMatrix::numTargets> targetRaw {};
    std::array<juce::NormalisableRange<float>, ModMatrix::numTargets> targetRange;

    // non-modulated parameters
    std::atomic<float> *pSync = nullptr, *pSyncDiv = nullptr, *pFreeze = nullptr;
    std::atomic<float> *pFilterType = nullptr, *pChordType = nullptr, *pChordRoot = nullptr, *pReverbType = nullptr;
    std::atomic<float> *pLfoRate[2] {}, *pLfoShape[2] {}, *pLfoDepth[2] {}, *pLfoSync[2] {}, *pLfoDiv[2] {};
    std::atomic<float> *pEnvAtt[2] {}, *pEnvRel[2] {}, *pEnvGain[2] {};

    // modulation sources state
    float lfoPhase[2] { 0.0f, 0.0f };
    float lfoHeld[2] { 0.0f, 0.0f };
    float envState[2] { 0.0f, 0.0f };
    juce::Random shRandom;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrannyDelayAudioProcessor)
};
