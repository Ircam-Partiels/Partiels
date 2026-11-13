#pragma once

#include "../Document/AnlDocumentDirector.h"
#include "../Track/AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Tools
    {
        // Tools that help to create tracks and groups but without any user interface and undo/redo management
        // Basically, the new position (or the group of the new tracks) must be retrieved, then the document must be
        // prepared for the new tracks, then the tracks must be added, then the tracks must be revealed.
        size_t getNewGroupPosition();
        // The new tracks are added to the group of the selected item (the top group if no item is selected)
        juce::String getNewTrackGroup();
        std::tuple<juce::Result, juce::String> prepareForNewTracks(juce::String const& groupIdentifier);
        void revealTracks(std::set<juce::String> const& trackIdentifiers);
        // The new tracks are inserted at the top of the group, except the vector tracks that are inserted at the bottom
        // of the group (the tracks with an unknown frame type, such as the file tracks, are inserted at the top and are not moved
        // once their results are loaded)
        size_t getNewTrackPosition(Document::Accessor const& documentAcsr, juce::String const& groupIdentifier, std::optional<Track::FrameType> const frameType);
        // The tracks are added with the presets (processor and graphic presets for the plugin key, or the global graphic preset)
        // If the output of the plugin has voices, a linked track is added for each voice from the bottom to the top (the first identifier is the one of the first voice at the bottom)
        // and the number of voices is the default one of the plugin if voiceCount is 0
        std::tuple<juce::Result, std::vector<juce::String>> addPluginTrack(Document::Director& documentDir, Track::PresetList::Accessor const& presetListAcsr, Track::GraphicsSettings const& globalGraphicPreset, juce::String const& groupIdentifier, Plugin::Key const& key, size_t const voiceCount = 0_z);
        std::tuple<juce::Result, juce::String> addFileTrack(Document::Director& documentDir, Track::GraphicsSettings const& globalGraphicPreset, juce::String const& groupIdentifier, juce::File const& file);

        // Tools that help to create tracks and groups with any user interface and undo/redo management
        // The position specifies where the new groups should be inserted and the group specifies where the new tracks
        // should be inserted; these helpers wrap the low-level creation logic with appropriate UI updates and undo/redo transactions.
        void addGroups(size_t numGroups, size_t firstGroupPosition);
        // If an associated component is defined, the numbers of voices of the outputs with voices are requested with a
        // call out box next to the component before creating the tracks (otherwise the default numbers are used)
        void addPluginTracks(juce::String const& groupIdentifier, std::vector<Plugin::Key> const& keys, juce::Component* associatedComponent = nullptr);
        void addFileTracks(juce::String const& groupIdentifier, std::vector<juce::File> const& files);

        //! @brief Notify the user if a new version is available
        //! @param upstreamVersion The newest version available (invalid if unknown)
        //! @param currentVersion The version compared with the newest version (the pre-release suffixes are considered)
        //! @param warnIfUpToDate If the user is also notified when the current version is up to date
        //! @param product The name of the product
        //! @param project The name of the project page
        //! @param callback Called with the button selected by the user (0 to close, 1 to proceed to the download page, 2 to ignore the version)
        void notifyForNewVersion(Release::Version const& upstreamVersion, Release::Version const& currentVersion, bool warnIfUpToDate, juce::String const& product, juce::String const& project, std::function<void(int)> callback);
    } // namespace Tools
} // namespace Application

ANALYSE_FILE_END
