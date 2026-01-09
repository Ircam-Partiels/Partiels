#pragma once

#include "AnlGroupModel.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    namespace Exporter
    {
        juce::Image toImage(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, std::set<size_t> const& channels, int width, int height, int scaledWidth, int scaledHeight, Zoom::Grid::Justification outsideGridjustification);
        juce::Result toImage(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, std::set<size_t> const& channels, juce::File const& file, int width, int height, int scaledWidth, int scaledHeight, Zoom::Grid::Justification outsideGridjustification, std::atomic<bool> const& shouldAbort);
    } // namespace Exporter
} // namespace Group

ANALYSE_FILE_END
