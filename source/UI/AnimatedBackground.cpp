#include "AnimatedBackground.h"
#include <cmath>

AnimatedBackground::AnimatedBackground()
{
    setInterceptsMouseClicks (false, false);

    juce::Random r;
    for (int i = 0; i < 9; ++i)
    {
        Shape s;
        s.position = { r.nextFloat(), r.nextFloat() };
        s.velocity = { (r.nextFloat() - 0.5f) * 0.02f, (r.nextFloat() - 0.5f) * 0.02f };
        s.size = 160.0f + r.nextFloat() * 260.0f;
        s.colour = juce::Colour::fromHSV (0.5f + r.nextFloat() * 0.35f, 0.7f, 0.9f, 0.05f);
        shapes.push_back (s);
    }

    startTimerHz (20);
}

AnimatedBackground::~AnimatedBackground()
{
    stopTimer();
}

void AnimatedBackground::timerCallback()
{
    const float dt = 1.0f / 20.0f;
    time += dt;

    for (auto& s : shapes)
    {
        s.position += s.velocity * dt;
        if (s.position.x < -0.1f) s.position.x += 1.2f;
        if (s.position.x > 1.1f)  s.position.x -= 1.2f;
        if (s.position.y < -0.1f) s.position.y += 1.2f;
        if (s.position.y > 1.1f)  s.position.y -= 1.2f;
    }

    repaint();
}

void AnimatedBackground::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xFF07080F), bounds.getTopLeft(),
                                             juce::Colour (0xFF12142B), bounds.getBottomRight(), false));
    g.fillRect (bounds);

    for (auto& s : shapes)
    {
        g.setGradientFill (juce::ColourGradient (s.colour, s.position.x * bounds.getWidth(), s.position.y * bounds.getHeight(),
                                                 s.colour.withAlpha (0.0f),
                                                 s.position.x * bounds.getWidth() + s.size, s.position.y * bounds.getHeight(), true));
        g.fillEllipse (s.position.x * bounds.getWidth() - s.size, s.position.y * bounds.getHeight() - s.size, s.size * 2.0f, s.size * 2.0f);
    }

    // faint dot matrix
    g.setColour (juce::Colours::white.withAlpha (0.035f));
    for (int y = 12; y < getHeight(); y += 24)
        for (int x = 12; x < getWidth(); x += 24)
            g.fillRect ((float) x, (float) y, 1.5f, 1.5f);
}
