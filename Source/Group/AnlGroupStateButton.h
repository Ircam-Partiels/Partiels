#pragma once

#include "AnlGroupDirector.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

namespace Group
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

        std::function<void(void)> onStateChanged = nullptr;

        bool isProcessingOrRendering() const;
        bool hasWarning() const;

    private:
        void updateTooltip();

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        LoadingIcon mProcessingButton;
        Icon mStateIcon;
        bool mHasWarning{false};
        LayoutNotifier mLayoutNotifier;
    };
} // namespace Group

ANALYSE_FILE_END
