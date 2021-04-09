#include "AnlFloatingWindow.h"

ANALYSE_FILE_BEGIN

FloatingWindow::FloatingWindow(juce::String const& name, bool escapeKeyTriggersCloseButton, bool addToDesktop)
: juce::DialogWindow(name, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(ColourIds::backgroundColourId), escapeKeyTriggersCloseButton, addToDesktop)
{
#ifdef JUCE_MAC
    setFloatingProperty(*this, true);
#else
    juce::Desktop::getInstance().addFocusChangeListener(this);
#endif
}

FloatingWindow::~FloatingWindow()
{
#ifndef JUCE_MAC
    juce::Desktop::getInstance().removeFocusChangeListener(this);
#endif
}

void FloatingWindow::closeButtonPressed()
{
    setVisible(false);
}

void FloatingWindow::lookAndFeelChanged()
{
    setBackgroundColour(findColour(ColourIds::backgroundColourId, true));
    juce::DialogWindow::lookAndFeelChanged();
}

void FloatingWindow::parentHierarchyChanged()
{
    setBackgroundColour(findColour(ColourIds::backgroundColourId, true));
    juce::DialogWindow::parentHierarchyChanged();
}

void FloatingWindow::visibilityChanged()
{
    juce::DialogWindow::visibilityChanged();
    if(onChanged != nullptr)
    {
        onChanged();
    }
}

void FloatingWindow::moved()
{
    juce::DialogWindow::moved();
    if(onChanged != nullptr)
    {
        onChanged();
    }
}

void FloatingWindow::resized()
{
    juce::DialogWindow::resized();
    if(onChanged != nullptr)
    {
        onChanged();
    }
}

#ifndef JUCE_MAC
void FloatingWindow::globalFocusChanged(juce::Component* focusedComponent)
{
    setAlwaysOnTop(focusedComponent != nullptr);
}
#endif

FloatingWindowContainer::FloatingWindowContainer(juce::String const& title)
: mFloatingWindow(title)
{
}

void FloatingWindowContainer::show()
{
    if(mFloatingWindow.getContentComponent() == nullptr)
    {
        auto const& desktop = juce::Desktop::getInstance();
        auto const mousePosition = desktop.getMainMouseSource().getScreenPosition().toInt();
        auto const* display = desktop.getDisplays().getDisplayForPoint(mousePosition);
        anlStrongAssert(display != nullptr);
        if(display != nullptr)
        {
            auto const bounds = display->userArea.withSizeKeepingCentre(mFloatingWindow.getWidth(), mFloatingWindow.getHeight());
            mFloatingWindow.setBounds(bounds);
        }
        mFloatingWindow.setContentNonOwned(this, true);
    }
    
    mFloatingWindow.setVisible(true);
    mFloatingWindow.toFront(false);
}

void FloatingWindowContainer::hide()
{
    mFloatingWindow.setVisible(false);
}

ANALYSE_FILE_END
