#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include "ModMatrix.h"

// 30 factory presets + user presets (XML files). A preset = every parameter + the modulation routing.
class PresetManager
{
public:
    struct Override { const char* id; float value; };

    struct Preset
    {
        const char* name;
        const char* category;
        std::vector<Override> values;              // everything not listed is reset to its default
        std::vector<ModMatrix::Route> routes;      // LFO / envelope assignments
    };

    PresetManager (juce::AudioProcessorValueTreeState& state, ModMatrix& matrix);

    static const std::vector<Preset>& getFactoryPresets();

    bool loadFactory (int index);
    bool loadUser (const juce::String& name);
    bool saveUser (const juce::String& name);
    bool deleteUser (const juce::String& name);

    bool isUserPreset (const juce::String& name) const;
    juce::StringArray getUserPresetNames() const;
    juce::String getCurrentName() const;
    juce::File getUserDirectory() const;

private:
    juce::File fileForName (const juce::String& name) const;
    void resetAllToDefault();
    void setParam (const juce::String& id, float realValue);
    void setCurrentName (const juce::String& name);

    juce::AudioProcessorValueTreeState& apvts;
    ModMatrix& matrix;
};
