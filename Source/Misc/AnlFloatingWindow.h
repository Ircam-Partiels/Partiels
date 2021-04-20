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
    // clang-format off
    enum ColourIds : int
    {
          backgroundColourId = 0x2000400
    };
    // clang-format on

    FloatingWindow(juce::String const& name, bool escapeKeyTriggersCloseButton = true, bool addToDesktop = true);
    ~FloatingWindow() override;

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

class FloatingWindowContainer
: public juce::Component
{
public:
    FloatingWindowContainer(juce::String const& title, juce::Component& content);
    ~FloatingWindowContainer() override = default;

    virtual void show();
    virtual void show(juce::Point<int> const& pt);
    virtual void hide();

protected:
    juce::Component& mContent;
    FloatingWindow mFloatingWindow;
};

ANALYSE_FILE_END
