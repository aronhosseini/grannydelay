#include "PresetBar.h"

namespace
{
    constexpr int userIdBase = 1000;
    const juce::Colour accent (0xFF00E5FF);

    void styleButton (juce::TextButton& b)
    {
        b.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF14162B));
        b.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFF1E2140));
        b.setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.9f));
        b.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    }
}

PresetBar::PresetBar (PresetManager& manager) : presets (manager)
{
    combo.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF14162B));
    combo.setColour (juce::ComboBox::outlineColourId, accent.withAlpha (0.35f));
    combo.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    combo.setColour (juce::ComboBox::arrowColourId, accent);
    combo.setJustificationType (juce::Justification::centred);
    combo.setTextWhenNothingSelected ("Custom");
    combo.onChange = [this]
    {
        const int id = combo.getSelectedId();
        if (id >= 1 && id < userIdBase)       presets.loadFactory (id - 1);
        else if (id >= userIdBase)            presets.loadUser (combo.getText());
        updateButtons();
    };
    addAndMakeVisible (combo);

    for (auto* b : { &prevButton, &nextButton, &saveButton, &deleteButton })
    {
        styleButton (*b);
        addAndMakeVisible (*b);
    }

    prevButton.onClick   = [this] { step (-1); };
    nextButton.onClick   = [this] { step (1); };
    saveButton.onClick   = [this] { promptSave(); };
    deleteButton.onClick = [this] { promptDelete(); };

    refresh();
}

void PresetBar::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawText ("PRESET", getLocalBounds().removeFromLeft (70), juce::Justification::centredLeft, false);
}

void PresetBar::resized()
{
    auto area = getLocalBounds();
    area.removeFromLeft (70);
    auto right = area.removeFromRight (190);
    deleteButton.setBounds (right.removeFromRight (90).reduced (2, 4));
    saveButton.setBounds (right.removeFromRight (90).reduced (2, 4));

    prevButton.setBounds (area.removeFromLeft (36).reduced (2, 4));
    nextButton.setBounds (area.removeFromRight (36).reduced (2, 4));
    combo.setBounds (area.reduced (4, 4));
}

void PresetBar::refresh()
{
    rebuild();
    selectCurrent();
    updateButtons();
}

void PresetBar::rebuild()
{
    combo.clear (juce::dontSendNotification);

    const auto& list = PresetManager::getFactoryPresets();
    juce::String lastCategory;
    for (size_t i = 0; i < list.size(); ++i)
    {
        if (lastCategory != list[i].category)
        {
            if (lastCategory.isNotEmpty())
                combo.addSeparator();
            combo.addSectionHeading (list[i].category);
            lastCategory = list[i].category;
        }
        combo.addItem (list[i].name, (int) i + 1);
    }

    auto userNames = presets.getUserPresetNames();
    if (! userNames.isEmpty())
    {
        combo.addSeparator();
        combo.addSectionHeading ("USER PRESETS");
        for (int i = 0; i < userNames.size(); ++i)
            combo.addItem (userNames[i], userIdBase + i);
    }
}

void PresetBar::selectCurrent()
{
    const auto name = presets.getCurrentName();
    for (int i = 0; i < combo.getNumItems(); ++i)
    {
        if (combo.getItemText (i) == name)
        {
            combo.setSelectedItemIndex (i, juce::dontSendNotification);
            return;
        }
    }
    combo.setSelectedId (0, juce::dontSendNotification);
}

void PresetBar::updateButtons()
{
    deleteButton.setEnabled (presets.isUserPreset (presets.getCurrentName()));
}

void PresetBar::step (int direction)
{
    const int n = combo.getNumItems();
    if (n == 0) return;

    int index = combo.getSelectedItemIndex();
    index = (index < 0) ? (direction > 0 ? 0 : n - 1) : (index + direction + n) % n;
    combo.setSelectedItemIndex (index, juce::sendNotification);
}

void PresetBar::promptSave()
{
    const auto current = presets.getCurrentName();

    auto* aw = new juce::AlertWindow ("Save preset", "Give your preset a name:",
                                      juce::MessageBoxIconType::NoIcon, this);
    aw->addTextEditor ("name", presets.isUserPreset (current) ? current : "My Preset");
    aw->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<PresetBar> safe (this);
    aw->enterModalState (true,
        juce::ModalCallbackFunction::create ([safe, aw] (int result)
        {
            if (result != 1 || safe == nullptr) return;
            const auto name = aw->getTextEditorContents ("name").trim();
            if (name.isNotEmpty() && safe->presets.saveUser (name))
                safe->refresh();
        }),
        true);
}

void PresetBar::promptDelete()
{
    const auto name = presets.getCurrentName();
    if (! presets.isUserPreset (name)) return;

    auto* aw = new juce::AlertWindow ("Delete preset", "Delete \"" + name + "\"?",
                                      juce::MessageBoxIconType::NoIcon, this);
    aw->addButton ("Delete", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<PresetBar> safe (this);
    aw->enterModalState (true,
        juce::ModalCallbackFunction::create ([safe, name] (int result)
        {
            if (result != 1 || safe == nullptr) return;
            if (safe->presets.deleteUser (name))
            {
                safe->presets.loadFactory (0);   // fall back to Init
                safe->refresh();
            }
        }),
        true);
}
