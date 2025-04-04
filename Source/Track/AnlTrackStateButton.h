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

        bool isProcessingOrRendering() const;
        bool hasWarning() const;

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        LoadingIcon mProcessingButton;
        Icon mStateIcon;
    };
} // namespace Track

ANALYSE_FILE_END
