#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

//! @brief A window that floats over the application
class FloatingWindow
: public juce::DialogWindow
#ifndef JUCE_MAC
, private juce::FocusChangeListener
#endif
{
public:
    
    enum ColourIds : int
    {
        backgroundColourId = 0x2000300
    };
    
    FloatingWindow(juce::String const& name, bool escapeKeyTriggersCloseButton = true, bool addToDesktop = true);
    ~FloatingWindow() override = default;
    
    std::function<void()> onChanged = nullptr;
private:
    
    // juce::DialogWindow
    void closeButtonPressed() override;
    
    void lookAndFeelChanged() override;
    void parentHierarchyChanged() override;
    void visibilityChanged() override;
    void moved() override;
    void resized() override;
    
#ifdef JUCE_MAC
    static void setFloatingProperty(juce::Component& component, bool state);
#else
    // juce::FocusChangeListener
    void globalFocusChanged(juce::Component* focusedComponent) override;
#endif
};

ANALYSE_FILE_END
