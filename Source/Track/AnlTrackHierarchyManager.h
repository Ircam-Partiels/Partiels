#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class HierarchyManager
    {
    public:
        // clang-format off
        enum class InputChangeType
        {
              results
            , extraThresholds
        };
        // clang-format on

        HierarchyManager() = default;
        virtual ~HierarchyManager() = default;

        struct TrackInfo
        {
            juce::String identifier;
            juce::String group;
            juce::String name;
        };

        virtual std::vector<TrackInfo> getAvailableTracksFor(juce::String const& ownerIdentifier, juce::String const& inputIdentifier) const = 0;
        virtual bool isTrackValidFor(juce::String const& ownerIdentifier, juce::String const& inputIdentifier, juce::String const& inputTrack) const = 0;
        virtual bool hasAccessor(juce::String const& identifier) const = 0;
        virtual Accessor const& getAccessor(juce::String const& identifier) const = 0;

        struct Listener
        {
            std::function<void(HierarchyManager const&)> onHierarchyChanged = nullptr;
            std::function<void(HierarchyManager const&, juce::String const&, InputChangeType)> onResultsChanged = nullptr;
        };

        void addHierarchyListener(Listener& listener, NotificationType notification);
        void removeHierarchyListener(Listener& listener);

    protected:
        void notifyHierarchyChanged(NotificationType notification);
        void notifyResultsChanged(juce::String const& identifier, InputChangeType type, NotificationType notification);

    private:
        Notifier<Listener> mNotifier;
    };
} // namespace Track

ANALYSE_FILE_END
