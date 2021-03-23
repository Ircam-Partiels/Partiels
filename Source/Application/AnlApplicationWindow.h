#pragma once

#include "AnlApplicationInterface.h"
#include "AnlApplicationCommandTarget.h"

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
        
        // juce::DocumentWindow
        void closeButtonPressed() override;
        void resized() override;
        void moved() override;
        
    private:
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;
        
        // juce::ChangeListener
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;
        
        juce::ComponentBoundsConstrainer mBoundsConstrainer;
        MainMenuModel mMainMenuModel;
        Interface mInterface;
        juce::OpenGLContext mOpenGLContext;
    };
}

ANALYSE_FILE_END

