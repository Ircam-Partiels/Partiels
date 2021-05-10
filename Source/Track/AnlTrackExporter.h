#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Exporter
    {
    public:
        static juce::Result toPreset(Accessor const& accessor, juce::File const& file);
        static juce::Result fromPreset(Accessor& accessor, juce::File const& file);

        static void toImage(Accessor const& accessor, AlertType const alertType);

        static void toCsv(Accessor const& accessor, AlertType const alertType);

        static void toXml(Accessor const& accessor, AlertType const alertType);
    };
} // namespace Track

ANALYSE_FILE_END
