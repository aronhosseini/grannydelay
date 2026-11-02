#include "PresetManager.h"

namespace { const char* const kExtension = ".gdpreset"; }

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state, ModMatrix& m) : apvts (state), matrix (m) {}

juce::File PresetManager::getUserDirectory() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    dir = dir.getChildFile ("Application Support");
   #endif
    return dir.getChildFile ("AronH").getChildFile ("GrannyDelay").getChildFile ("Presets");
}

juce::File PresetManager::fileForName (const juce::String& name) const
{
    return getUserDirectory().getChildFile (juce::File::createLegalFileName (name.trim()) + kExtension);
}

juce::String PresetManager::getCurrentName() const
{
    return apvts.state.getProperty ("presetName", "Init").toString();
}

void PresetManager::setCurrentName (const juce::String& name)
{
    apvts.state.setProperty ("presetName", name, nullptr);
}

void PresetManager::resetAllToDefault()
{
    for (auto* p : apvts.processor.getParameters())
    {
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            rp->beginChangeGesture();
            rp->setValueNotifyingHost (rp->getDefaultValue());
            rp->endChangeGesture();
        }
    }
}

void PresetManager::setParam (const juce::String& id, float realValue)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (realValue));
        p->endChangeGesture();
    }
}

bool PresetManager::loadFactory (int index)
{
    const auto& list = getFactoryPresets();
    if (index < 0 || index >= (int) list.size())
        return false;

    const auto& preset = list[(size_t) index];
    resetAllToDefault();
    for (auto& o : preset.values)
        setParam (o.id, o.value);
    matrix.applyRoutes (preset.routes);

    setCurrentName (preset.name);
    return true;
}

juce::StringArray PresetManager::getUserPresetNames() const
{
    juce::StringArray names;
    auto dir = getUserDirectory();
    if (dir.isDirectory())
        for (auto& f : dir.findChildFiles (juce::File::findFiles, false, juce::String ("*") + kExtension))
            names.add (f.getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}

bool PresetManager::isUserPreset (const juce::String& name) const
{
    return name.isNotEmpty() && fileForName (name).existsAsFile();
}

bool PresetManager::loadUser (const juce::String& name)
{
    auto file = fileForName (name);
    if (! file.existsAsFile())
        return false;

    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName ("GrannyDelayPreset"))
        return false;

    resetAllToDefault();
    for (auto* child : xml->getChildWithTagNameIterator ("Param"))
        setParam (child->getStringAttribute ("id"), (float) child->getDoubleAttribute ("value"));

    matrix.fromXml (xml->getChildByName ("ModMatrix"));

    setCurrentName (name);
    return true;
}

bool PresetManager::saveUser (const juce::String& name)
{
    auto cleanName = juce::File::createLegalFileName (name.trim());
    if (cleanName.isEmpty())
        return false;

    // never shadow a factory preset
    for (auto& f : getFactoryPresets())
        if (cleanName.equalsIgnoreCase (f.name))
            cleanName += " (User)";

    auto dir = getUserDirectory();
    if (! dir.createDirectory().wasOk())
        return false;

    juce::XmlElement xml ("GrannyDelayPreset");
    xml.setAttribute ("name", cleanName);
    xml.setAttribute ("version", 2);

    for (auto* p : apvts.processor.getParameters())
    {
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            auto* e = xml.createNewChildElement ("Param");
            e->setAttribute ("id", rp->paramID);
            e->setAttribute ("value", (double) rp->convertFrom0to1 (rp->getValue()));
        }
    }

    matrix.toXml (xml);

    if (! xml.writeTo (fileForName (cleanName)))
        return false;

    setCurrentName (cleanName);
    return true;
}

bool PresetManager::deleteUser (const juce::String& name)
{
    auto file = fileForName (name);
    return file.existsAsFile() && file.deleteFile();
}
