#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

//! @brief A window that floats over the application
class FloatingWindow
: public juce::DialogWindow
#ifndef JUCE_MAC
, private juce::FocusChangeListener
#endif
, private juce::Timer
{
public:
    // clang-format off
    enum ColourIds : int
    {
          backgroundColourId = 0x2000400
        , hightlightColourId = 0x2000401
    };
    // clang-format on

    FloatingWindow(juce::String const& name, bool escapeKeyTriggersClose = true, bool addToDesktop = true);
    ~FloatingWindow() override;

    void highlight();

private:
    // juce::DialogWindow
    void closeButtonPressed() override;
    bool keyPressed(juce::KeyPress const& key) override;

    // juce::Component
    void lookAndFeelChanged() override;
    void parentHierarchyChanged() override;
    void visibilityChanged() override;
    void moved() override;
    void resized() override;
    void paintOverChildren(juce::Graphics& g) override;

#ifdef JUCE_MAC
    static void setFloatingProperty(juce::Component& component, bool state);
#else
    // juce::FocusChangeListener
    void globalFocusChanged(juce::Component* focusedComponent) override;
#endif

    // juce::Timer
    void timerCallback() override;

    int mHighlighting{0};
    static auto constexpr highlightTime = 1000;
    static auto constexpr timerTime = 10;
};

class FloatingWindowContainer
: public juce::Component
{
public:
    FloatingWindowContainer(juce::String const& title, juce::Component& content);
    ~FloatingWindowContainer() override = default;

    virtual void show();
    virtual void showAt(juce::Point<int> const& pt);
    virtual void hide();

    void toFront();
    void lookAndFeelChanged() override;

protected:
    juce::Component& mContent;
    FloatingWindow mFloatingWindow;
    juce::ComponentBoundsConstrainer mBoundsConstrainer;
};

MISC_FILE_END
