#include "AnlTooltip.h"

ANALYSE_FILE_BEGIN

void Tooltip::Server::timerCallback()
{
    auto getTipFor = [](juce::Component* component) -> juce::String
    {
        if(juce::Process::isForegroundProcess())
        {
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

ANALYSE_FILE_END
