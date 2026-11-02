#include "ModSourceHandle.h"
#include "ModernKnob.h"

ModSourceHandle::ModSourceHandle (int sourceIndex, ModMatrix& m) : source (sourceIndex), matrix (m)
{
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    setTooltip ("Drag onto any knob to modulate it");
    startTimerHz (30);
}

ModSourceHandle::~ModSourceHandle()
{
    stopTimer();
}

void ModSourceHandle::paint (juce::Graphics& g)
{
    const auto col = ModernKnob::sourceColour (source);
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    const float v = matrix.sourceValue[(size_t) source].load (std::memory_order_relaxed);
    const bool bipolar = source < 2;

    g.setGradientFill (juce::ColourGradient (col.withAlpha (0.28f), b.getX(), b.getY(),
                                             col.withAlpha (0.08f), b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 14.0f);
    g.setColour (col.withAlpha (0.8f));
    g.drawRoundedRectangle (b, 14.0f, 1.5f);

    g.setColour (juce::Colours::white.withAlpha (0.95f));
    g.setFont (juce::Font (13.0f, juce::Font::bold));
    g.drawText (ModMatrix::sourceName (source), b.removeFromTop (28.0f), juce::Justification::centred, false);

    // live meter
    auto meter = b.reduced (b.getWidth() * 0.32f, 6.0f);
    meter.removeFromBottom (22.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (meter, 4.0f);

    g.setColour (col);
    if (bipolar)
    {
        const float mid = meter.getCentreY();
        const float h = juce::jlimit (-1.0f, 1.0f, v) * meter.getHeight() * 0.5f;
        g.fillRect (meter.getX(), h >= 0.0f ? mid - h : mid, meter.getWidth(), std::abs (h));
        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.drawLine (meter.getX(), mid, meter.getRight(), mid, 1.0f);
    }
    else
    {
        const float h = juce::jlimit (0.0f, 1.0f, v) * meter.getHeight();
        g.fillRect (meter.getX(), meter.getBottom() - h, meter.getWidth(), h);
    }

    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.setFont (juce::Font (9.0f, juce::Font::bold));
    g.drawText ("DRAG TO KNOB", getLocalBounds().toFloat().removeFromBottom (22.0f), juce::Justification::centred, false);
}

void ModSourceHandle::mouseDrag (const juce::MouseEvent& e)
{
    if (e.getDistanceFromDragStart() < 5)
        return;

    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this))
        if (! container->isDragAndDropActive())
            container->startDragging ("mod:" + juce::String (source), this);
}
