#include "ModernKnob.h"
#include <cmath>

juce::Colour ModernKnob::sourceColour (int s)
{
    static const juce::uint32 colours[4] = { 0xFF4DFFB8, 0xFFFFD84D, 0xFFFF6B6B, 0xFFB86BFF };
    return juce::Colour (colours[juce::jlimit (0, 3, s)]);
}

// ------------------------------------------------------------------ chip
class ModernKnob::ModChip : public juce::Component,
                            public juce::SettableTooltipClient
{
public:
    ModChip (ModernKnob& o, int s) : owner (o), source (s)
    {
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        setTooltip (juce::String (ModMatrix::sourceName (s)) + ": drag to change amount, double-click to remove");
    }

    void paint (juce::Graphics& g) override
    {
        const auto col = ModernKnob::sourceColour (source);
        const float amount = owner.matrix->getAmount (source, owner.target);
        auto b = getLocalBounds().toFloat().reduced (1.0f);
        const auto c = b.getCentre();
        const float r = b.getWidth() * 0.5f;

        g.setColour (col.withAlpha (0.18f));
        g.fillEllipse (b);

        const float sweep = amount * juce::MathConstants<float>::pi * 1.8f;
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, r - 1.5f, r - 1.5f, 0.0f, juce::jmin (0.0f, sweep), juce::jmax (0.0f, sweep) + 0.001f, true);
        g.setColour (col);
        g.strokePath (arc, juce::PathStrokeType (2.2f));

        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.setFont (juce::Font (7.5f, juce::Font::bold));
        const juce::String letter = source < 2 ? "L" + juce::String (source + 1) : "E" + juce::String (source - 1);
        g.drawText (letter, getLocalBounds(), juce::Justification::centred, false);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { owner.matrix->setAmount (source, owner.target, 0.0f); return; }
        startAmount = owner.matrix->getAmount (source, owner.target);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) return;

        float a = startAmount - (float) e.getDistanceFromDragStartY() * 0.006f + (float) e.getDistanceFromDragStartX() * 0.004f;
        a = juce::jlimit (-1.0f, 1.0f, a);
        if (std::abs (a) < 0.01f) a = (a < 0.0f ? -0.01f : 0.01f);     // never reaches 0 while dragging
        owner.matrix->setAmount (source, owner.target, a);
        owner.showAmount (source, a);
    }

    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        owner.matrix->setAmount (source, owner.target, 0.0f);
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        float a = juce::jlimit (-1.0f, 1.0f, owner.matrix->getAmount (source, owner.target) + w.deltaY * 0.2f);
        if (std::abs (a) < 0.01f) a = 0.01f;
        owner.matrix->setAmount (source, owner.target, a);
        owner.showAmount (source, a);
    }

private:
    ModernKnob& owner;
    int source;
    float startAmount = 0.0f;
};

// ------------------------------------------------------------------ look and feel
void ModernKnob::KnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                                    float sliderPos, float startAngle, float endAngle,
                                                    juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (9.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float arcR = radius - 5.0f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    juce::Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (sliderPos > 0.001f)
    {
        juce::Path value;
        value.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
        g.setColour (accent.withAlpha (0.18f));
        g.strokePath (value, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (value, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float bx = c.x + arcR * std::sin (angle);
    const float by = c.y - arcR * std::cos (angle);
    g.setColour (accent.withAlpha (0.35f));
    g.fillEllipse (bx - 8.0f, by - 8.0f, 16.0f, 16.0f);
    g.setColour (juce::Colours::white);
    g.fillEllipse (bx - 3.5f, by - 3.5f, 7.0f, 7.0f);

    const float innerR = radius * 0.70f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xFF1B1D36), c.x, c.y - innerR,
                                             juce::Colour (0xFF0B0C18), c.x, c.y + innerR, false));
    g.fillEllipse (c.x - innerR, c.y - innerR, innerR * 2.0f, innerR * 2.0f);
    g.setColour (accent.withAlpha (0.25f));
    g.drawEllipse (c.x - innerR, c.y - innerR, innerR * 2.0f, innerR * 2.0f, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.92f));
    g.setFont (juce::Font (juce::jlimit (9.0f, 13.0f, radius * 0.27f), juce::Font::bold));
    g.drawFittedText (slider.getTextFromValue (slider.getValue()),
                      juce::Rectangle<float> (c.x - innerR, c.y - 9.0f, innerR * 2.0f, 18.0f).toNearestInt(),
                      juce::Justification::centred, 1, 0.7f);
}

// ------------------------------------------------------------------ knob
ModernKnob::ModernKnob (const juce::String& labelName, juce::Colour accentColour, ModMatrix* m, int targetIndex)
    : lookAndFeel (accentColour), baseName (labelName), accent (accentColour), matrix (m), target (targetIndex)
{
    slider.setLookAndFeel (&lookAndFeel);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                juce::MathConstants<float>::pi * 2.8f, true);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setScrollWheelEnabled (true);
    addAndMakeVisible (slider);

    label.setText (labelName, juce::dontSendNotification);
    label.setFont (juce::Font (10.5f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, accentColour.withAlpha (0.95f));
    label.setJustificationType (juce::Justification::centred);
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);

    if (matrix != nullptr && target >= 0)
        startTimerHz (20);
}

ModernKnob::~ModernKnob()
{
    stopTimer();
    chips.clear();
    slider.setLookAndFeel (nullptr);
}

void ModernKnob::resized()
{
    auto area = getLocalBounds();
    chipRow = area.removeFromBottom (16);
    label.setBounds (area.removeFromBottom (16));
    slider.setBounds (area);
    layoutChips();
}

int ModernKnob::currentMask() const
{
    int mask = 0;
    for (int s = 0; s < ModMatrix::numSources; ++s)
        if (std::abs (matrix->getAmount (s, target)) > 0.0001f)
            mask |= (1 << s);
    return mask;
}

void ModernKnob::timerCallback()
{
    const int mask = currentMask();
    if (mask != lastMask)
        rebuildChips();

    if (mask != 0)
    {
        for (auto& c : chips) c->repaint();
        repaint();
    }

    if (labelTicks > 0 && --labelTicks == 0)
        label.setText (baseName, juce::dontSendNotification);
}

void ModernKnob::rebuildChips()
{
    chips.clear();
    lastMask = currentMask();

    for (int s = 0; s < ModMatrix::numSources; ++s)
    {
        if ((lastMask & (1 << s)) != 0)
        {
            auto chip = std::make_unique<ModChip> (*this, s);
            addAndMakeVisible (*chip);
            chips.push_back (std::move (chip));
        }
    }

    layoutChips();
    repaint();
}

void ModernKnob::layoutChips()
{
    const int size = 16;
    int x = chipRow.getCentreX() - (int) chips.size() * size / 2;
    for (auto& c : chips)
    {
        c->setBounds (x, chipRow.getY(), size, size);
        x += size;
    }
}

void ModernKnob::showAmount (int source, float amount)
{
    label.setText (juce::String (ModMatrix::sourceName (source)) + " " + (amount >= 0.0f ? "+" : "")
                       + juce::String (juce::roundToInt (amount * 100.0f)) + "%", juce::dontSendNotification);
    labelTicks = 30;
}

void ModernKnob::paintOverChildren (juce::Graphics& g)
{
    auto sb = slider.getBounds().toFloat().reduced (9.0f);
    const float radius = juce::jmin (sb.getWidth(), sb.getHeight()) * 0.5f;
    const auto c = sb.getCentre();
    const float arcR = radius - 5.0f;
    const float startA = juce::MathConstants<float>::pi * 1.2f;
    const float endA   = juce::MathConstants<float>::pi * 2.8f;

    if (matrix != nullptr && target >= 0)
    {
        const float n0 = (float) slider.valueToProportionOfLength (slider.getValue());
        int drawn = 0;

        for (int s = 0; s < ModMatrix::numSources; ++s)
        {
            const float a = matrix->getAmount (s, target);
            if (std::abs (a) < 0.0001f) continue;

            const float n1 = juce::jlimit (0.0f, 1.0f, n0 + a);
            const float lo = juce::jmin (n0, n1), hi = juce::jmax (n0, n1);
            const float r2 = arcR + 5.0f + 2.5f * (float) drawn++;

            juce::Path p;
            p.addCentredArc (c.x, c.y, r2, r2, 0.0f, startA + lo * (endA - startA), startA + hi * (endA - startA) + 0.001f, true);
            g.setColour (sourceColour (s).withAlpha (0.9f));
            g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        if (drawn > 0)
        {
            const float nl = matrix->liveNorm[(size_t) target].load (std::memory_order_relaxed);
            const float ang = startA + nl * (endA - startA);
            g.setColour (juce::Colours::white);
            g.fillEllipse (c.x + arcR * std::sin (ang) - 2.5f, c.y - arcR * std::cos (ang) - 2.5f, 5.0f, 5.0f);
        }
    }

    if (dragHover)
    {
        g.setColour (accent.withAlpha (0.5f));
        g.drawEllipse (c.x - radius - 4.0f, c.y - radius - 4.0f, (radius + 4.0f) * 2.0f, (radius + 4.0f) * 2.0f, 3.0f);
    }
}

bool ModernKnob::isInterestedInDragSource (const SourceDetails& details)
{
    return matrix != nullptr && target >= 0 && details.description.toString().startsWith ("mod:");
}

void ModernKnob::itemDragEnter (const SourceDetails&)
{
    dragHover = true;
    repaint();
}

void ModernKnob::itemDragExit (const SourceDetails&)
{
    dragHover = false;
    repaint();
}

void ModernKnob::itemDropped (const SourceDetails& details)
{
    dragHover = false;
    const int source = details.description.toString().fromFirstOccurrenceOf ("mod:", false, false).getIntValue();

    if (source >= 0 && source < ModMatrix::numSources && std::abs (matrix->getAmount (source, target)) < 0.001f)
        matrix->setAmount (source, target, 0.35f);   // default amount; adjust by dragging the chip

    showAmount (source, matrix->getAmount (source, target));
    repaint();
}
