#include "AnlTrackThumbnail.h"
#include "AnlTrackSection.h"

ANALYSE_FILE_BEGIN

Track::Thumbnail::Thumbnail(Director& director)
: mDirector(director)
{
    addAndMakeVisible(mPropertiesButton);
    mPropertiesButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(mStateButton);
    mStateButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(mDropdownButton);
    mDropdownButton.setWantsKeyboardFocus(false);

    mPropertiesButton.setTooltip(juce::translate("Change the analysis properties"));
    mDropdownButton.setTooltip(juce::translate("Show group actions menu"));

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

    mDropdownButton.onClick = [&]()
    {
        juce::PopupMenu menu;
        auto addItem = [&](juce::Button& button)
        {
            if(!button.isVisible())
            {
                menu.addItem(button.getTooltip(), button.onClick);
            }
        };
        addItem(mPropertiesButton);
        menu.showAt(&mDropdownButton);
    };

    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::name:
            {
                setTooltip(acsr.getAttr<AttrType::name>());
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
            case AttrType::file:
            case AttrType::key:
            case AttrType::description:
            case AttrType::state:
            case AttrType::results:
            case AttrType::graphics:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::colours:
            case AttrType::channelsLayout:
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
                mPropertyPanel.showAt({x, y});
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
    bool useDropdown = false;
    auto bounds = getLocalBounds();
    auto constexpr separator = 2;
    auto const size = bounds.getWidth() - separator;

    auto layoutButton = [&](juce::Component& component)
    {
        useDropdown = bounds.getHeight() < size * 2;
        component.setVisible(!useDropdown);
        if(!useDropdown)
        {
            component.setBounds(bounds.removeFromBottom(size).reduced(separator));
        }
    };

    layoutButton(mStateButton);
    layoutButton(mPropertiesButton);
    mDropdownButton.setVisible(useDropdown);
    if(useDropdown)
    {
        mDropdownButton.setBounds(bounds.removeFromBottom(size).reduced(separator));
    }
}

void Track::Thumbnail::paint(juce::Graphics& g)
{
    auto constexpr separator = 2;
    auto constexpr rotation = -1.5707963268f;

    auto const focused = mAccessor.getAttr<AttrType::focused>();
    g.setColour(findColour(ColourIds::backgroundColourId).contrasting(focused ? 0.1f : 0.0f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 2.0f);

    auto const width = getWidth();
    auto const height = getHeight();
    auto const bottom = height - 2 * width + separator;
    auto const size = height - 2 * (width + separator);
    if(size <= 0)
    {
        return;
    }
    g.setColour(findColour(ColourIds::textColourId));
    g.addTransform(juce::AffineTransform::rotation(rotation, 0.0f, static_cast<float>(bottom)));
    g.drawFittedText(mAccessor.getAttr<AttrType::name>(), 0, bottom, size, width, juce::Justification::centredLeft, 1, 1.0f);
}

void Track::Thumbnail::lookAndFeelChanged()
{
    auto* laf = dynamic_cast<IconManager::LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(laf != nullptr);
    if(laf != nullptr)
    {
        laf->setButtonIcon(mDropdownButton, IconManager::IconType::chevron);
        laf->setButtonIcon(mPropertiesButton, IconManager::IconType::properties);
    }
}

void Track::Thumbnail::parentHierarchyChanged()
{
    lookAndFeelChanged();
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
        juce::Image snapshot(juce::Image::ARGB, parent->getWidth(), parent->getHeight(), true);
        juce::Graphics g(snapshot);
        g.beginTransparencyLayer(0.6f);
        parent->paintEntireComponent(g, false);
        g.endTransparencyLayer();

        auto const p = -event.getMouseDownPosition();
        dragContainer->startDragging(DraggableTable::createDescription(event, "Track", mAccessor.getAttr<AttrType::identifier>(), parent->getHeight()), parent, snapshot, true, &p, &event.source);
    }
}

void Track::Thumbnail::mouseUp(juce::MouseEvent const& event)
{
    setMouseCursor(event.mods.isCtrlDown() ? juce::MouseCursor::CopyingCursor : juce::MouseCursor::DraggingHandCursor);
}

ANALYSE_FILE_END
