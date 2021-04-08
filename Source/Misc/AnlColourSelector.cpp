#include "AnlColourSelector.h"

ANALYSE_FILE_BEGIN

ColourSelector::ColourSelector()
{
    addChangeListener(this);
}

ColourSelector::~ColourSelector()
{
    removeChangeListener(this);
}

void ColourSelector::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    juce::ignoreUnused(source);
    anlWeakAssert(source == this);
    if(onColourChanged != nullptr)
    {
        onColourChanged(getCurrentColour());
    }
}

ColourButton::ColourButton(juce::Colour const& colour, juce::String const& title)
: juce::ShapeButton("ColourButton", colour, colour.brighter(), colour.brighter())
, mTitle(title)
{
    setClickingTogglesState(false);
    setCurrentColour(colour, juce::NotificationType::dontSendNotification);
}

juce::Colour ColourButton::getCurrentColour() const
{
    return mColour;
}

void ColourButton::resized()
{
    juce::Path p;
    p.addRoundedRectangle(getLocalBounds().toFloat(), 2.0f);
    setShape(p, false, true, false);
}

void ColourButton::setTitle(juce::String const& title)
{
    mTitle = title;
}

void ColourButton::setCurrentColour(juce::Colour const& newColour, juce::NotificationType notificationType)
{
    if(newColour != mColour)
    {
        mColour = newColour;
        setColours(mColour, mColour.brighter(), mColour.brighter());
        repaint();
        
        if(notificationType == juce::NotificationType::sendNotificationAsync)
        {
            juce::WeakReference<juce::Component> target(this);
            juce::MessageManager::callAsync([=, this]
            {
                if(target.get() != nullptr && onColourChanged != nullptr)
                {
                    onColourChanged(getCurrentColour());
                }
            });
        }
        else if(notificationType != juce::NotificationType::dontSendNotification)
        {
            if(onColourChanged != nullptr)
            {
                onColourChanged(getCurrentColour());
            }
        }
    }
}

void ColourButton::clicked()
{
    ColourSelector colourSelector;
    colourSelector.setSize(400, 300);
    colourSelector.setCurrentColour(mColour, juce::NotificationType::dontSendNotification);
    colourSelector.onColourChanged = [&](juce::Colour const& colour)
    {
        setCurrentColour(colour, juce::NotificationType::sendNotificationSync);
    };
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = juce::translate(mTitle);
    options.content.setNonOwned(&colourSelector);
    options.componentToCentreAround = this;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.runModal();
}

ANALYSE_FILE_END
