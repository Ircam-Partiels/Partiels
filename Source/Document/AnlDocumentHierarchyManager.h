#pragma once

#include "../Track/AnlTrackHierarchyManager.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class HierarchyManager
    : public Track::HierarchyManager
    {
    public:
        HierarchyManager(Accessor& accessor);
        ~HierarchyManager() override = default;

        // Track::HierarchyManager
        std::map<juce::String, juce::String> getAvailableTracksFor(juce::String const& current, Track::FrameType const frameType) const override;
        bool isTrackValidFor(juce::String const& current, Track::FrameType const frameType, juce::String const& identifier) const override;
        bool hasAccessor(juce::String const& identifier) const override;
        Track::Accessor const& getAccessor(juce::String const& identifier) const override;

        using Track::HierarchyManager::notifyHierarchyChanged;
        using Track::HierarchyManager::notifyResultsChanged;

    private:
        Accessor& mAccessor;
    };
} // namespace Document

ANALYSE_FILE_END
