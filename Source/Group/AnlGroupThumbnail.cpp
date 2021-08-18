#include "AnlGroupThumbnail.h"
#include "AnlGroupStrechableSection.h"

ANALYSE_FILE_BEGIN

Group::Thumbnail::Thumbnail(Director& director)
: mDirector(director)
{
    addAndMakeVisible(mPropertiesButton);
    mPropertiesButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(mStateButton);
    mStateButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(mExpandButton);
    mExpandButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(mDropdownButton);
    mDropdownButton.setWantsKeyboardFocus(false);

    mPropertiesButton.setTooltip(juce::translate("Show group or track properties"));
    mExpandButton.setTooltip(juce::translate("Expand the tracks"));
    mDropdownButton.setTooltip(juce::translate("Show group actions menu"));

    mExpandButton.onClick = [&]()
    {
        mAccessor.setAttr<AttrType::expanded>(!mAccessor.getAttr<AttrType::expanded>(), NotificationType::synchronous);
    };

    auto getPropertiesMenu = [&]()
    {
        juce::PopupMenu menu;
        menu.addItem(juce::translate("Show 'GROUPNAME' group properties").replace("GROUPNAME", mAccessor.getAttr<AttrType::name>()), [&]()
                     {
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
                menu.addItem(juce::translate("Show 'TRACKNAME' track properties").replace("TRACKNAME", trackName), [=]
                             {
                                 if(auto var = std::make_unique<juce::DynamicObject>())
                                 {
                                     auto const position = juce::Desktop::getInstance().getMousePosition();
                                     var->setProperty("x", position.x);
                                     var->setProperty("y", position.y - 40);
                                     trackAcsr->get().sendSignal(Track::SignalType::showProperties, var.release(), NotificationType::synchronous);
                                 }
                             });
            }
        }
        return menu;
    };

    mPropertiesButton.onClick = [=, this]()
    {
        auto menu = getPropertiesMenu();
        menu.showAt(&mPropertiesButton);
    };

    mDropdownButton.onClick = [=, this]()
    {
        juce::PopupMenu menu;
        if(!mPropertiesButton.isVisible())
        {
            menu = getPropertiesMenu();
        }
        auto addItem = [&](juce::Button& button)
        {
            if(!button.isVisible())
            {
                menu.addItem(button.getTooltip(), button.onClick);
            }
        };
        addItem(mExpandButton);
        menu.showAt(&mDropdownButton);
    };

    mListener.onAttrChanged = [&](Group::Accessor const& acsr, AttrType const attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::colour:
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
                lookAndFeelChanged();
            }
            break;
            case AttrType::layout:
            case AttrType::tracks:
            {
                resized();
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
                mPropertyPanel.showAt({x, y});
            }
            break;
        }
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
    bool useDropdown = false;
    auto bounds = getLocalBounds().withTrimmedLeft(getWidth() / 2 + 1);
    auto constexpr separator = 2;
    auto const size = bounds.getWidth() - separator;

    auto const hasTrack = !mAccessor.getAttr<AttrType::layout>().empty();
    auto layoutButton = [&](juce::Component& component)
    {
        useDropdown = bounds.getHeight() < size * 2;
        component.setVisible(!useDropdown);
        if(!useDropdown)
        {
            component.setBounds(bounds.removeFromBottom(size).reduced(separator));
        }
    };

    layoutButton(mExpandButton);
    mExpandButton.setEnabled(hasTrack);
    layoutButton(mStateButton);
    mStateButton.setEnabled(hasTrack);
    layoutButton(mPropertiesButton);
    mDropdownButton.setVisible(hasTrack && useDropdown);
    if(useDropdown)
    {
        mDropdownButton.setBounds(bounds.removeFromBottom(size).reduced(separator));
    }
}

void Group::Thumbnail::paint(juce::Graphics& g)
{
    auto constexpr separator = 2;
    auto constexpr rotation = -1.5707963268f;

    auto const width = getWidth() / 2;
    auto const height = getHeight();
    auto const bottom = height - 2 * separator;
    auto const size = height - 4 * separator;

    auto const focused = mAccessor.getAttr<AttrType::focused>();
    g.setColour(findColour(ColourIds::backgroundColourId).contrasting(focused ? 0.1f : 0.0f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 2.0f);
    g.setColour(findColour(focused ? Decorator::ColourIds::highlightedBorderColourId : Decorator::ColourIds::normalBorderColourId));
    g.drawVerticalLine(width, static_cast<float>(separator * 2), static_cast<float>(bottom));
    g.setColour(findColour(ColourIds::textColourId));
    g.addTransform(juce::AffineTransform::rotation(rotation, 0.0f, static_cast<float>(bottom)));
    g.drawFittedText(mAccessor.getAttr<AttrType::name>(), 0, bottom, size, width, juce::Justification::centredLeft, 1, 1.0f);
}

void Group::Thumbnail::lookAndFeelChanged()
{
    auto* laf = dynamic_cast<IconManager::LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(laf != nullptr);
    if(laf != nullptr)
    {
        laf->setButtonIcon(mDropdownButton, IconManager::IconType::menu);
        laf->setButtonIcon(mPropertiesButton, IconManager::IconType::properties);
        if(mAccessor.getAttr<AttrType::expanded>())
        {
            laf->setButtonIcon(mExpandButton, IconManager::IconType::shrink);
            mExpandButton.setTooltip(juce::translate("Shrink the tracks"));
        }
        else
        {
            laf->setButtonIcon(mExpandButton, IconManager::IconType::expand);
            mExpandButton.setTooltip(juce::translate("Expand the tracks"));
        }
    }
}

void Group::Thumbnail::parentHierarchyChanged()
{
    lookAndFeelChanged();
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
    anlWeakAssert(dragContainer != nullptr && parent != nullptr);
    if(dragContainer != nullptr && !dragContainer->isDragAndDropActive() && section != nullptr && parent != nullptr)
    {
        juce::Image snapshot(juce::Image::ARGB, section->getWidth(), section->getHeight(), true);
        juce::Graphics g(snapshot);
        g.beginTransparencyLayer(0.6f);
        section->paintEntireComponent(g, false);
        g.endTransparencyLayer();

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
                                     parent, snapshot, true, &p, &event.source);
    }
}

void Group::Thumbnail::mouseUp(juce::MouseEvent const& event)
{
    setMouseCursor(event.mods.isCtrlDown() ? juce::MouseCursor::CopyingCursor : juce::MouseCursor::DraggingHandCursor);
}

ANALYSE_FILE_END
