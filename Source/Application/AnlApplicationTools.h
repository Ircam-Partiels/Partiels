#pragma once

#include "../Track/AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Tools
    {
        size_t getNewGroupPosition();
        std::tuple<juce::String, size_t> getNewTrackPosition();
        void addPluginTracks(std::tuple<juce::String, size_t> position, std::set<Plugin::Key> const& keys);
        void addFileTrack(std::tuple<juce::String, size_t> position, Track::FileInfo const& fileInfo);
    } // namespace Tools
} // namespace Application

ANALYSE_FILE_END
