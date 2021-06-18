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
        // clang-format off
        enum class Mode
        {
              analysis
            , rendering
            , both
        };
        // clang-format on

        ProgressBar(Accessor& accessor, Mode mode);
        ~ProgressBar() override;

        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;

    private:
        Accessor& mAccessor;
        Mode const mMode;
        Accessor::Listener mListener;
        double mProgressValue;
        juce::ProgressBar mProgressBar{mProgressValue};
        juce::Image mStateImage;
        juce::String mMessage;
    };
} // namespace Track

ANALYSE_FILE_END
