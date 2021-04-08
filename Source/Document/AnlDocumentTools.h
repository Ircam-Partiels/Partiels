#pragma once

#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    namespace Tools
    {
        std::optional<std::reference_wrapper<Track::Accessor>> getTrack(Accessor& accessor, juce::String identifier);
        bool hasTrack(Accessor const& accessor, juce::String identifier);
    }
}

ANALYSE_FILE_END
