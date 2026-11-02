#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

class AnimatedBackground  : public juce::Component,
                            private juce::Timer
{
public:
    AnimatedBackground();
    ~AnimatedBackground() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    struct Shape
    {
        juce::Point<float> position;   // normalised 0..1
        juce::Point<float> velocity;   // normalised units per second
        float size;                    // pixels
        juce::Colour colour;
    };

    std::vector<Shape> shapes;
    float time = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimatedBackground)
};
