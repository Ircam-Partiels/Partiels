#include "AnlFloatingWindow.h"

ANALYSE_FILE_BEGIN

FloatingWindow::FloatingWindow(juce::String const& name, bool escapeKeyTriggersClose, bool addToDesktop, bool alwayOnTop)
: juce::DialogWindow(name, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(ColourIds::backgroundColourId), escapeKeyTriggersClose, addToDesktop)
{
    if(alwayOnTop)
    {
        setAlwaysOnTop(true);
    }
    else
    {
#if JUCE_MAC
        setFloatingProperty(*this, true);
#else
        juce::Desktop::getInstance().addFocusChangeListener(this);
#endif
    }
    lookAndFeelChanged();
    if(auto* commandManager = App::getApplicationCommandManager())
    {
        addKeyListener(commandManager->getKeyMappings());
    }
}

FloatingWindow::~FloatingWindow()
{
#if !JUCE_MAC
    juce::Desktop::getInstance().removeFocusChangeListener(this);
#endif
    if(auto* commandManager = App::getApplicationCommandManager())
    {
        removeKeyListener(commandManager->getKeyMappings());
    }
}

void FloatingWindow::closeButtonPressed()
{
    if(onCloseButtonPressed == nullptr || onCloseButtonPressed())
    {
        setVisible(false);
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
    setAlwaysOnTop(focusedComponent != nullptr || juce::Process::isForegroundProcess());
}
#endif

FloatingWindowContainer::FloatingWindowContainer(juce::String const& title, juce::Component& content, bool alwayOnTop)
: mContent(content)
, mFloatingWindow(title, true, true, alwayOnTop)
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
        showAt((mContent.getBounds().withCentre(display->userArea.getCentre())).getTopLeft());
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
