#pragma once

#include "AnlGroupModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    namespace Exporter
    {
        struct Options;
    }
}

namespace Group
{
    namespace Exporter
    {
        juce::Image toImage(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, std::set<size_t> const& channels, int width, int height, int scaledWidth, int scaledHeight);
        juce::Result toImage(Accessor& accessor, Zoom::Accessor const& timeZoomAccessor, std::set<size_t> const& channels, juce::File const& file, int width, int height, int scaledWidth, int scaledHeight, std::atomic<bool> const& shouldAbort);
        
        juce::Image toImage(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, std::set<size_t> const& channels, int width, int height, int scaledWidth, int scaledHeight, Document::Exporter::Options const& options);
        juce::Result toImage(Accessor& accessor, Zoom::Accessor const& timeZoomAccessor, std::set<size_t> const& channels, juce::File const& file, int width, int height, int scaledWidth, int scaledHeight, std::atomic<bool> const& shouldAbort, Document::Exporter::Options const& options);
    } // namespace Exporter
} // namespace Group

ANALYSE_FILE_END
