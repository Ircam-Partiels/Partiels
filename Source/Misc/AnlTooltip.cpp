#include "AnlTooltip.h"

ANALYSE_FILE_BEGIN

void Tooltip::Server::timerCallback()
{
    auto getTipFor = [](juce::Component* component) -> juce::String
    {
        if(component != nullptr && !component->isCurrentlyBlockedByAnotherModalComponent() &&  juce::Process::isForegroundProcess())
        {
            if(dynamic_cast<BubbleClient*>(component) != nullptr)
            {
                return {};
            }
            if(auto* client = dynamic_cast<Client*>(component))
            {
                return client->getTooltip();
            }
        }
        return {};
    };
    
    auto mouseSource = juce::Desktop::getInstance().getMainMouseSource();
    auto const newTip = getTipFor(mouseSource.isTouch() ? nullptr : mouseSource.getComponentUnderMouse());
    if(newTip != mTip)
    {
        mTip = newTip;
        if(onChanged != nullptr)
        {
            onChanged(mTip);
        }
    }
}

Tooltip::Display::Display()
{
    addAndMakeVisible(mLabel);
    mServer.onChanged = [&](juce::String const& tip)
    {
        mLabel.setText(tip, juce::NotificationType::dontSendNotification);
    };
    if(juce::Desktop::getInstance().getMainMouseSource().canHover())
    {
        mServer.startTimer(123);
    }
}

void Tooltip::Display::resized()
{
    mLabel.setBounds(getLocalBounds());
}

Tooltip::BubbleWindow::BubbleWindow()
: juce::Component("TooltipBubbleWindow")
{
    setAlwaysOnTop(true);
    setOpaque(true);
    
    if(juce::Desktop::getInstance().getMainMouseSource().canHover())
    {
        startTimer(25);
    }
}

Tooltip::BubbleWindow::~BubbleWindow()
{
    removeFromDesktop();
    setVisible(false);
}

void Tooltip::BubbleWindow::paint(juce::Graphics& g)
{
    getLookAndFeel().drawTooltip(g, mTooltip, getWidth(), getHeight());
}

void Tooltip::BubbleWindow::timerCallback()
{
    auto getTipFor = [](juce::Component* component) -> juce::String
    {
        if(component != nullptr && !component->isCurrentlyBlockedByAnotherModalComponent() &&  juce::Process::isForegroundProcess())
        {
            if(auto* client = dynamic_cast<BubbleClient*>(component))
            {
                return client->getTooltip();
            }
        }
        return {};
    };
    
    auto& desktop = juce::Desktop::getInstance();
    auto mouseSource = desktop.getMainMouseSource();
    mTooltip = getTipFor(mouseSource.isTouch() ? nullptr : mouseSource.getComponentUnderMouse());
    
    if(mouseSource.isDragging() || mTooltip.isEmpty())
    {
        if(isVisible())
        {
            mTooltip.clear();
            removeFromDesktop();
            setVisible(false);
        }
    }
    else
    {
        auto const mousePosition = mouseSource.getScreenPosition().toInt();
        auto const parentArea = desktop.getDisplays().getDisplayForPoint(mousePosition)->userArea;
        setBounds(getLookAndFeel().getTooltipBounds(mTooltip, mousePosition, parentArea));
        setVisible(true);
        addToDesktop(juce::ComponentPeer::windowHasDropShadow | juce::ComponentPeer::windowIsTemporary | juce::ComponentPeer::windowIgnoresKeyPresses | juce::ComponentPeer::windowIgnoresMouseClicks);
        repaint();
    }
}

ANALYSE_FILE_END
