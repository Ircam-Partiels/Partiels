#include "AnlToolTip.h"

ANALYSE_FILE_BEGIN

ToolTipServer::ToolTipServer()
{
    if(juce::Desktop::getInstance().getMainMouseSource().canHover())
    {
        startTimer(123);
    }
}

void ToolTipServer::timerCallback()
{
    auto getTipFor = [](juce::Component* component) -> juce::String
    {
        if(juce::Process::isForegroundProcess() && !juce::ModifierKeys::getCurrentModifiers().isAnyMouseButtonDown())
        {
            if(auto* toolTipClient = dynamic_cast<juce::TooltipClient*>(component))
            {
                return toolTipClient->getTooltip();
            }
        }
        return {};
    };
    
    auto mouseSource = juce::Desktop::getInstance().getMainMouseSource();
    auto const newTip = getTipFor(mouseSource.isTouch() ? nullptr : mouseSource.getComponentUnderMouse());
    if(newTip != mTip)
    {
        mTip = newTip;
        if(onToolTipChanged != nullptr)
        {
            onToolTipChanged(mTip);
        }
    }
}

ToolTipDisplay::ToolTipDisplay()
{
    addAndMakeVisible(mLabel);
    mServer.onToolTipChanged = [&](juce::String const& tip)
    {
        mLabel.setText(tip, juce::NotificationType::dontSendNotification);
    };
}

void ToolTipDisplay::resized()
{
    mLabel.setBounds(getLocalBounds());
}

ANALYSE_FILE_END
