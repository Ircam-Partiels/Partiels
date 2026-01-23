#pragma once

#include "../Track/AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Tools
    {
        // Tools that help to create tracks and groups but without any user interface and undo/redo management
        // Basically, the new position must be retrieved, then the document must be prepared for
        // the new tracks, then the tracks must be added, then the tracks must be revealed.
        size_t getNewGroupPosition();
        std::tuple<juce::String, size_t> getNewTrackPosition();
        std::tuple<juce::Result, juce::String, size_t> prepareForNewTracks(juce::String const& groupIdentifier, size_t const trackPosition);
        void revealTracks(std::set<juce::String> const& trackIdentifiers);
        std::tuple<juce::Result, juce::String> addPluginTrack(juce::String const& groupIdentifier, size_t const trackPosition, Plugin::Key const& key);
        std::tuple<juce::Result, juce::String> addFileTrack(juce::String const& groupIdentifier, size_t const trackPosition, juce::File const& file);

        // Tools that help to create tracks and groups with any user interface and undo/redo management
        // The positions specify where the new groups or tracks should be inserted; these helpers wrap
        // the low-level creation logic with appropriate UI updates and undo/redo transactions.
        void addGroups(size_t numGroups, size_t firstGroupPosition);
        void addPluginTracks(juce::String const& groupIdentifier, size_t const& trackFirstPosition, std::vector<Plugin::Key> const& keys);
        void addFileTracks(juce::String const& groupIdentifier, size_t const& trackFirstPosition, std::vector<juce::File> const& files);

        void notifyForNewVersion(Misc::Version const& upstreamVersion, Version const currentVersion, bool isCurrentVersionDev, bool warnIfUpToDate, juce::String const& product, juce::String const& project, std::function<void(int)> callback);
    } // namespace Tools
} // namespace Application

ANALYSE_FILE_END
