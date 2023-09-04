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

        Interface& getInterface();

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

        juce::ComponentBoundsConstrainer mBoundsConstrainer;
        Interface mInterface;
    };
} // namespace Application

ANALYSE_FILE_END
