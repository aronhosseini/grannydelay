#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <vector>
#include "PluginProcessor.h"
#include "UI/GranularVisualizer.h"
#include "UI/ModernKnob.h"
#include "UI/ModSourceHandle.h"
#include "UI/AnimatedBackground.h"
#include "UI/PresetBar.h"

class GrannyDelayAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                         public juce::DragAndDropContainer,
                                         private juce::Timer
{
public:
    explicit GrannyDelayAudioProcessorEditor (GrannyDelayAudioProcessor&);
    ~GrannyDelayAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment    = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    static constexpr int baseW = 1400;
    static constexpr int baseH = 950;

    enum PanelId { pTime = 0, pGrains, pFilter, pChord, pReverb, pLfo1, pLfo2, pEnv1, pEnv2, numPanels };

    struct HeaderItem { juce::Component* comp; int width; };
    struct Panel
    {
        juce::String title;
        juce::Colour colour;
        juce::Rectangle<int> rect;
        std::vector<ModernKnob*> knobs;
        std::vector<HeaderItem> header;
        juce::Component* lead = nullptr;     // first column (modulation source handle)
    };

    // non-interactive layer that draws the title and the glass panels above the animated background
    struct DecorLayer : public juce::Component
    {
        DecorLayer() { setInterceptsMouseClicks (false, false); }
        void paint (juce::Graphics& g) override { if (painter) painter (g); }
        std::function<void (juce::Graphics&)> painter;
    };

    void timerCallback() override;
    void paintDecor (juce::Graphics&);
    void layoutContent();

    void addKnob (int panel, const juce::String& name, const char* paramId, juce::Colour accent);
    void addCombo (int panel, const char* paramId, int width);
    void addToggle (int panel, const char* paramId, const juce::String& text, int width, juce::Colour colour);
    void addSource (int panel, int sourceIndex);
    void layoutRow (std::initializer_list<int> ids, juce::Rectangle<int> row);

    GrannyDelayAudioProcessor& audioProcessor;
    juce::ComponentBoundsConstrainer constrainer;
    juce::TooltipWindow tooltipWindow { this, 600 };

    juce::Component content;      // fixed-size design surface; scaled to the window size
    AnimatedBackground background;
    DecorLayer decor;
    GranularVisualizer visualizer;
    PresetBar presetBar;
    juce::HyperlinkButton authorLink;

    std::array<Panel, numPanels> panels;

    // widgets first, attachments after (attachments are destroyed first)
    std::vector<std::unique_ptr<ModernKnob>> knobs;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;
    std::vector<std::unique_ptr<juce::TextButton>> toggles;
    std::vector<std::unique_ptr<ModSourceHandle>> handles;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;

    juce::Rectangle<int> titleRect, authorRect;

    struct Watched
    {
        std::atomic<float> *pitch, *spread, *reverbMix, *motion, *saturation, *distance, *grainSize, *density, *delay, *freeze;
    } watched {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrannyDelayAudioProcessorEditor)
};
