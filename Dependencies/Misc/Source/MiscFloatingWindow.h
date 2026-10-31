#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

//! @brief A window that floats over the window it is opened from
class FloatingWindow
: public juce::DialogWindow
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
    void moved() override;
    void resized() override;
    void paintOverChildren(juce::Graphics& g) override;

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

    //! @brief Shows the window centred on the display under the mouse
    //! @param associatedComponent The component the window is opened from (the window is kept above the window containing it)
    virtual void show(juce::Component& associatedComponent);

    //! @brief Shows the window at a position
    //! @param associatedComponent The component the window is opened from (the window is kept above the window containing it)
    //! @param pt The position of the top left corner of the window in screen coordinates
    virtual void showAt(juce::Component& associatedComponent, juce::Point<float> const& pt);
    virtual void hide();

    void toFront();
    void lookAndFeelChanged() override;

protected:
    juce::Component& mContent;
    FloatingWindow mFloatingWindow;
    juce::ComponentBoundsConstrainer mBoundsConstrainer;
};

MISC_FILE_END
