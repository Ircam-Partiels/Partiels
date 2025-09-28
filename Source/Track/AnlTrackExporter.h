#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    namespace Exporter
    {
        juce::Result fromPreset(Accessor& accessor, juce::File const& file);

        juce::Result toPreset(Accessor const& accessor, juce::File const& file);

        juce::Image toImage(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, std::set<size_t> const& channels, int width, int height, int scaledWidth, int scaledHeight, Zoom::Grid::Justification outsideGridjustification);
        juce::Result toImage(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, std::set<size_t> const& channels, juce::File const& file, int width, int height, int scaledWidth, int scaledHeight, Zoom::Grid::Justification outsideGridjustification, std::atomic<bool> const& shouldAbort);

        juce::Result toCsv(Accessor const& accessor, Zoom::Range timeRange, std::set<size_t> const& channels, std::ostream& stream, bool includeHeader, char separator, bool useEndTime, std::atomic<bool> const& shouldAbort, bool useSemicolonSeparator = false, bool disableEscaping = false);
        juce::Result toCsv(Accessor const& accessor, Zoom::Range timeRange, std::set<size_t> const& channels, juce::File const& file, bool includeHeader, char separator, bool useEndTime, std::atomic<bool> const& shouldAbort, bool useSemicolonSeparator = false, bool disableEscaping = false);
        juce::Result toCsv(Accessor const& accessor, Zoom::Range timeRange, std::set<size_t> const& channels, juce::String& string, bool includeHeader, char separator, bool useEndTime, std::atomic<bool> const& shouldAbort, bool useSemicolonSeparator = false, bool disableEscaping = false);

        juce::Result toJson(Accessor const& accessor, Zoom::Range timeRange, std::set<size_t> const& channels, std::ostream& stream, bool includeDescription, std::atomic<bool> const& shouldAbort);
        juce::Result toJson(Accessor const& accessor, Zoom::Range timeRange, std::set<size_t> const& channels, juce::File const& file, bool includeDescription, std::atomic<bool> const& shouldAbort);
        juce::Result toJson(Accessor const& accessor, Zoom::Range timeRange, std::set<size_t> const& channels, juce::String& string, bool includeDescription, std::atomic<bool> const& shouldAbort);

        juce::Result toCue(Accessor const& accessor, Zoom::Range timeRange, std::set<size_t> const& channels, std::ostream& stream, std::atomic<bool> const& shouldAbort);
        juce::Result toCue(Accessor const& accessor, Zoom::Range timeRange, std::set<size_t> const& channels, juce::File const& file, std::atomic<bool> const& shouldAbort);
        juce::Result toCue(Accessor const& accessor, Zoom::Range timeRange, std::set<size_t> const& channels, juce::String& string, std::atomic<bool> const& shouldAbort);

        juce::Result toReaper(Accessor const& accessor, Zoom::Range timeRange, std::set<size_t> const& channels, std::ostream& stream, bool isMarker, std::atomic<bool> const& shouldAbort);
        juce::Result toReaper(Accessor const& accessor, Zoom::Range timeRange, std::set<size_t> const& channels, juce::File const& file, bool isMarker, std::atomic<bool> const& shouldAbort);
        juce::Result toReaper(Accessor const& accessor, Zoom::Range timeRange, std::set<size_t> const& channels, juce::String& string, bool isMarker, std::atomic<bool> const& shouldAbort);

        juce::Result toBinary(Accessor const& accessor, Zoom::Range timeRange, std::set<size_t> const& channels, std::ostream& stream, std::atomic<bool> const& shouldAbort);
        juce::Result toBinary(Accessor const& accessor, Zoom::Range timeRange, std::set<size_t> const& channels, juce::File const& file, std::atomic<bool> const& shouldAbort);
        juce::Result toBinary(Accessor const& accessor, Zoom::Range timeRange, std::set<size_t> const& channels, juce::String& string, std::atomic<bool> const& shouldAbort);

        juce::Result toSdif(Accessor const& accessor, Zoom::Range timeRange, juce::File const& file, uint32_t frameId, uint32_t matrixId, std::optional<juce::String> columnName, std::atomic<bool> const& shouldAbort);

        juce::File getConsolidatedFile(Accessor const& accessor, juce::File const& directory);
        juce::Result consolidateInDirectory(Accessor const& accessor, juce::File const& directory);
    } // namespace Exporter
} // namespace Track

ANALYSE_FILE_END
