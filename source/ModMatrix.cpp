#include "ModMatrix.h"

namespace
{
    const char* const kTargetIds[ModMatrix::numTargets] =
    {
        "delayTime", "feedback", "mix", "grainSize", "grainDensity", "pitch", "spread", "motion",
        "saturation", "stretch", "position", "distance", "filterCutoff", "filterRes", "filterGain",
        "chordTime", "chordDecay", "chordGain", "chordMix", "reverbDecay", "space",
        "reverbHighDamp", "reverbLowDamp", "reverbPreDecay", "pan"
    };

    const char* const kSourceNames[ModMatrix::numSources] = { "LFO 1", "LFO 2", "ENV 1", "ENV 2" };
}

ModMatrix::ModMatrix()
{
    for (auto& a : amounts)     a.store (0.0f);
    for (auto& v : sourceValue) v.store (0.0f);
    for (auto& v : liveNorm)    v.store (0.0f);
}

const char* ModMatrix::targetId (int t)    { return (t >= 0 && t < numTargets) ? kTargetIds[t] : ""; }
const char* ModMatrix::sourceName (int s)  { return (s >= 0 && s < numSources) ? kSourceNames[s] : ""; }

int ModMatrix::indexOf (const juce::String& paramId)
{
    for (int t = 0; t < numTargets; ++t)
        if (paramId == kTargetIds[t])
            return t;
    return -1;
}

float ModMatrix::getAmount (int s, int t) const noexcept
{
    return amounts[(size_t) (s * numTargets + t)].load (std::memory_order_relaxed);
}

void ModMatrix::setAmount (int s, int t, float a) noexcept
{
    if (s < 0 || s >= numSources || t < 0 || t >= numTargets) return;
    amounts[(size_t) (s * numTargets + t)].store (juce::jlimit (-1.0f, 1.0f, a), std::memory_order_relaxed);
}

void ModMatrix::clear() noexcept
{
    for (auto& a : amounts) a.store (0.0f, std::memory_order_relaxed);
}

void ModMatrix::applyRoutes (const std::vector<Route>& routes)
{
    clear();
    for (auto& r : routes)
        setAmount (r.source, indexOf (r.target), r.amount);
}

void ModMatrix::toXml (juce::XmlElement& parent) const
{
    auto* m = parent.createNewChildElement ("ModMatrix");
    for (int s = 0; s < numSources; ++s)
        for (int t = 0; t < numTargets; ++t)
        {
            const float a = getAmount (s, t);
            if (std::abs (a) > 0.0001f)
            {
                auto* r = m->createNewChildElement ("Route");
                r->setAttribute ("source", s);
                r->setAttribute ("target", kTargetIds[t]);
                r->setAttribute ("amount", (double) a);
            }
        }
}

void ModMatrix::fromXml (const juce::XmlElement* e)
{
    clear();
    if (e == nullptr) return;

    for (auto* r : e->getChildWithTagNameIterator ("Route"))
        setAmount (r->getIntAttribute ("source", -1), indexOf (r->getStringAttribute ("target")),
                   (float) r->getDoubleAttribute ("amount", 0.0));
}
