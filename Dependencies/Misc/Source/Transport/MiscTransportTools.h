#pragma once

#include "MiscTransportModel.h"

MISC_FILE_BEGIN

namespace Transport
{
    namespace Tools
    {
        bool canRewindPlayhead(Accessor const& accessor);
        bool canMovePlayheadBackward(Accessor const& accessor, std::optional<juce::Range<double>> const& range);
        bool canMovePlayheadForward(Accessor const& accessor, std::optional<juce::Range<double>> const& range);

        void rewindPlayhead(Accessor& accessor, NotificationType notification);
        void movePlayheadBackward(Accessor& accessor, std::optional<juce::Range<double>> const& range, NotificationType notification);
        void movePlayheadForward(Accessor& accessor, std::optional<juce::Range<double>> const& range, NotificationType notification);

        double getPreviousTime(Accessor const& accessor, double time, std::optional<juce::Range<double>> const& range, bool forceMagnetize = false);
        double getNextTime(Accessor const& accessor, double time, std::optional<juce::Range<double>> const& range, bool forceMagnetize = false);
    } // namespace Tools
} // namespace Transport

MISC_FILE_END
