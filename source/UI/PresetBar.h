#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../PresetManager.h"

class PresetBar  : public juce::Component
{
public:
    explicit PresetBar (PresetManager& manager);

    void resized() override;
    void paint (juce::Graphics&) override;

    void refresh();

private:
    void rebuild();
    void selectCurrent();
    void updateButtons();
    void step (int direction);
    void promptSave();
    void promptDelete();

    PresetManager& presets;

    juce::ComboBox combo;
    juce::TextButton prevButton { "<" }, nextButton { ">" }, saveButton { "SAVE" }, deleteButton { "DELETE" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBar)
};
