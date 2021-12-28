#pragma once

#include "AnlTransportModel.h"

ANALYSE_FILE_BEGIN

namespace Transport
{
    namespace Tools
    {
        bool canRewindPlayhead(Accessor const& accessor);
        bool canMovePlayheadBackward(Accessor const& accessor, std::optional<juce::Range<double>> const& range);
        bool canMovePlayheadForward(Accessor const& accessor, std::optional<juce::Range<double>> const& range);

        void rewindPlayhead(Accessor& accessor);
        double getPreviousTime(Accessor const& accessor, double time, std::optional<juce::Range<double>> const& range);
        double getNextTime(Accessor const& accessor, double time, std::optional<juce::Range<double>> const& range);
        double getNearestTime(Accessor const& accessor, double time, std::optional<juce::Range<double>> const& range);
        juce::Range<double> getTimeRange(Accessor const& accessor, double time, std::optional<juce::Range<double>> const& range);
    } // namespace Tools
} // namespace Transport

ANALYSE_FILE_END
