#pragma once

#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    namespace Selection
    {
        struct Item
        {
            juce::String identifier;
            std::optional<size_t> channel;
        };

        void clearAll(Accessor& accessor, NotificationType notification);
        void selectItem(Accessor& accessor, Item item, bool deselectOthers, bool flipSelection, NotificationType notification);
        void selectItems(Accessor& accessor, Item begin, Item end, bool deselectOthers, bool flipSelection, NotificationType notification);

        std::set<juce::String> getTracks(Accessor const& accessor);
        std::set<juce::String> getGroups(Accessor const& accessor);
        std::tuple<std::set<juce::String>, std::set<juce::String>> getItems(Accessor const& accessor);
        std::optional<juce::String> getClosestItem(Accessor const& accessor, bool ignoreCollapsedTracks = false);
        std::optional<juce::String> getFarthestItem(Accessor const& accessor, bool ignoreCollapsedTracks = false);
        std::optional<juce::String> getNextItem(Accessor const& accessor, bool ignoreCollapsedTracks = false);
    } // namespace Selection
} // namespace Document

ANALYSE_FILE_END
