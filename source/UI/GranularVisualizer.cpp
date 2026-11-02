#include "GranularVisualizer.h"
#include <algorithm>
#include <cmath>

GranularVisualizer::GranularVisualizer (const GranularEngine& engineToWatch)
    : engine (engineToWatch)
{
    startTimerHz (40);
}

GranularVisualizer::~GranularVisualizer()
{
    stopTimer();
}

void GranularVisualizer::mouseDown (const juce::MouseEvent&)
{
    startYaw = dragYaw;
    startPitch = dragPitch;
}

void GranularVisualizer::mouseDrag (const juce::MouseEvent& e)
{
    dragYaw   = startYaw   + (float) e.getDistanceFromDragStartX() * 0.008f;
    dragPitch = juce::jlimit (-0.4f, 0.55f, startPitch + (float) e.getDistanceFromDragStartY() * 0.004f);
}

void GranularVisualizer::mouseDoubleClick (const juce::MouseEvent&)
{
    dragYaw = 0.0f;
    dragPitch = 0.0f;
}

juce::Colour GranularVisualizer::colourFor (float depth, float alpha) const
{
    float h = 0.50f + depth * 0.36f + params.pitch / 12.0f * 0.06f;   // cyan -> violet -> magenta, nudged by pitch
    float sat = 0.72f;
    if (params.freeze) { h = 0.53f + depth * 0.05f; sat = 0.35f; }    // icy when frozen
    h -= std::floor (h);
    return juce::Colour::fromHSV (h, sat, 1.0f, juce::jlimit (0.0f, 1.0f, alpha));
}

GranularVisualizer::Projected GranularVisualizer::project (float x, float y, float z) const
{
    const float cy = std::cos (yaw),      sy = std::sin (yaw);
    const float cp = std::cos (camPitch), sp = std::sin (camPitch);

    const float x1 = x * cy - z * sy;
    const float z1 = x * sy + z * cy;
    const float y1 = y - 0.35f;

    const float y2 = y1 * cp + z1 * sp;
    const float z2 = -y1 * sp + z1 * cp;

    const float zc = juce::jmax (0.6f, z2 + 3.8f);
    const float scale = focal / zc;

    Projected r;
    r.p = { centre.x + x1 * scale, centre.y - y2 * scale };
    r.scale = scale;
    r.zc = zc;
    return r;
}

void GranularVisualizer::timerCallback()
{
    const float dt = 1.0f / 40.0f;
    time += dt;
    if (! params.freeze) scrollTime += dt;

    std::array<GranularEngine::VizGrain, GranularEngine::maxGrains> data;
    float wet = 0.0f, in = 0.0f;
    engine.copyVisualData (data, wet, in);

    int engineActive = 0;
    for (auto& d : data) if (d.active > 0.5f) ++engineActive;
    silentTicks = (engineActive == 0) ? juce::jmin (silentTicks + 1, 1000) : 0;
    idle = silentTicks > 10;   // engine not running (or not yet started): animate from the settings instead

    if (idle)
    {
        // simulate a grain cloud from the current settings so the view is alive from the first frame
        const float rate = juce::jlimit (3.0f, 30.0f, params.density * 0.5f);
        synthAccum += dt * rate;
        const float lenSec = juce::jlimit (0.12f, 0.7f, params.grainSizeMs * 0.001f * 2.0f);

        while (synthAccum >= 1.0f)
        {
            synthAccum -= 1.0f;
            for (auto& sg : synth)
            {
                if (sg.on) continue;
                sg.on = true;
                sg.age = 0.0f;
                sg.len = lenSec * (0.7f + random.nextFloat() * 0.6f);
                sg.depth = juce::jlimit (0.03f, 0.95f, params.delayMs / 2500.0f + (random.nextFloat() - 0.3f) * 0.25f);
                sg.pan = (random.nextFloat() * 2.0f - 1.0f) * juce::jmax (0.15f, params.spread);
                break;
            }
        }

        for (size_t i = 0; i < synth.size(); ++i)
        {
            auto& sg = synth[i];
            auto& d = data[i];
            d.active = 0.0f;
            if (! sg.on) continue;

            sg.age += dt / sg.len;
            if (sg.age >= 1.0f) { sg.on = false; continue; }

            d.active = 1.0f;
            d.depth = sg.depth + 0.04f * std::sin (time * 1.3f + (float) i);
            d.env = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * sg.age));
            d.pan = sg.pan;
            d.age = sg.age;
        }

        wet = 0.12f + 0.08f * std::sin (time * 1.1f);
        in = 0.0f;
    }
    else
    {
        for (auto& sg : synth) sg.on = false;
    }

    wetLevel += (juce::jmin (1.0f, wet * 1.5f) - wetLevel) * 0.25f;
    inLevel  += (juce::jmin (1.0f, in * 1.5f)  - inLevel)  * (in > inLevel ? 0.5f : 0.12f);

    // more sound = bigger, taller, brighter grains
    const float targetEnergy = idle ? 0.45f : (0.55f + 1.0f * juce::jmin (1.0f, juce::jmax (inLevel, wetLevel * 0.8f) * 2.5f));
    energy += (targetEnergy - energy) * 0.15f;

    yaw = dragYaw + 0.28f * std::sin (time * 0.17f);
    camPitch = 0.5f + dragPitch;

    activeCount = 0;
    for (size_t i = 0; i < slots.size(); ++i)
    {
        auto& s = slots[i];
        const auto& d = data[i];
        const bool nowActive = d.active > 0.5f;

        if (nowActive)
        {
            if (! s.active)
                s.trailCount = 0;

            ++activeCount;
            s.depth = d.depth;
            s.env = d.env;
            s.pan = d.pan;
            s.age = d.age;

            const float wobble = params.motion * 0.2f * std::sin (time * 3.0f + (float) i);
            s.pos.x = d.pan * 3.0f + wobble;
            s.pos.y = 0.05f + d.env * (0.55f + 0.5f * juce::jmin (1.0f, energy));
            s.pos.z = (d.depth - 0.5f) * 3.0f;

            for (int k = trailLength - 1; k > 0; --k)
                s.trail[(size_t) k] = s.trail[(size_t) (k - 1)];
            s.trail[0] = s.pos;
            s.trailCount = juce::jmin (trailLength, s.trailCount + 1);
            s.trailAlpha = 1.0f;
        }
        else
        {
            s.trailAlpha *= 0.86f;
        }

        s.active = nowActive;
    }

    repaint();
}

void GranularVisualizer::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    juce::Path clip;
    clip.addRoundedRectangle (b, 20.0f);
    g.saveState();
    g.reduceClipRegion (clip);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xFF05060D), b.getCentreX(), b.getY(),
                                             juce::Colour (0xFF10122A), b.getCentreX(), b.getBottom(), false));
    g.fillRect (b);

    centre = { b.getCentreX(), b.getY() + b.getHeight() * 0.60f };
    focal = b.getHeight() * 1.25f;

    // horizon glow reacts to the signal
    {
        const float r = b.getWidth() * 0.45f;
        g.setGradientFill (juce::ColourGradient (colourFor (0.5f, 0.08f + 0.22f * energy + 0.25f * wetLevel),
                                                 centre.x, centre.y - b.getHeight() * 0.15f,
                                                 juce::Colour (0x00000000),
                                                 centre.x + r, centre.y - b.getHeight() * 0.15f, true));
        g.fillRect (b);
    }

    // floor grid (scrolls into the past, stops when frozen)
    {
        const float stepZ = 0.375f;
        const float offset = std::fmod (scrollTime * 0.12f, stepZ);
        const juce::Colour gridCol = colourFor (0.15f, 1.0f);

        for (int i = 0; i <= 16; ++i)
        {
            const float x = -3.2f + (float) i * 0.4f;
            for (int seg = 0; seg < 8; ++seg)
            {
                const float z0 = -1.5f + (float) seg * (3.0f / 8.0f);
                const float z1 = z0 + 3.0f / 8.0f;
                const float fade = 1.0f - (float) seg / 8.0f;
                g.setColour (gridCol.withAlpha (0.05f + 0.20f * fade));
                g.drawLine (juce::Line<float> (project (x, 0.0f, z0).p, project (x, 0.0f, z1).p), 1.0f);
            }
        }

        for (int k = 0; k < 9; ++k)
        {
            const float z = -1.5f + (float) k * stepZ + offset;
            if (z > 1.5f) continue;
            const float fade = 1.0f - (z + 1.5f) / 3.0f;
            g.setColour (gridCol.withAlpha (0.05f + 0.22f * fade));
            g.drawLine (juce::Line<float> (project (-3.2f, 0.0f, z).p, project (3.2f, 0.0f, z).p), 1.0f);
        }

        g.setColour (colourFor (0.0f, 0.30f + 0.5f * juce::jmax (wetLevel, inLevel)));
        g.drawLine (juce::Line<float> (project (-3.2f, 0.0f, -1.5f).p, project (3.2f, 0.0f, -1.5f).p), 2.0f);

        g.setFont (juce::Font (10.0f, juce::Font::bold));
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        const auto nowP = project (-3.2f, 0.0f, -1.5f).p;
        const auto pastP = project (-3.2f, 0.0f, 1.5f).p;
        g.drawText ("NOW", juce::Rectangle<float> (nowP.x, nowP.y + 2.0f, 60.0f, 14.0f), juce::Justification::centredLeft, false);
        g.drawText ("PAST", juce::Rectangle<float> (pastP.x, pastP.y - 16.0f, 60.0f, 14.0f), juce::Justification::centredLeft, false);
    }

    // trails
    for (auto& s : slots)
    {
        if (s.trailCount < 2 || s.trailAlpha < 0.03f)
            continue;

        juce::Path path;
        for (int k = 0; k < s.trailCount; ++k)
        {
            const auto p = project (s.trail[(size_t) k].x, s.trail[(size_t) k].y, s.trail[(size_t) k].z).p;
            if (k == 0) path.startNewSubPath (p); else path.lineTo (p);
        }
        g.setColour (colourFor (s.depth, 0.35f * s.trailAlpha * juce::jmin (1.0f, energy)));
        g.strokePath (path, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // orbs, far to near
    struct Orb { const Slot* s; Projected top; Projected foot; };
    std::array<Orb, GranularEngine::maxGrains> orbs;
    int numOrbs = 0;

    for (auto& s : slots)
    {
        if (! s.active) continue;
        orbs[(size_t) numOrbs++] = { &s, project (s.pos.x, s.pos.y, s.pos.z), project (s.pos.x, 0.0f, s.pos.z) };
    }

    std::sort (orbs.begin(), orbs.begin() + numOrbs, [] (const Orb& a, const Orb& c) { return a.top.zc > c.top.zc; });

    const float glowBoost = 1.0f + 0.8f * params.saturation;
    for (int i = 0; i < numOrbs; ++i)
    {
        const auto& o = orbs[(size_t) i];
        const auto& s = *o.s;
        const float fog = 1.0f - 0.55f * params.distance * s.depth;
        const float r = o.top.scale * (0.035f + 0.085f * s.env) * (0.7f + 0.6f * energy);
        const float glowR = r * (2.4f + 1.6f * params.reverbMix) * glowBoost;

        g.setColour (colourFor (s.depth, (0.10f + 0.30f * s.env) * fog));
        g.drawLine (juce::Line<float> (o.foot.p, o.top.p), 1.0f);
        g.setColour (colourFor (s.depth, 0.18f * s.env * fog));
        g.fillEllipse (o.foot.p.x - r, o.foot.p.y - r * 0.3f, r * 2.0f, r * 0.6f);

        g.setGradientFill (juce::ColourGradient (colourFor (s.depth, juce::jmin (1.0f, 0.55f * (0.3f + s.env) * energy * fog)), o.top.p.x, o.top.p.y,
                                                 colourFor (s.depth, 0.0f), o.top.p.x + glowR, o.top.p.y, true));
        g.fillEllipse (o.top.p.x - glowR, o.top.p.y - glowR, glowR * 2.0f, glowR * 2.0f);

        g.setColour (colourFor (s.depth, 0.95f * fog));
        g.fillEllipse (o.top.p.x - r, o.top.p.y - r, r * 2.0f, r * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (juce::jmin (1.0f, (0.55f + params.saturation) * s.env)));
        g.fillEllipse (o.top.p.x - r * 0.4f, o.top.p.y - r * 0.5f, r * 0.8f, r * 0.8f);
    }

    // HUD
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.setColour (colourFor (0.0f, 0.9f));
    g.drawText (params.freeze ? "GRANULAR FIELD  |  FROZEN" : "GRANULAR FIELD",
                juce::Rectangle<float> (b.getX() + 18.0f, b.getY() + 12.0f, 260.0f, 14.0f), juce::Justification::centredLeft, false);
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.setFont (juce::Font (11.0f));
    g.drawText ("GRAINS  " + juce::String (activeCount).paddedLeft ('0', 2) + (idle ? "   (idle preview)" : ""),
                juce::Rectangle<float> (b.getX() + 18.0f, b.getY() + 28.0f, 260.0f, 14.0f), juce::Justification::centredLeft, false);
    g.setColour (juce::Colours::white.withAlpha (0.3f));
    g.drawText ("DRAG TO ORBIT  |  DOUBLE-CLICK TO RESET",
                juce::Rectangle<float> (b.getRight() - 320.0f, b.getY() + 12.0f, 300.0f, 14.0f), juce::Justification::centredRight, false);

    g.restoreState();

    g.setColour (colourFor (0.0f, 0.22f));
    g.drawRoundedRectangle (b.reduced (0.75f), 20.0f, 1.5f);
}
