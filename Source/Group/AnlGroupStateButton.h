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

        std::function<void(void)> onStateChanged = nullptr;

        bool isProcessingOrRendering() const;
        bool hasWarning() const;

    private:
        void updateContent();
        void updateTooltip();

        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        Track::Accessor::Listener mTrackListener{typeid(*this).name()};
        LoadingCircle mProcessingButton;
        bool mHasWarning{false};

        TrackMap<std::reference_wrapper<Track::Accessor>> mTrackAccessors;
        LayoutNotifier mLayoutNotifier;
    };
} // namespace Group

ANALYSE_FILE_END