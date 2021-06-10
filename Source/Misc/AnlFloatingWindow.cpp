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
    lookAndFeelChanged();
    if(auto* commandManager = App::getApplicationCommandManager())
    {
        addKeyListener(commandManager->getKeyMappings());
    }
}

FloatingWindow::~FloatingWindow()
{
#ifndef JUCE_MAC
    juce::Desktop::getInstance().removeFocusChangeListener(this);
#endif
    if(auto* commandManager = App::getApplicationCommandManager())
    {
        removeKeyListener(commandManager->getKeyMappings());
    }
}

void FloatingWindow::closeButtonPressed()
{
    if(mCanBeClosedByUser)
    {
        setVisible(false);
    }
    else
    {
        getLookAndFeel().playAlertSound();
    }
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

void FloatingWindow::setCanBeClosedByUser(bool state)
{
    mCanBeClosedByUser = state;
}

FloatingWindowContainer::FloatingWindowContainer(juce::String const& title, juce::Component& content)
: mContent(content)
, mFloatingWindow(title)
{
    auto constexpr max = std::numeric_limits<int>::max();
    mBoundsConstrainer.setMinimumOnscreenAmounts(max, 40, 40, 40);
    mFloatingWindow.setConstrainer(&mBoundsConstrainer);
}

void FloatingWindowContainer::show()
{
    auto const& desktop = juce::Desktop::getInstance();
    auto const mousePosition = desktop.getMainMouseSource().getScreenPosition().toInt();
    auto const* display = desktop.getDisplays().getDisplayForPoint(mousePosition);
    anlStrongAssert(display != nullptr);
    if(display != nullptr)
    {
        showAt(display->userArea.getCentre().translated(-mFloatingWindow.getWidth() / 2, -mFloatingWindow.getHeight() / 2));
    }
}

void FloatingWindowContainer::showAt(juce::Point<int> const& pt)
{
    if(mFloatingWindow.getContentComponent() == nullptr)
    {
        mFloatingWindow.setContentNonOwned(&mContent, true);
        juce::Rectangle<int> const bounds{pt.x, pt.y, mFloatingWindow.getWidth(), mFloatingWindow.getHeight()};
        auto const& desktop = juce::Desktop::getInstance();
        auto const* display = desktop.getDisplays().getDisplayForPoint(pt);
        anlStrongAssert(display != nullptr);
        if(display != nullptr)
        {
            mFloatingWindow.setBounds(bounds.constrainedWithin(display->userArea));
        }
    }

    mFloatingWindow.setVisible(true);
    mFloatingWindow.toFront(false);
}

void FloatingWindowContainer::hide()
{
    mFloatingWindow.setVisible(false);
}

ANALYSE_FILE_END
