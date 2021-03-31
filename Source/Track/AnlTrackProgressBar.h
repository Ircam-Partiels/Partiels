#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class ProgressBar
    : public juce::Component
    , public juce::SettableTooltipClient
    {
    public:
        ProgressBar(Accessor& accessor);
        ~ProgressBar() override;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;
        double mProgressValue;
        juce::ProgressBar mProgressBar {mProgressValue};
        juce::Image mStateImage;
        juce::String mMessage;
    };
}

ANALYSE_FILE_END
