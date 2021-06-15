#pragma once

#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class StateButton
    : public juce::Component
    , public juce::SettableTooltipClient
    {
    public:
        StateButton(Accessor& groupAccessor);
        ~StateButton() override;

        // juce::Component
        void resized() override;

    private:
        void updateContent();

        Accessor& mAccessor;
        Accessor::Listener mListener;
        Track::Accessor::Listener mTrackListener;
        LoadingCircle mProcessingButton;

        TrackMap<std::reference_wrapper<Track::Accessor>> mTrackAccessors;
        TrackLayoutNotifier mTrackLayoutNotifier;
    };
} // namespace Group

ANALYSE_FILE_END
