#pragma once

#include "../AnlTrackDirector.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Scroller
    : public juce::Component
    , private juce::ApplicationCommandManagerListener
    {
    public:
        Scroller(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor, juce::ApplicationCommandManager& commandManager);
        ~Scroller() override;

        // juce::Component
        void resized() override;

        // juce::Component
        void mouseMagnify(juce::MouseEvent const& event, float magnifyAmount) override;
        void mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel) override;

    private:
        // juce::ApplicationCommandManagerListener
        void applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info) override;
        void applicationCommandListChanged() override;

        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Transport::Accessor& mTransportAccessor;
        juce::ApplicationCommandManager& mCommandManager;
        Accessor::Listener mListener{typeid(*this).name()};
        std::vector<std::unique_ptr<MouseScroller>> mMouseScrollers;
    };
} // namespace Track

ANALYSE_FILE_END
