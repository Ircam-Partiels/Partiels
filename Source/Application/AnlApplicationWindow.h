#pragma once

#include "../Tools/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Window
    : public juce::DocumentWindow
    , private juce::AsyncUpdater
    {
    public:
        Window(juce::Component& content);
        ~Window() override;
        
        // juce::DocumentWindow
        void closeButtonPressed() override;
        void resized() override;
        void moved() override;
        
    private:
        
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;
        
        juce::ComponentBoundsConstrainer mBoundsConstrainer;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Window)
    };
}

ANALYSE_FILE_END

