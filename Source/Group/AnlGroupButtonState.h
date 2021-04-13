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
        Accessor& mAccessor;
        Accessor::Listener mListener;
        TrackMap<std::reference_wrapper<Track::Accessor>> mTrackAccessors;
        
        Track::Accessor::Listener mTrackListener;
        LoadingCircle mProcessingButton;
    };
}

ANALYSE_FILE_END
