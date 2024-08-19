#include "MiscFloatingWindow.h"

MISC_FILE_BEGIN

FloatingWindow::FloatingWindow(juce::String const& name, bool escapeKeyTriggersClose, bool addToDesktop)
: juce::DialogWindow(name, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(ColourIds::backgroundColourId), escapeKeyTriggersClose, addToDesktop)
{
#ifdef JUCE_MAC
    setFloatingProperty(*this, true);
#else
    juce::Desktop::getInstance().addFocusChangeListener(this);
#endif
    lookAndFeelChanged();
}

FloatingWindow::~FloatingWindow()
{
#ifndef JUCE_MAC
    juce::Desktop::getInstance().removeFocusChangeListener(this);
#endif
}

void FloatingWindow::highlight()
{
    startTimer(timerTime);
    mHighlighting = highlightTime;
    repaint();
}

void FloatingWindow::closeButtonPressed()
{
    setVisible(false);
}

bool FloatingWindow::keyPressed(juce::KeyPress const& key)
{
    if(key == juce::KeyPress('q', juce::ModifierKeys::commandModifier, 0))
    {
        if(auto* instance = juce::JUCEApplication::getInstance())
        {
            return instance->invokeDirectly(juce::StandardApplicationCommandIDs::quit, true);
        }
    }
    return DialogWindow::keyPressed(key);
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
}

void FloatingWindow::moved()
{
    juce::DialogWindow::moved();
}

void FloatingWindow::resized()
{
    juce::DialogWindow::resized();
}

void FloatingWindow::paintOverChildren(juce::Graphics& g)
{
    if(mHighlighting > 0)
    {
        auto const alpha = static_cast<float>(mHighlighting) / static_cast<float>(highlightTime);
        g.fillAll(findColour(ColourIds::hightlightColourId, true).withAlpha(alpha));
    }
}

#ifndef JUCE_MAC
void FloatingWindow::globalFocusChanged(juce::Component* focusedComponent)
{
#ifdef JUCE_LINUX
    if(focusedComponent != nullptr || juce::Process::isForegroundProcess())
    {
        toFront(false);
    }
#elif JUCE_WINDOWS
    setAlwaysOnTop(focusedComponent != nullptr || juce::Process::isForegroundProcess());
#endif
}
#endif

void FloatingWindow::timerCallback()
{
    mHighlighting = std::max(0, mHighlighting - timerTime);
    repaint();
    if(mHighlighting == 0)
    {
        stopTimer();
    }
}

FloatingWindowContainer::FloatingWindowContainer(juce::String const& title, juce::Component& content)
: mContent(content)
, mFloatingWindow(title, true, true)
{
    MiscWeakAssert(std::addressof(content) != this);
    auto constexpr max = std::numeric_limits<int>::max();
    mBoundsConstrainer.setMinimumOnscreenAmounts(max, 40, 40, 40);
    mFloatingWindow.setConstrainer(&mBoundsConstrainer);
}

void FloatingWindowContainer::show()
{
    auto const& desktop = juce::Desktop::getInstance();
    auto const mousePosition = desktop.getMainMouseSource().getScreenPosition().toInt();
    auto const* display = desktop.getDisplays().getDisplayForPoint(mousePosition);
    MiscWeakAssert(display != nullptr);
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
        MiscWeakAssert(display != nullptr);
        if(display != nullptr)
        {
            mFloatingWindow.setBounds(bounds.constrainedWithin(display->userArea));
        }
    }
    if(mFloatingWindow.isShowing())
    {
        toFront();
    }
    else
    {
        mContent.setLookAndFeel(&getLookAndFeel());
        mFloatingWindow.setLookAndFeel(&getLookAndFeel());
        mFloatingWindow.setVisible(true);
        mFloatingWindow.toFront(false);
    }
}

void FloatingWindowContainer::hide()
{
    mFloatingWindow.setVisible(false);
}

void FloatingWindowContainer::toFront()
{
    mFloatingWindow.toFront(false);
    mFloatingWindow.highlight();
}

void FloatingWindowContainer::lookAndFeelChanged()
{
    mContent.setLookAndFeel(&getLookAndFeel());
    mFloatingWindow.setLookAndFeel(&getLookAndFeel());
    mFloatingWindow.sendLookAndFeelChange();
    mContent.sendLookAndFeelChange();
}

MISC_FILE_END
