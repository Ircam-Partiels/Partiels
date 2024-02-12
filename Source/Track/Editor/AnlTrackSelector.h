#pragma once

#include "../AnlTrackDirector.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Selector
    : public juce::Component
    , private juce::ApplicationCommandManagerListener
    {
    public:
        Selector(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor, juce::ApplicationCommandManager& commandManager);
        ~Selector() override;

        // juce::Component
        void resized() override;
        void colourChanged() override;

        void setFocusInfo(FocusInfo const& info);

    private:
        // juce::ApplicationCommandManagerListener
        void applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info) override;
        void applicationCommandListChanged() override;

        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Transport::Accessor& mTransportAccessor;
        juce::ApplicationCommandManager& mCommandManager;
        Accessor::Listener mListener{typeid(*this).name()};
        std::vector<std::unique_ptr<Transport::SelectionBar>> mSelectionBars;
        FocusInfo mFocusInfo;
    };
} // namespace Track

ANALYSE_FILE_END
