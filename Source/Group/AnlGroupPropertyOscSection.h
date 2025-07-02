#pragma once

#include "AnlGroupDirector.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class PropertyOscSection
    : public juce::Component
    {
    public:
        PropertyOscSection(Director& director);
        ~PropertyOscSection() override = default;

        // juce::Component
        void resized() override;

    private:
        void showTrackOsc();

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        PropertyTextButton mPropertyTrackOsc;
        bool mTrackOscActionStarted{false};
    };
} // namespace Group

ANALYSE_FILE_END
