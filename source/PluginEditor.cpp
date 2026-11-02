#include "PluginEditor.h"

namespace
{
    const juce::Colour kTime   (0xFF00E5FF);
    const juce::Colour kGrain  (0xFFFF2BD6);
    const juce::Colour kFilter (0xFFFFB020);
    const juce::Colour kChord  (0xFF7CFF6B);
    const juce::Colour kReverb (0xFF3D8BFF);
}

GrannyDelayAudioProcessorEditor::GrannyDelayAudioProcessorEditor (GrannyDelayAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      visualizer (p.getEngine()),
      presetBar (p.presets),
      authorLink ("Aron H", juce::URL ("https://www.instagram.com/aronh.official"))
{
    addAndMakeVisible (content);
    content.addAndMakeVisible (background);
    content.addAndMakeVisible (decor);
    decor.painter = [this] (juce::Graphics& g) { paintDecor (g); };

    content.addAndMakeVisible (visualizer);
    content.addAndMakeVisible (presetBar);

    authorLink.setFont (juce::Font (20.0f, juce::Font::bold), false, juce::Justification::centredRight);
    authorLink.setColour (juce::HyperlinkButton::textColourId, juce::Colour (0xFFFF2BD6));
    authorLink.setTooltip ("Open Aron H on Instagram");
    content.addAndMakeVisible (authorLink);

    // ---- TIME
    panels[pTime] = { "TIME", kTime, {}, {}, {}, nullptr };
    addToggle (pTime, "sync", "SYNC", 64, kTime);
    addKnob (pTime, "DELAY TIME", "delayTime", juce::Colour (0xFF00E5FF));
    addKnob (pTime, "SYNC DIV",   "syncDiv",   juce::Colour (0xFF5CF0FF));
    addKnob (pTime, "FEEDBACK",   "feedback",  juce::Colour (0xFF3D8BFF));
    addKnob (pTime, "MIX",        "mix",       juce::Colour (0xFF7CF5FF));

    // ---- GRAINS
    panels[pGrains] = { "GRAINS", kGrain, {}, {}, {}, nullptr };
    addToggle (pGrains, "freeze", "FREEZE", 74, kGrain);
    addKnob (pGrains, "SIZE",       "grainSize",    juce::Colour (0xFFFF2BD6));
    addKnob (pGrains, "DENSITY",    "grainDensity", juce::Colour (0xFFFF5C8A));
    addKnob (pGrains, "PITCH",      "pitch",        juce::Colour (0xFFB86BFF));
    addKnob (pGrains, "SPREAD",     "spread",       juce::Colour (0xFFFF8AE8));
    addKnob (pGrains, "PAN",        "pan",          juce::Colour (0xFFFF9BD5));
    addKnob (pGrains, "MOTION",     "motion",       juce::Colour (0xFFFF6FB5));
    addKnob (pGrains, "SATURATION", "saturation",   juce::Colour (0xFFFF8A5C));
    addKnob (pGrains, "STRETCH",    "stretch",      juce::Colour (0xFFD66BFF));
    addKnob (pGrains, "POSITION",   "position",     juce::Colour (0xFFFF4FA0));
    addKnob (pGrains, "DISTANCE",   "distance",     juce::Colour (0xFFC49BFF));

    // ---- FILTER
    panels[pFilter] = { "FILTER", kFilter, {}, {}, {}, nullptr };
    addCombo (pFilter, "filterType", 120);
    addKnob (pFilter, "CUTOFF", "filterCutoff", juce::Colour (0xFFFFB020));
    addKnob (pFilter, "RESO",   "filterRes",    juce::Colour (0xFFFFD84D));
    addKnob (pFilter, "GAIN",   "filterGain",   juce::Colour (0xFFFF9A3D));

    // ---- CHORD RESONATOR
    panels[pChord] = { "CHORD RESONATOR", kChord, {}, {}, {}, nullptr };
    addCombo (pChord, "chordType", 92);
    addCombo (pChord, "chordRoot", 56);
    addKnob (pChord, "TIME",  "chordTime",  juce::Colour (0xFF7CFF6B));
    addKnob (pChord, "DECAY", "chordDecay", juce::Colour (0xFF4DFFB8));
    addKnob (pChord, "GAIN",  "chordGain",  juce::Colour (0xFFB5FF6B));
    addKnob (pChord, "MIX",   "chordMix",   juce::Colour (0xFF2BFFB0));

    // ---- REVERB
    panels[pReverb] = { "REVERB", kReverb, {}, {}, {}, nullptr };
    addCombo (pReverb, "reverbType", 110);
    addKnob (pReverb, "DECAY",     "reverbDecay",    juce::Colour (0xFF3D8BFF));
    addKnob (pReverb, "HIGH DAMP", "reverbHighDamp", juce::Colour (0xFF7C9BFF));
    addKnob (pReverb, "LOW DAMP",  "reverbLowDamp",  juce::Colour (0xFF4D6BFF));
    addKnob (pReverb, "PRE-DECAY", "reverbPreDecay", juce::Colour (0xFF9BC4FF));
    addKnob (pReverb, "MIX",       "space",          juce::Colour (0xFF5CB0FF));

    // ---- MODULATION (drag a source handle onto any knob)
    panels[pLfo1] = { "LFO 1", ModernKnob::sourceColour (0), {}, {}, {}, nullptr };
    addSource (pLfo1, 0);
    addCombo (pLfo1, "lfo1Shape", 70);
    addToggle (pLfo1, "lfo1Sync", "SYNC", 52, ModernKnob::sourceColour (0));
    addKnob (pLfo1, "RATE",  "lfo1Rate",  ModernKnob::sourceColour (0));
    addKnob (pLfo1, "DIV",   "lfo1Div",   ModernKnob::sourceColour (0));
    addKnob (pLfo1, "DEPTH", "lfo1Depth", ModernKnob::sourceColour (0));

    panels[pLfo2] = { "LFO 2", ModernKnob::sourceColour (1), {}, {}, {}, nullptr };
    addSource (pLfo2, 1);
    addCombo (pLfo2, "lfo2Shape", 70);
    addToggle (pLfo2, "lfo2Sync", "SYNC", 52, ModernKnob::sourceColour (1));
    addKnob (pLfo2, "RATE",  "lfo2Rate",  ModernKnob::sourceColour (1));
    addKnob (pLfo2, "DIV",   "lfo2Div",   ModernKnob::sourceColour (1));
    addKnob (pLfo2, "DEPTH", "lfo2Depth", ModernKnob::sourceColour (1));

    panels[pEnv1] = { "ENVELOPE 1", ModernKnob::sourceColour (2), {}, {}, {}, nullptr };
    addSource (pEnv1, 2);
    addKnob (pEnv1, "ATTACK",  "env1Attack",  ModernKnob::sourceColour (2));
    addKnob (pEnv1, "RELEASE", "env1Release", ModernKnob::sourceColour (2));
    addKnob (pEnv1, "GAIN",    "env1Gain",    ModernKnob::sourceColour (2));

    panels[pEnv2] = { "ENVELOPE 2", ModernKnob::sourceColour (3), {}, {}, {}, nullptr };
    addSource (pEnv2, 3);
    addKnob (pEnv2, "ATTACK",  "env2Attack",  ModernKnob::sourceColour (3));
    addKnob (pEnv2, "RELEASE", "env2Release", ModernKnob::sourceColour (3));
    addKnob (pEnv2, "GAIN",    "env2Gain",    ModernKnob::sourceColour (3));

    auto& a = audioProcessor.apvts;
    watched = { a.getRawParameterValue ("pitch"), a.getRawParameterValue ("spread"), a.getRawParameterValue ("space"),
                a.getRawParameterValue ("motion"), a.getRawParameterValue ("saturation"), a.getRawParameterValue ("distance"),
                a.getRawParameterValue ("grainSize"), a.getRawParameterValue ("grainDensity"),
                a.getRawParameterValue ("delayTime"), a.getRawParameterValue ("freeze") };

    layoutContent();

    // resizable window: fixed aspect ratio, the whole UI scales with it
    constrainer.setFixedAspectRatio ((double) baseW / (double) baseH);
    constrainer.setSizeLimits (700, (int) (700.0 * baseH / baseW), 2100, (int) (2100.0 * baseH / baseW));
    setConstrainer (&constrainer);
    setResizable (true, true);
    setSize ((int) (baseW * 0.92 * 0.93 * 0.93), (int) (baseH * 0.92 * 0.93 * 0.93));   // default opening size (about 1114 x 756)

    startTimerHz (30);
}

GrannyDelayAudioProcessorEditor::~GrannyDelayAudioProcessorEditor()
{
    stopTimer();
}

void GrannyDelayAudioProcessorEditor::addKnob (int panel, const juce::String& name, const char* paramId, juce::Colour accent)
{
    auto knob = std::make_unique<ModernKnob> (name, accent, &audioProcessor.modMatrix, ModMatrix::indexOf (paramId));
    content.addAndMakeVisible (*knob);
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (audioProcessor.apvts, paramId, knob->getSlider()));
    panels[(size_t) panel].knobs.push_back (knob.get());
    knobs.push_back (std::move (knob));
}

void GrannyDelayAudioProcessorEditor::addCombo (int panel, const char* paramId, int width)
{
    auto box = std::make_unique<juce::ComboBox>();
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (audioProcessor.apvts.getParameter (paramId)))
        box->addItemList (choice->choices, 1);

    const auto accent = panels[(size_t) panel].colour;
    box->setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF14162B));
    box->setColour (juce::ComboBox::outlineColourId, accent.withAlpha (0.45f));
    box->setColour (juce::ComboBox::textColourId, juce::Colours::white);
    box->setColour (juce::ComboBox::arrowColourId, accent);
    box->setJustificationType (juce::Justification::centred);

    content.addAndMakeVisible (*box);
    comboAttachments.push_back (std::make_unique<ComboAttachment> (audioProcessor.apvts, paramId, *box));
    panels[(size_t) panel].header.push_back ({ box.get(), width });
    combos.push_back (std::move (box));
}

void GrannyDelayAudioProcessorEditor::addToggle (int panel, const char* paramId, const juce::String& text, int width, juce::Colour colour)
{
    auto button = std::make_unique<juce::TextButton> (text);
    button->setClickingTogglesState (true);
    button->setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF14162B));
    button->setColour (juce::TextButton::buttonOnColourId, colour.withAlpha (0.85f));
    button->setColour (juce::TextButton::textColourOffId, colour);
    button->setColour (juce::TextButton::textColourOnId, juce::Colours::black);

    content.addAndMakeVisible (*button);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (audioProcessor.apvts, paramId, *button));
    panels[(size_t) panel].header.push_back ({ button.get(), width });
    toggles.push_back (std::move (button));
}

void GrannyDelayAudioProcessorEditor::addSource (int panel, int sourceIndex)
{
    auto handle = std::make_unique<ModSourceHandle> (sourceIndex, audioProcessor.modMatrix);
    content.addAndMakeVisible (*handle);
    panels[(size_t) panel].lead = handle.get();
    handles.push_back (std::move (handle));
}

void GrannyDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF07080F));
}

void GrannyDelayAudioProcessorEditor::resized()
{
    content.setBounds (0, 0, baseW, baseH);
    content.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) baseW));
}

void GrannyDelayAudioProcessorEditor::layoutRow (std::initializer_list<int> ids, juce::Rectangle<int> row)
{
    const int gap = 12, pad = 6;
    const int count = (int) ids.size();

    int totalCols = 0;
    for (int id : ids)
        totalCols += (int) panels[(size_t) id].knobs.size() + (panels[(size_t) id].lead != nullptr ? 1 : 0);

    const int colW = (row.getWidth() - gap * (count - 1) - 2 * pad * count) / totalCols;
    const int usedW = colW * totalCols + 2 * pad * count + gap * (count - 1);
    int x = row.getX() + (row.getWidth() - usedW) / 2;

    for (int id : ids)
    {
        auto& panel = panels[(size_t) id];
        const int cols = (int) panel.knobs.size() + (panel.lead != nullptr ? 1 : 0);
        panel.rect = { x, row.getY(), colW * cols + 2 * pad, row.getHeight() };

        auto inner = panel.rect.reduced (pad, 4);
        inner.removeFromTop (30);

        if (panel.lead != nullptr)
            panel.lead->setBounds (inner.removeFromLeft (colW).reduced (2, 6));

        for (auto* k : panel.knobs)
            k->setBounds (inner.removeFromLeft (colW).reduced (2, 0));

        int hx = panel.rect.getRight() - 12;
        for (auto it = panel.header.rbegin(); it != panel.header.rend(); ++it)
        {
            it->comp->setBounds (hx - it->width, panel.rect.getY() + 6, it->width, 22);
            hx -= it->width + 6;
        }

        x += panel.rect.getWidth() + gap;
    }
}

void GrannyDelayAudioProcessorEditor::layoutContent()
{
    background.setBounds (0, 0, baseW, baseH);
    decor.setBounds (0, 0, baseW, baseH);

    auto area = juce::Rectangle<int> (0, 0, baseW, baseH).reduced (20, 16);

    auto header = area.removeFromTop (58);
    titleRect = header.removeFromLeft (460);
    authorRect = header.removeFromRight (150).withTrimmedTop (22).withHeight (30);
    authorLink.setBounds (authorRect);

    presetBar.setBounds (area.removeFromTop (44).withSizeKeepingCentre (800, 44));
    area.removeFromTop (6);

    visualizer.setBounds (area.removeFromTop (250));
    area.removeFromTop (10);

    const int rowH = 176, gap = 10;
    layoutRow ({ pTime, pGrains },                    area.removeFromTop (rowH));
    area.removeFromTop (gap);
    layoutRow ({ pFilter, pChord, pReverb },          area.removeFromTop (rowH));
    area.removeFromTop (gap);
    layoutRow ({ pLfo1, pLfo2, pEnv1, pEnv2 },        area.removeFromTop (rowH));
}

void GrannyDelayAudioProcessorEditor::paintDecor (juce::Graphics& g)
{
    // title
    auto t = titleRect;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xFF00E5FF), (float) t.getX(), 0.0f,
                                             juce::Colour (0xFFFF2BD6), (float) t.getX() + 300.0f, 0.0f, false));
    g.setFont (juce::Font (34.0f, juce::Font::bold));
    g.drawText ("GRANNY DELAY", t.removeFromTop (38), juce::Justification::centredLeft, false);

    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawText ("GRANULAR DELAY  |  V1.0.0", t, juce::Justification::centredLeft, false);

    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.drawText ("DEVELOPER", juce::Rectangle<int> (authorRect.getX(), authorRect.getY() - 14, authorRect.getWidth(), 14),
                juce::Justification::centredRight, false);

    // glass panels
    for (auto& panel : panels)
    {
        auto r = panel.rect.toFloat();
        const auto c = panel.colour;

        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), r.getX(), r.getY(),
                                                 juce::Colours::white.withAlpha (0.015f), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 18.0f);
        g.setColour (c.withAlpha (0.25f));
        g.drawRoundedRectangle (r.reduced (0.5f), 18.0f, 1.2f);

        g.setColour (c);
        g.fillRoundedRectangle (r.getX() + 16.0f, r.getY() + 12.0f, 4.0f, 14.0f, 2.0f);
        g.setFont (juce::Font (12.0f, juce::Font::bold));
        g.drawText (panel.title, juce::Rectangle<float> (r.getX() + 28.0f, r.getY() + 10.0f, 130.0f, 18.0f),
                    juce::Justification::centredLeft, false);
    }
}

void GrannyDelayAudioProcessorEditor::timerCallback()
{
    GranularVisualizer::VizParams v;
    v.pitch       = watched.pitch->load();
    v.spread      = watched.spread->load();
    v.reverbMix   = watched.reverbMix->load();
    v.motion      = watched.motion->load();
    v.saturation  = watched.saturation->load();
    v.distance    = watched.distance->load();
    v.grainSizeMs = watched.grainSize->load();
    v.density     = watched.density->load();
    v.delayMs     = watched.delay->load();
    v.freeze      = watched.freeze->load() > 0.5f;
    visualizer.setParameters (v);
}
