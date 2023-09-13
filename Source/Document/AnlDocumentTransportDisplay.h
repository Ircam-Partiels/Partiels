#pragma once

#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class TransportDisplay
    : public juce::Component
    , private juce::ApplicationCommandManagerListener
    {
    public:
        TransportDisplay(Transport::Accessor& accessor, Zoom::Accessor& zoomAcsr, juce::ApplicationCommandManager& commandManager);
        ~TransportDisplay() override;

        // juce::Component
        void resized() override;

    private:
        // juce::ApplicationCommandManagerListener
        void applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info) override;
        void applicationCommandListChanged() override;

        Transport::Accessor& mTransportAccessor;
        Transport::Accessor::Listener mTransportListener{typeid(*this).name()};
        Zoom::Accessor& mZoomAccessor;
        Zoom::Accessor::Listener mZoomListener{typeid(*this).name()};
        juce::ApplicationCommandManager& mApplicationCommandManager;

        Icon mRewindButton;
        Icon mPlaybackButton;
        Icon mLoopButton;
        HMSmsField mPosition;
        juce::Slider mVolumeSlider{juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::NoTextBox};
    };
} // namespace Document

ANALYSE_FILE_END
