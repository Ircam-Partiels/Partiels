#pragma once

#include "AnlApplicationInterface.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Window
    : public juce::DocumentWindow
    , private juce::AsyncUpdater
    , private juce::ChangeListener
    {
    public:
        Window();
        ~Window() override;

        void moveKeyboardFocusTo(juce::String const& identifier);
        juce::Rectangle<int> getPlotBounds(juce::String const& identifier) const;
        void showDesktopScaler();

        // juce::DocumentWindow
        void closeButtonPressed() override;
        void resized() override;
        void moved() override;
        void maximiseButtonPressed() override;
        void lookAndFeelChanged() override;

    private:
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        // juce::ChangeListener
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;

        class DesktopScaler
        : public juce::Component
        {
        public:
            DesktopScaler();
            ~DesktopScaler() override;
            void resized() override;

        private:
            PropertyNumber mNumber;
            juce::Slider mSlider;
            Accessor::Listener mListener{typeid(*this).name()};
        };

        juce::ComponentBoundsConstrainer mBoundsConstrainer;
        Interface mInterface;
        DesktopScaler mDesktopScaler;
        FloatingWindowContainer mDesktopScalerWindow;
    };
} // namespace Application

ANALYSE_FILE_END
