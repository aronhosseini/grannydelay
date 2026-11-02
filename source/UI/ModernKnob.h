#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>
#include "../ModMatrix.h"

// Ring-style knob with drag-and-drop modulation:
//  - drop an LFO / ENV handle on it to assign the source
//  - a coloured chip appears under the knob for every assigned source: drag it (up/right = more, down/left = less,
//    negative values invert), double-click or right-click to remove
class ModernKnob  : public juce::Component,
                    public juce::DragAndDropTarget,
                    private juce::Timer
{
public:
    // matrix may be nullptr and targetIndex -1 for knobs that cannot be modulated
    ModernKnob (const juce::String& labelName, juce::Colour accentColour, ModMatrix* matrix, int targetIndex);
    ~ModernKnob() override;

    void resized() override;
    void paintOverChildren (juce::Graphics&) override;

    bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDragEnter (const SourceDetails&) override;
    void itemDragExit (const SourceDetails&) override;
    void itemDropped (const SourceDetails&) override;

    juce::Slider& getSlider() { return slider; }

    static juce::Colour sourceColour (int source);

private:
    class KnobLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        explicit KnobLookAndFeel (juce::Colour c) : accent (c) {}
        void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                               float sliderPos, float startAngle, float endAngle,
                               juce::Slider&) override;
    private:
        juce::Colour accent;
    };

    class ModChip;

    void timerCallback() override;
    int  currentMask() const;
    void rebuildChips();
    void layoutChips();
    void showAmount (int source, float amount);

    KnobLookAndFeel lookAndFeel;
    juce::Slider slider;
    juce::Label label;
    juce::String baseName;
    juce::Colour accent;

    ModMatrix* matrix;
    int target;
    int lastMask = 0;
    int labelTicks = 0;
    bool dragHover = false;
    juce::Rectangle<int> chipRow;
    std::vector<std::unique_ptr<ModChip>> chips;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModernKnob)
};
