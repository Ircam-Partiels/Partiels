#pragma once

#include "../Document/AnlDocumentDirector.h"
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
        // The tracks are added with the presets (processor and graphic presets for the plugin key, or the global graphic preset)
        // If the output of the plugin has voices, a linked track is added for each voice from the bottom to the top (the first identifier is the one of the first voice at the bottom)
        // and the number of voices is the default one of the plugin if voiceCount is 0
        std::tuple<juce::Result, std::vector<juce::String>> addPluginTrack(Document::Director& documentDir, Track::PresetList::Accessor const& presetListAcsr, Track::GraphicsSettings const& globalGraphicPreset, juce::String const& groupIdentifier, size_t const trackPosition, Plugin::Key const& key, size_t const voiceCount = 0_z);
        std::tuple<juce::Result, juce::String> addFileTrack(Document::Director& documentDir, Track::GraphicsSettings const& globalGraphicPreset, juce::String const& groupIdentifier, size_t const trackPosition, juce::File const& file);

        // Tools that help to create tracks and groups with any user interface and undo/redo management
        // The positions specify where the new groups or tracks should be inserted; these helpers wrap
        // the low-level creation logic with appropriate UI updates and undo/redo transactions.
        void addGroups(size_t numGroups, size_t firstGroupPosition);
        // If an associated component is defined, the numbers of voices of the outputs with voices are requested with a
        // call out box next to the component before creating the tracks (otherwise the default numbers are used)
        void addPluginTracks(juce::String const& groupIdentifier, size_t const& trackFirstPosition, std::vector<Plugin::Key> const& keys, juce::Component* associatedComponent = nullptr);
        void addFileTracks(juce::String const& groupIdentifier, size_t const& trackFirstPosition, std::vector<juce::File> const& files);

        void notifyForNewVersion(Misc::Version const& upstreamVersion, Version const currentVersion, bool isCurrentVersionDev, bool warnIfUpToDate, juce::String const& product, juce::String const& project, std::function<void(int)> callback);
    } // namespace Tools
} // namespace Application

ANALYSE_FILE_END
