#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class HierarchyManager
    {
    public:
        HierarchyManager() = default;
        virtual ~HierarchyManager() = default;

        struct TrackInfo
        {
            juce::String identifier;
            juce::String group;
            juce::String name;
        };

        virtual std::vector<TrackInfo> getAvailableTracksFor(juce::String const& current, FrameType const frameType) const = 0;
        virtual bool isTrackValidFor(juce::String const& current, Track::FrameType const frameType, juce::String const& identifier) const = 0;
        virtual bool hasAccessor(juce::String const& identifier) const = 0;
        virtual Accessor const& getAccessor(juce::String const& identifier) const = 0;

        struct Listener
        {
            std::function<void(HierarchyManager const&)> onHierarchyChanged = nullptr;
            std::function<void(HierarchyManager const&, juce::String const&)> onResultsChanged = nullptr;
        };

        void addHierarchyListener(Listener& listener, NotificationType notification);
        void removeHierarchyListener(Listener& listener);

    protected:
        void notifyHierarchyChanged(NotificationType notification);
        void notifyResultsChanged(juce::String const& identifier, NotificationType notification);

    private:
        Notifier<Listener> mNotifier;
    };
} // namespace Track

ANALYSE_FILE_END
