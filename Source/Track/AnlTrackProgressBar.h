#pragma once

#include "AnlTrackDirector.h"

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

        ProgressBar(Director& director, Mode mode);
        ~ProgressBar() override;

        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Mode const mMode;
        Accessor::Listener mListener{typeid(*this).name()};
        double mProgressValue;
        juce::ProgressBar mProgressBar{mProgressValue};
        Icon mStateIcon{Icon::Type::verified};
        juce::String mMessage;
    };
} // namespace Track

ANALYSE_FILE_END
