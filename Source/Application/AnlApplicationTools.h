#pragma once

#include "../Track/AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Tools
    {
        std::tuple<juce::String, size_t> getNewTrackPosition();
        void addPluginTrack(std::tuple<juce::String, size_t> position, Plugin::Key const& key, Plugin::Description const& description);
        void addFileTrack(std::tuple<juce::String, size_t> position, Track::FileInfo const& fileInfo);
    } // namespace Tools
} // namespace Application

ANALYSE_FILE_END
