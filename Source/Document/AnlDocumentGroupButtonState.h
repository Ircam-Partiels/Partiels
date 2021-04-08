#pragma once

#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class GroupStateButton
    : public juce::Component
    , public juce::SettableTooltipClient
    {
    public:
        GroupStateButton(Accessor& accessor);
        ~GroupStateButton() override;
        
        // juce::Component
        void resized() override;
        
    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;
        Track::Accessor::Listener mTrackListener;
        
        std::map<juce::String, std::reference_wrapper<Track::Accessor>> mTrackAccessors;
        LoadingCircle mProcessingButton;
    };
}

ANALYSE_FILE_END
