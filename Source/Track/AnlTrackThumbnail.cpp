#include "AnlTrackThumbnail.h"
#include "AnlTrackSection.h"

ANALYSE_FILE_BEGIN

Track::Thumbnail::Thumbnail(Director& director, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAcsr)
: mDirector(director)
, mTimeZoomAccessor(timeZoomAccessor)
, mTransportAccessor(transportAcsr)
{
    addAndMakeVisible(mPropertiesButton);
    mPropertiesButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(mEditButton);
    mEditButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(mStateButton);
    mStateButton.setWantsKeyboardFocus(false);

    mEditButton.setTooltip(juce::translate("Edit the track results"));
    mEditButton.onClick = [&]()
    {
        auto var = std::make_unique<juce::DynamicObject>();
        if(var != nullptr)
        {
            auto const center = mEditButton.getScreenBounds().getCentre();
            var->setProperty("x", center.x);
            var->setProperty("y", center.y - 40);
            mAccessor.sendSignal(SignalType::showTable, var.release(), NotificationType::synchronous);
        }
    };

    mPropertiesButton.setTooltip(juce::translate("Show the track properties"));
    mPropertiesButton.onClick = [&]()
    {
        auto var = std::make_unique<juce::DynamicObject>();
        if(var != nullptr)
        {
            auto const center = mPropertiesButton.getScreenBounds().getCentre();
            var->setProperty("x", center.x);
            var->setProperty("y", center.y - 40);
            mAccessor.sendSignal(SignalType::showProperties, var.release(), NotificationType::synchronous);
        }
    };

    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::file:
            case AttrType::description:
            case AttrType::name:
            {
                setTooltip(Tools::getInfoTooltip(acsr));
                repaint();
            }
            break;
            case AttrType::focused:
            {
                repaint();
            }
            break;
            case AttrType::processing:
            case AttrType::warnings:
            {
                resized();
                repaint();
            }
            break;
            case AttrType::key:
            case AttrType::state:
            case AttrType::results:
            case AttrType::graphics:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::unit:
            case AttrType::channelsLayout:
            case AttrType::showInGroup:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::grid:
                break;
        }
    };

    mReceiver.onSignal = [&](Accessor const& acsr, SignalType signal, juce::var value)
    {
        juce::ignoreUnused(acsr);
        switch(signal)
        {
            case SignalType::showProperties:
            {
                auto const x = static_cast<int>(value.getProperty("x", 0.0));
                auto const y = static_cast<int>(value.getProperty("y", 0.0));
                mPropertyWindowContainer.showAt({x, y});
            }
            break;
            case SignalType::showTable:
            {
                auto const x = static_cast<int>(value.getProperty("x", 0.0));
                auto const y = static_cast<int>(value.getProperty("y", 0.0));
                mResultsWindowContainer.showAt({x, y});
            }
            break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.addReceiver(mReceiver);
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

Track::Thumbnail::~Thumbnail()
{
    mAccessor.removeReceiver(mReceiver);
    mAccessor.removeListener(mListener);
}

void Track::Thumbnail::resized()
{
    auto bounds = getLocalBounds();
    auto constexpr separator = 2;

    auto layoutButton = [&](juce::Component& component)
    {
        component.setVisible(bounds.getHeight() >= bounds.getWidth());
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromBottom(bounds.getWidth()).reduced(separator));
        }
    };

    if(mStateButton.isProcessingOrRendering() || mStateButton.hasWarning())
    {
        layoutButton(mStateButton);
    }
    else
    {
        mStateButton.setVisible(false);
    }
    layoutButton(mPropertiesButton);
    layoutButton(mEditButton);
}

void Track::Thumbnail::paint(juce::Graphics& g)
{
    auto constexpr separator = 2;
    auto constexpr rotation = -1.5707963268f;

    auto const focused = Tools::isSelected(mAccessor);
    g.setColour(findColour(ColourIds::backgroundColourId).contrasting(focused ? 0.1f : 0.0f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 2.0f);

    auto const width = getWidth();
    auto const height = getHeight();
    auto const numElements = static_cast<int>(mStateButton.isVisible()) + static_cast<int>(mEditButton.isVisible()) + static_cast<int>(mPropertiesButton.isVisible());
    auto const bottom = height - numElements * (width + separator);
    if(bottom <= 0)
    {
        return;
    }
    g.setColour(findColour(ColourIds::textColourId));
    g.addTransform(juce::AffineTransform::rotation(rotation, 0.0f, static_cast<float>(bottom)));
    g.drawFittedText(mAccessor.getAttr<AttrType::name>(), 0, bottom, bottom, width, juce::Justification::centredLeft, 1, 1.0f);
}

void Track::Thumbnail::mouseMove(juce::MouseEvent const& event)
{
    setMouseCursor(event.mods.isCtrlDown() ? juce::MouseCursor::CopyingCursor : juce::MouseCursor::DraggingHandCursor);
}

void Track::Thumbnail::mouseDown(juce::MouseEvent const& event)
{
    beginDragAutoRepeat(5);
    setMouseCursor(event.mods.isCtrlDown() ? juce::MouseCursor::CopyingCursor : juce::MouseCursor::DraggingHandCursor);
}

void Track::Thumbnail::mouseDrag(juce::MouseEvent const& event)
{
    if((event.eventTime - event.mouseDownTime).inMilliseconds() < static_cast<juce::int64>(200))
    {
        return;
    }
    setMouseCursor(event.mods.isCtrlDown() ? juce::MouseCursor::CopyingCursor : juce::MouseCursor::DraggingHandCursor);
    auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this);
    auto* parent = findParentComponentOfClass<Section>();
    anlWeakAssert(dragContainer != nullptr && parent != nullptr);
    if(dragContainer != nullptr && !dragContainer->isDragAndDropActive() && parent != nullptr)
    {
        auto const p = -event.getMouseDownPosition();
        dragContainer->startDragging(DraggableTable::createDescription(event, "Track", mAccessor.getAttr<AttrType::identifier>(), parent->getHeight()), parent, juce::ScaledImage{}, true, &p, &event.source);
    }
}

void Track::Thumbnail::mouseUp(juce::MouseEvent const& event)
{
    setMouseCursor(event.mods.isCtrlDown() ? juce::MouseCursor::CopyingCursor : juce::MouseCursor::DraggingHandCursor);
}

ANALYSE_FILE_END
