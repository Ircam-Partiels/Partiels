#pragma once

#include "AnlTransportModel.h"

ANALYSE_FILE_BEGIN

namespace Transport
{
    namespace Tools
    {
        bool canRewindPlayhead(Accessor const& accessor);
        void rewindPlayhead(Accessor& accessor);
        double getNearestTime(Accessor const& accessor, double time, std::optional<juce::Range<double>> const& range);
    } // namespace Tools
} // namespace Transport

ANALYSE_FILE_END