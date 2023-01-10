#include "AnlGroupThumbnail.h"
#include "AnlGroupStrechableSection.h"

ANALYSE_FILE_BEGIN

Group::Thumbnail::Thumbnail(Director& director)
: mDirector(director)
{
    addAndMakeVisible(mPropertiesButton);
    mPropertiesButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(mEditButton);
    mEditButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(mStateButton);
    mStateButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(mExpandButton);
    mExpandButton.setWantsKeyboardFocus(false);

    mPropertiesButton.setTooltip(juce::translate("Show group or track properties"));
    mEditButton.setTooltip(juce::translate("Show the track results"));
    mExpandButton.setTooltip(juce::translate("Expand the tracks"));

    mExpandButton.onClick = [&]()
    {
        mAccessor.setAttr<AttrType::expanded>(!mAccessor.getAttr<AttrType::expanded>(), NotificationType::synchronous);
    };

    mEditButton.onClick = [&]()
    {
        juce::PopupMenu menu;
        juce::WeakReference<juce::Component> safePointer(this);
        auto const layout = mAccessor.getAttr<AttrType::layout>();
        for(auto const& identifier : layout)
        {
            auto trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
            if(trackAcsr.has_value())
            {
                auto const trackName = trackAcsr->get().getAttr<Track::AttrType::name>();
                menu.addItem(juce::translate("Track results: TRACKNAME").replace("TRACKNAME", trackName), [=, this]
                             {
                                 if(safePointer.get() == nullptr)
                                 {
                                     return;
                                 }
                                 if(auto var = std::make_unique<juce::DynamicObject>())
                                 {
                                     auto const position = juce::Desktop::getInstance().getMousePosition();
                                     var->setProperty("x", position.x);
                                     var->setProperty("y", position.y - 40);
                                     auto trackAcsrRef = Tools::getTrackAcsr(mAccessor, identifier);
                                     if(trackAcsrRef.has_value())
                                     {
                                         trackAcsr->get().sendSignal(Track::SignalType::showTable, var.release(), NotificationType::synchronous);
                                     }
                                 }
                             });
            }
        }
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mEditButton));
    };

    mPropertiesButton.onClick = [this]()
    {
        juce::PopupMenu menu;
        juce::WeakReference<juce::Component> safePointer(this);
        menu.addItem(juce::translate("Group properties"), [=, this]()
                     {
                         if(safePointer.get() == nullptr)
                         {
                             return;
                         }
                         if(auto var = std::make_unique<juce::DynamicObject>())
                         {
                             auto const position = juce::Desktop::getInstance().getMousePosition();
                             var->setProperty("x", position.x);
                             var->setProperty("y", position.y - 40);
                             mAccessor.sendSignal(SignalType::showProperties, var.release(), NotificationType::synchronous);
                         }
                     });
        auto const layout = mAccessor.getAttr<AttrType::layout>();
        for(auto const& identifier : layout)
        {
            auto trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
            if(trackAcsr.has_value())
            {
                auto const trackName = trackAcsr->get().getAttr<Track::AttrType::name>();
                menu.addItem(juce::translate("Track properties: TRACKNAME").replace("TRACKNAME", trackName), [=, this]
                             {
                                 if(safePointer.get() == nullptr)
                                 {
                                     return;
                                 }
                                 if(auto var = std::make_unique<juce::DynamicObject>())
                                 {
                                     auto const position = juce::Desktop::getInstance().getMousePosition();
                                     var->setProperty("x", position.x);
                                     var->setProperty("y", position.y - 40);
                                     auto trackAcsrRef = Tools::getTrackAcsr(mAccessor, identifier);
                                     if(trackAcsrRef.has_value())
                                     {
                                         trackAcsr->get().sendSignal(Track::SignalType::showProperties, var.release(), NotificationType::synchronous);
                                     }
                                 }
                             });
            }
        }
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mPropertiesButton));
    };

    mListener.onAttrChanged = [&](Group::Accessor const& acsr, AttrType const attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::colour:
            case AttrType::zoomid:
                break;
            case AttrType::layout:
            case AttrType::tracks:
            {
                mExpandButton.setEnabled(!mAccessor.getAttr<AttrType::layout>().empty());
                mEditButton.setEnabled(!mAccessor.getAttr<AttrType::layout>().empty());
            }
            break;
            case AttrType::focused:
            {
                repaint();
            }
            break;
            case AttrType::name:
            {
                setTooltip(acsr.getAttr<AttrType::name>());
                repaint();
            }
            break;
            case AttrType::expanded:
            {
                if(mAccessor.getAttr<AttrType::expanded>())
                {
                    mExpandButton.setTypes(Icon::Type::shrink);
                    mExpandButton.setTooltip(juce::translate("Shrink the tracks"));
                }
                else
                {
                    mExpandButton.setTypes(Icon::Type::expand);
                    mExpandButton.setTooltip(juce::translate("Expand the tracks"));
                }
            }
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
        }
    };

    mStateButton.onStateChanged = [this]()
    {
        resized();
        repaint();
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.addReceiver(mReceiver);
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

Group::Thumbnail::~Thumbnail()
{
    mAccessor.removeReceiver(mReceiver);
    mAccessor.removeListener(mListener);
}

void Group::Thumbnail::resized()
{
    auto constexpr separator = 2;
    auto bounds = getLocalBounds();

    auto leftBounds = bounds.removeFromLeft(getWidth() / 2 - separator / 2);
    auto layoutButton = [&](juce::Rectangle<int>& lBounds, juce::Component& component)
    {
        component.setVisible(lBounds.getHeight() >= lBounds.getWidth());
        if(component.isVisible())
        {
            component.setBounds(lBounds.removeFromBottom(lBounds.getWidth()).reduced(separator));
        }
    };

    if(mStateButton.hasWarning() || mStateButton.isProcessingOrRendering())
    {
        layoutButton(leftBounds, mStateButton);
    }
    else
    {
        mStateButton.setVisible(false);
    }
    layoutButton(leftBounds, mPropertiesButton);
    layoutButton(leftBounds, mEditButton);

    auto rightBounds = bounds.withTrimmedLeft(separator);
    layoutButton(rightBounds, mExpandButton);
}

void Group::Thumbnail::paint(juce::Graphics& g)
{
    auto constexpr separator = 2;
    auto constexpr rotation = -1.5707963268f;

    auto const width = getWidth() / 2;
    auto const height = getHeight();

    auto const focused = Tools::isSelected(mAccessor);
    g.setColour(findColour(ColourIds::backgroundColourId).contrasting(focused ? 0.1f : 0.0f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 2.0f);
    g.setColour(findColour(focused ? Decorator::ColourIds::highlightedBorderColourId : Decorator::ColourIds::normalBorderColourId));
    g.drawVerticalLine(width, static_cast<float>(separator * 2), static_cast<float>(height - separator * 2));

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

void Group::Thumbnail::mouseMove(juce::MouseEvent const& event)
{
    setMouseCursor(event.mods.isCtrlDown() ? juce::MouseCursor::CopyingCursor : juce::MouseCursor::DraggingHandCursor);
}

void Group::Thumbnail::mouseDown(juce::MouseEvent const& event)
{
    beginDragAutoRepeat(5);
    setMouseCursor(event.mods.isCtrlDown() ? juce::MouseCursor::CopyingCursor : juce::MouseCursor::DraggingHandCursor);
}

void Group::Thumbnail::mouseDrag(juce::MouseEvent const& event)
{
    if((event.eventTime - event.mouseDownTime).inMilliseconds() < static_cast<juce::int64>(200))
    {
        return;
    }
    setMouseCursor(event.mods.isCtrlDown() ? juce::MouseCursor::CopyingCursor : juce::MouseCursor::DraggingHandCursor);
    auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this);
    auto* section = findParentComponentOfClass<Section>();
    auto* parent = findParentComponentOfClass<StrechableSection>();
    auto* laf = dynamic_cast<LookAndFeel*>(&getLookAndFeel());
    anlWeakAssert(dragContainer != nullptr && parent != nullptr && laf != nullptr);
    if(dragContainer != nullptr && laf != nullptr && !dragContainer->isDragAndDropActive() && section != nullptr && parent != nullptr)
    {
        auto const p = -event.getMouseDownPosition();
        auto const expanded = mAccessor.getAttr<AttrType::expanded>();
        dragContainer->startDragging(DraggableTable::createDescription(
                                         event, "Group", mAccessor.getAttr<AttrType::identifier>(), section->getHeight(), [=, this]()
                                         {
                                             if(expanded)
                                             {
                                                 mAccessor.setAttr<AttrType::expanded>(false, NotificationType::synchronous);
                                             }
                                         },
                                         [=, this]()
                                         {
                                             if(expanded)
                                             {
                                                 mAccessor.setAttr<AttrType::expanded>(true, NotificationType::synchronous);
                                             }
                                         }),
                                     parent, laf->createDragImage(*section), true, &p, &event.source);
    }
}

void Group::Thumbnail::mouseUp(juce::MouseEvent const& event)
{
    setMouseCursor(event.mods.isCtrlDown() ? juce::MouseCursor::CopyingCursor : juce::MouseCursor::DraggingHandCursor);
}

ANALYSE_FILE_END
