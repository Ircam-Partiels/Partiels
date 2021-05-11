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

        // juce::DocumentWindow
        void closeButtonPressed() override;
        void resized() override;
        void moved() override;
        void lookAndFeelChanged() override;

    private:
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        // juce::ChangeListener
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;

        juce::ComponentBoundsConstrainer mBoundsConstrainer;
        Interface mInterface;
#ifndef JUCE_MAC
        juce::OpenGLContext mOpenGLContext;
#endif
    };
} // namespace Application

ANALYSE_FILE_END
