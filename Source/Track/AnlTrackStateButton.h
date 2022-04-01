#pragma once

#include "AnlTrackDirector.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class StateButton
    : public juce::Component
    , public juce::SettableTooltipClient
    {
    public:
        StateButton(Director& director);
        ~StateButton() override;

        // juce::Component
        void resized() override;

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        LoadingCircle mProcessingButton;
        Icon mStateIcon{Icon::Type::alert};
    };
} // namespace Track

ANALYSE_FILE_END
