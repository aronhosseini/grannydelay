#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../ModMatrix.h"

// Draggable LFO / envelope source: drag it onto any knob to modulate that parameter.
class ModSourceHandle  : public juce::Component,
                         public juce::SettableTooltipClient,
                         private juce::Timer
{
public:
    ModSourceHandle (int sourceIndex, ModMatrix& matrixToUse);
    ~ModSourceHandle() override;

    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    void timerCallback() override { repaint(); }

    int source;
    ModMatrix& matrix;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModSourceHandle)
};
