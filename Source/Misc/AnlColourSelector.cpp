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

ColourButton::ColourButton(juce::String const& title)
: juce::Button("ColourButton")
, mTitle(title)
{
    setClickingTogglesState(false);
}

juce::Colour ColourButton::getCurrentColour() const
{
    return mColour;
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

void ColourButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto constexpr cornerSize = 4.0f;
    auto constexpr thickness = 1.0f;
    auto const bounds = getLocalBounds().toFloat().reduced(thickness);
    g.setColour(mColour);
    g.fillRoundedRectangle(bounds, cornerSize);
    if(shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
    {
        g.setColour(findColour(ColourIds::borderOnColourId));
    }
    else
    {
        g.setColour(findColour(ColourIds::borderOffColourId));
    }
    g.drawRoundedRectangle(bounds, cornerSize, thickness);
}

ANALYSE_FILE_END
