#include "AnlColourSelector.h"

ANALYSE_FILE_BEGIN

ColourSelector::ColourSelector()
: juce::ColourSelector((showAlphaChannel | showColourAtTop | editableColour | showSliders | showColourspace))
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
            juce::MessageManager::callAsync([=]
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

void ColourButton::mouseDown(juce::MouseEvent const& event)
{
    juce::Button::mouseDown(event);
    juce::ignoreUnused(event);
}

void ColourButton::mouseDrag(juce::MouseEvent const& event)
{
    if(!event.source.isLongPressOrDrag())
    {
        juce::Button::mouseDrag(event);
        return;
    }

    auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this);
    if(dragContainer != nullptr && !dragContainer->isDragAndDropActive())
    {
        juce::Image snapshot(juce::Image::ARGB, getWidth(), getHeight(), true);
        juce::Graphics g(snapshot);
        g.beginTransparencyLayer(0.6f);
        paintEntireComponent(g, false);
        g.endTransparencyLayer();

        auto const p = -event.getMouseDownPosition();
        dragContainer->startDragging({}, this, snapshot, true, &p, &event.source);
    }
}

void ColourButton::mouseUp(juce::MouseEvent const& event)
{
    if(!event.source.isLongPressOrDrag())
    {
        juce::Button::mouseUp(event);
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
    auto const colour = mDraggedColour != nullptr ? mDraggedColour->mColour : mColour;
    if(!colour.isTransparent())
    {
        g.setColour(colour);
        g.fillRoundedRectangle(bounds, cornerSize);
    }
    else
    {
        juce::Path p;
        p.addRoundedRectangle(bounds, cornerSize);
        g.saveState();
        g.reduceClipRegion(p);
        g.setColour(juce::Colours::white);
        for(int i = 0; i < getWidth() * 2; i += 4)
        {
            g.drawLine(static_cast<float>(i), 0.0f, 0.0f, static_cast<float>(i), 1.0f);
        }
        g.restoreState();
    }

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

bool ColourButton::isInterestedInDragSource(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    auto* source = dynamic_cast<ColourButton*>(dragSourceDetails.sourceComponent.get());
    if(source == nullptr || source == this)
    {
        return false;
    }
    return true;
}

void ColourButton::itemDragEnter(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    mDraggedColour = dynamic_cast<ColourButton*>(dragSourceDetails.sourceComponent.get());
    anlWeakAssert(mDraggedColour.getComponent() != nullptr);
    if(mDraggedColour.getComponent() == nullptr)
    {
        mDraggedColour = nullptr;
        return;
    }
    repaint();
}

void ColourButton::itemDragExit(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    juce::ignoreUnused(dragSourceDetails);
    mDraggedColour = nullptr;
    repaint();
}

void ColourButton::itemDropped(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    mDraggedColour = dynamic_cast<ColourButton*>(dragSourceDetails.sourceComponent.get());
    anlWeakAssert(mDraggedColour.getComponent() != nullptr);
    if(mDraggedColour == nullptr)
    {
        mDraggedColour = nullptr;
        return;
    }

    setCurrentColour(mDraggedColour->mColour, juce::NotificationType::sendNotificationSync);
    itemDragExit(dragSourceDetails);
}

ANALYSE_FILE_END
