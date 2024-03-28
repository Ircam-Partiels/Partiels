#include "AnlDocumentSection.h"
#include "AnlDocumentAudioReader.h"
#include "AnlDocumentTools.h"
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

void Document::Section::Viewport::visibleAreaChanged(juce::Rectangle<int> const& newVisibleArea)
{
    if(onVisibleAreaChanged != nullptr)
    {
        onVisibleAreaChanged(newVisibleArea);
    }
}

void Document::Section::Viewport::mouseWheelMove(juce::MouseEvent const& e, juce::MouseWheelDetails const& wheel)
{
    useMouseWheelMoveIfNeeded(e, wheel);
}

Document::Section::Section(Director& director, juce::ApplicationCommandManager& commandManager)
: CommandTarget(director, commandManager)
, mDirector(director)
, mApplicationCommandManager(commandManager)
, mHeader(director, commandManager)
, mGridButton(juce::ImageCache::getFromMemory(AnlIconsData::griddisabled_png, AnlIconsData::griddisabled_pngSize))
, mExpandLayoutButton(juce::ImageCache::getFromMemory(AnlIconsData::expand_png, AnlIconsData::expand_pngSize), juce::ImageCache::getFromMemory(AnlIconsData::shrink_png, AnlIconsData::shrink_pngSize))
, mResizeLayoutButton(juce::ImageCache::getFromMemory(AnlIconsData::unlocksize_png, AnlIconsData::unlocksize_pngSize), juce::ImageCache::getFromMemory(AnlIconsData::locksize_png, AnlIconsData::locksize_pngSize))
, mMagnetizeButton(juce::ImageCache::getFromMemory(AnlIconsData::magnet_png, AnlIconsData::magnet_pngSize))
, mTimeRuler(mAccessor.getAcsr<AcsrType::timeZoom>(), Zoom::Ruler::Orientation::horizontal, [](double value)
             {
                 return Format::secondsToString(value, {":", ":", ":", ""});
             })
, mLayoutNotifier(typeid(*this).name(), mAccessor, [this]()
                  {
                      updateLayout();
                  })
, mExpandedNotifier(typeid(*this).name(), mAccessor, [this]()
                    {
                        updateExpandState();
                    },
                    {Group::AttrType::expanded})
, mFocusNotifier(typeid(*this).name(), mAccessor, [this]()
                 {
                     updateFocus();
                 },
                 {Group::AttrType::focused}, {Track::AttrType::focused})
{
    mTimeRuler.onDoubleClick = [this]([[maybe_unused]] juce::MouseEvent const& event)
    {
        auto& acsr = mAccessor.getAcsr<AcsrType::timeZoom>();
        acsr.setAttr<Zoom::AttrType::visibleRange>(acsr.getAttr<Zoom::AttrType::globalRange>(), NotificationType::synchronous);
    };
    mTimeRuler.onMouseDown = [this](juce::MouseEvent const& event)
    {
        if(event.mods.isPopupMenu())
        {
            auto timeRangeEditor = Tools::createTimeRangeEditor(mAccessor);
            if(timeRangeEditor == nullptr)
            {
                return false;
            }

            auto const x = juce::Desktop::getMousePosition().getX();
            auto const timeRulerBounds = mTimeRuler.getScreenBounds();
            auto const height = timeRulerBounds.getHeight();
            auto const bounds = timeRulerBounds.withX(x - height / 2).withWidth(height);
            auto& box = juce::CallOutBox::launchAsynchronously(std::move(timeRangeEditor), bounds, nullptr);
            box.setLookAndFeel(std::addressof(getLookAndFeel()));
            box.setArrowSize(0.0f);
            return false;
        }
        return true;
    };

    mDraggableTable.onComponentDropped = [&](juce::String const& identifier, size_t index, bool copy)
    {
        if(copy)
        {
            anlWeakAssert(Tools::hasGroupAcsr(mAccessor, identifier));
            if(!Tools::hasGroupAcsr(mAccessor, identifier))
            {
                return;
            }
            mDirector.startAction();
            auto const references = mDirector.sanitize(NotificationType::synchronous);
            auto const existingGroupIdentifier = references.count(identifier) > 0_z ? references.at(identifier) : identifier;
            anlWeakAssert(Tools::hasGroupAcsr(mAccessor, existingGroupIdentifier));
            if(!Tools::hasGroupAcsr(mAccessor, existingGroupIdentifier))
            {
                return;
            }
            auto const newGroupIdentifier = mDirector.addGroup(index, NotificationType::synchronous);
            if(!newGroupIdentifier.has_value())
            {
                mDirector.endAction(ActionState::abort);
            }

            auto const& groupAcsr = Tools::getGroupAcsr(mAccessor, existingGroupIdentifier);
            Group::Accessor copyAcsr;
            copyAcsr.copyFrom(groupAcsr, NotificationType::synchronous);
            copyAcsr.setAttr<Group::AttrType::identifier>(newGroupIdentifier.value(), NotificationType::synchronous);
            copyAcsr.setAttr<Group::AttrType::layout>(std::vector<juce::String>(), NotificationType::synchronous);
            auto& newGroupAcsr = Tools::getGroupAcsr(mAccessor, newGroupIdentifier.value());
            newGroupAcsr.copyFrom(copyAcsr, NotificationType::synchronous);

            auto const trackIdentifiers = groupAcsr.getAttr<Group::AttrType::layout>();
            for(size_t i = 0; i < trackIdentifiers.size(); ++i)
            {
                if(!mDirector.copyTrack(newGroupIdentifier.value(), i, trackIdentifiers.at(i), NotificationType::synchronous).has_value())
                {
                    mDirector.endAction(ActionState::abort);
                    return;
                }
            }
            [[maybe_unused]] auto const newReferences = mDirector.sanitize(NotificationType::synchronous);
            mDirector.endAction(ActionState::newTransaction, juce::translate("Copy Group"));
        }
        else
        {
            mDirector.startAction();
            auto layout = copy_with_erased(mAccessor.getAttr<AttrType::layout>(), identifier);
            layout.insert(layout.begin() + static_cast<long>(index), identifier);
            mAccessor.setAttr<AttrType::layout>(layout, NotificationType::synchronous);
            [[maybe_unused]] auto const newReferences = mDirector.sanitize(NotificationType::synchronous);
            mDirector.endAction(ActionState::newTransaction, juce::translate("Move Group"));
        }
    };

    setWantsKeyboardFocus(true);
    mViewport.setViewedComponent(&mDraggableTable, false);
    mViewport.setScrollBarsShown(true, false, false, false);

    addAndMakeVisible(mHeader);

    addAndMakeVisible(mExpandLayoutButton);
    mExpandLayoutButton.setClickingTogglesState(false);
    mExpandLayoutButton.setToggleable(true);
    mExpandLayoutButton.setTooltip(juce::translate("Shrink of expand the tracks of all the groups"));
    mExpandLayoutButton.onClick = [this]()
    {
        auto groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
        auto const expanded = std::any_of(groupAcsrs.cbegin(), groupAcsrs.cend(), [](auto const groupAcsr)
                                          {
                                              return groupAcsr.get().template getAttr<Group::AttrType::expanded>();
                                          });

        for(auto groupAcsr : groupAcsrs)
        {
            groupAcsr.get().setAttr<Group::AttrType::expanded>(!expanded, NotificationType::synchronous);
        }
    };

    addAndMakeVisible(mResizeLayoutButton);
    mResizeLayoutButton.setClickingTogglesState(false);
    mResizeLayoutButton.setToggleable(true);
    mResizeLayoutButton.setTooltip(juce::translate("Toggle the optimization of the height of groups and tracks to fit the height of the document"));
    mResizeLayoutButton.onClick = [this]()
    {
        if(mResizeLayoutButton.getModifierKeys().isShiftDown())
        {
            if(mViewport.getHeight() <= 0)
            {
                return;
            }
            Tools::resizeItems(mAccessor, false, mViewport.getHeight());
        }
        else
        {
            auto const autoresize = mAccessor.getAttr<AttrType::autoresize>();
            mAccessor.setAttr<AttrType::autoresize>(!autoresize, NotificationType::synchronous);
        }
    };

    addAndMakeVisible(mGridButton);
    mGridButton.setClickingTogglesState(false);
    mGridButton.setToggleable(true);
    mGridButton.setTooltip(juce::translate("Change the mode of the grid"));
    mGridButton.onClick = [this]()
    {
        auto const mode = mAccessor.getAttr<AttrType::grid>();
        switch(mode)
        {
            case GridMode::hidden:
            {
                mAccessor.setAttr<AttrType::grid>(GridMode::partial, NotificationType::synchronous);
            }
            break;
            case GridMode::partial:
            {
                mAccessor.setAttr<AttrType::grid>(GridMode::full, NotificationType::synchronous);
            }
            break;
            case GridMode::full:
            {
                mAccessor.setAttr<AttrType::grid>(GridMode::hidden, NotificationType::synchronous);
            }
            break;
        }
    };

    addAndMakeVisible(mMagnetizeButton);
    mMagnetizeButton.setClickingTogglesState(true);
    mMagnetizeButton.setTooltip(juce::translate("Toggle the magnetism mechanism with markers"));
    mMagnetizeButton.onClick = [this]()
    {
        auto& transportAcsr = mAccessor.getAcsr<AcsrType::transport>();
        transportAcsr.setAttr<Transport::AttrType::magnetize>(mMagnetizeButton.getToggleState(), NotificationType::synchronous);
    };

    mAddGroupButton.setButtonText("+");
    mAddGroupButton.setWantsKeyboardFocus(false);
    mAddGroupButton.setMouseClickGrabsKeyboardFocus(false);
    mAddGroupButton.setCommandToTrigger(std::addressof(mApplicationCommandManager), ApplicationCommandIDs::editNewGroup, true);

    addAndMakeVisible(pluginListButton);
    addAndMakeVisible(mTimeRulerDecoration);
    addAndMakeVisible(mLoopBarDecoration);
    addAndMakeVisible(mTopSeparator);
    addAndMakeVisible(mViewport);
    addAndMakeVisible(mBottomSeparator);
    addAndMakeVisible(mTimeScrollBar);
    addAndMakeVisible(mAddGroupButton);
    setSize(480, 200);

    mTransportListener.onAttrChanged = [&](Transport::Accessor const& acsr, Transport::AttrType attribute)
    {
        switch(attribute)
        {
            case Transport::AttrType::playback:
            case Transport::AttrType::startPlayhead:
            case Transport::AttrType::runningPlayhead:
            case Transport::AttrType::looping:
            case Transport::AttrType::loopRange:
            case Transport::AttrType::stopAtLoopEnd:
            case Transport::AttrType::gain:
            case Transport::AttrType::autoScroll:
            case Transport::AttrType::selection:
                break;
            case Transport::AttrType::markers:
            case Transport::AttrType::magnetize:
            {
                auto const enabled = !acsr.getAttr<Transport::AttrType::markers>().empty();
                mMagnetizeButton.setEnabled(enabled);
                mMagnetizeButton.setToggleState(enabled && acsr.getAttr<Transport::AttrType::magnetize>(), juce::NotificationType::dontSendNotification);
            }
            break;
        }
    };

    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::grid:
            {
                switch(mAccessor.getAttr<AttrType::grid>())
                {
                    case GridMode::hidden:
                    {
                        mGridButton.setToggleState(false, juce::NotificationType::dontSendNotification);
                        mGridButton.setImages(juce::ImageCache::getFromMemory(AnlIconsData::griddisabled_png, AnlIconsData::griddisabled_pngSize));
                    }
                    break;
                    case GridMode::partial:
                    {
                        mGridButton.setToggleState(true, juce::NotificationType::dontSendNotification);
                        mGridButton.setImages(juce::ImageCache::getFromMemory(AnlIconsData::gridpartial_png, AnlIconsData::gridpartial_pngSize));
                    }
                    break;
                    case GridMode::full:
                    {
                        mGridButton.setToggleState(true, juce::NotificationType::dontSendNotification);
                        mGridButton.setImages(juce::ImageCache::getFromMemory(AnlIconsData::gridfull_png, AnlIconsData::gridfull_pngSize));
                    }
                    break;
                }
            }
            break;
            case AttrType::autoresize:
            {
                auto const autoresize = acsr.getAttr<AttrType::autoresize>();
                mResizeLayoutButton.setToggleState(autoresize, juce::NotificationType::dontSendNotification);
                triggerAsyncUpdate();
                updateAutoresize();
            }
            break;
            case AttrType::reader:
            case AttrType::path:
            case AttrType::layout:
            case AttrType::viewport:
            case AttrType::samplerate:
            case AttrType::channels:
            case AttrType::editMode:
            case AttrType::drawingState:
                break;
        }
    };

    mListener.onAccessorInserted = [this]([[maybe_unused]] Accessor const& acsr, AcsrType type, [[maybe_unused]] size_t index)
    {
        switch(type)
        {
            case AcsrType::groups:
            case AcsrType::tracks:
            {
                updateExpandState();
                updateLayout();
            }
            break;
            case AcsrType::timeZoom:
            case AcsrType::transport:
                break;
        }
    };
    mListener.onAccessorErased = mListener.onAccessorInserted;

    mReceiver.onSignal = [&]([[maybe_unused]] Accessor const& acsr, SignalType signal, juce::var value)
    {
        switch(signal)
        {
            case SignalType::viewport:
            {
                auto const x = static_cast<int>(value.getProperty("x", mViewport.getViewPositionX()));
                auto const y = static_cast<int>(value.getProperty("y", mViewport.getViewPositionY()));
                mViewport.setViewPosition(x, y);
                break;
            }
            case SignalType::isLoading:
            {
                mIsLoading = static_cast<bool>(value);
                for(auto& section : mGroupSections)
                {
                    section.second->setCanAnimate(!mIsLoading);
                }
                break;
            }
            case SignalType::showReaderLayoutPanel:
                break;
        }
    };

    mViewport.onVisibleAreaChanged = [this](juce::Rectangle<int> const& area)
    {
        if(!mIsLoading)
        {
            mAccessor.setAttr<AttrType::viewport>(area.getTopLeft(), NotificationType::synchronous);
        }
    };

    mAccessor.getAcsr<AcsrType::transport>().addListener(mTransportListener, NotificationType::synchronous);
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.addReceiver(mReceiver);
    mApplicationCommandManager.addListener(this);
    applicationCommandListChanged();
    addKeyListener(mCommandManager.getKeyMappings());
}

Document::Section::~Section()
{
    removeKeyListener(mCommandManager.getKeyMappings());
    mApplicationCommandManager.removeListener(this);
    mAccessor.removeReceiver(mReceiver);
    mAccessor.removeListener(mListener);
    mAccessor.getAcsr<AcsrType::transport>().removeListener(mTransportListener);
}

juce::BorderSize<int> Document::Section::getMainSectionBorderSize()
{
    return {48 + 28 + 1, 0, 14 + 1, 0};
}

void Document::Section::resized()
{
    auto const scrollbarWidth = mViewport.getScrollBarThickness();
    auto bounds = getLocalBounds();
    mHeader.setBounds(bounds.removeFromTop(48));

    {
        auto topPart = bounds.removeFromTop(28);
        mGridButton.setBounds(topPart.removeFromLeft(28).withSizeKeepingCentre(20, 20));
        mExpandLayoutButton.setBounds(topPart.removeFromLeft(28).withSizeKeepingCentre(20, 20));
        mResizeLayoutButton.setBounds(topPart.removeFromLeft(28).withSizeKeepingCentre(20, 20));
        mMagnetizeButton.setBounds(topPart.removeFromRight(24 + scrollbarWidth).withSizeKeepingCentre(20, 20));
        mTimeRulerDecoration.setBounds(topPart.removeFromTop(14));
        mLoopBarDecoration.setBounds(topPart);
        mTopSeparator.setBounds(bounds.removeFromTop(1));
    }

    {
        auto bottomPart = bounds.removeFromBottom(14).reduced(0, 1);
        mAddGroupButton.setBounds(bottomPart.removeFromLeft(48));
        bottomPart = bottomPart.withTrimmedLeft(36);
        pluginListButton.setBounds(bottomPart.removeFromRight(24 + scrollbarWidth));
        mTimeScrollBar.setBounds(bottomPart);
        mBottomSeparator.setBounds(bounds.removeFromBottom(1));
    }

    mViewport.setBounds(bounds);
    mDraggableTable.setBounds(0, 0, bounds.getWidth() - scrollbarWidth, mDraggableTable.getHeight());
    handleAsyncUpdate();
}

void Document::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
}

Document::Selection::Item Document::Section::getSelectionItem(juce::Component* component, juce::MouseEvent const& event) const
{
    if(auto* trackSection = Utils::findComponentOfClass<Track::Section>(component))
    {
        auto const identifier = trackSection->getIdentifier();
        MiscWeakAssert(Tools::hasTrackAcsr(mAccessor, identifier));
        if(!Tools::hasTrackAcsr(mAccessor, identifier))
        {
            return {};
        }
        auto const& trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
        auto const point = trackSection == component ? event.position.toInt() : event.getEventRelativeTo(trackSection).position.toInt();
        if(point.x <= (Track::Section::getThumbnailOffset() + Track::Section::getThumbnailWidth()))
        {
            return {identifier, {}};
        }
        auto const channel = Track::Tools::getChannel(trackAcsr, trackSection->getLocalBounds(), point.y, true);
        return {identifier, channel};
    }
    if(auto* groupSection = Utils::findComponentOfClass<Group::Section>(component))
    {
        auto const identifier = groupSection->getIdentifier();
        MiscWeakAssert(Tools::hasGroupAcsr(mAccessor, identifier));
        if(!Tools::hasGroupAcsr(mAccessor, identifier))
        {
            return {};
        }
        auto const& groupAcsr = Tools::getGroupAcsr(mAccessor, identifier);
        auto const point = groupSection == component ? event.position.toInt() : event.getEventRelativeTo(groupSection).position.toInt();
        if(point.x <= (Track::Section::getThumbnailOffset() + Track::Section::getThumbnailWidth()))
        {
            return {identifier, {}};
        }
        auto const channel = Group::Tools::getChannel(groupAcsr, groupSection->getLocalBounds(), point.y, true);
        return {identifier, channel};
    }
    return {};
}

void Document::Section::mouseDown(juce::MouseEvent const& event)
{
    if(isDragAndDropActive())
    {
        return;
    }
    auto const item = getSelectionItem(event.eventComponent, event);
    if(item.identifier.isEmpty())
    {
        return;
    }

    mAccessor.setAttr<AttrType::editMode>(item.channel.has_value() ? EditMode::frames : EditMode::items, NotificationType::synchronous);
    if(event.mods.isShiftDown() && mLastSelectedItem.identifier.isNotEmpty())
    {
        Selection::selectItems(mAccessor, mLastSelectedItem, item, true, false, NotificationType::synchronous);
    }
    else if(!event.mods.isCommandDown())
    {
        mLastSelectedItem = item;
        auto const deselectAll = !event.mods.isAltDown();
        auto const flipSelection = event.mods.isAltDown();
        Selection::selectItem(mAccessor, item, deselectAll, flipSelection, NotificationType::synchronous);
    }
}

void Document::Section::mouseDrag(juce::MouseEvent const& event)
{
    if(event.mods.isShiftDown() || event.mods.isCommandDown() || isDragAndDropActive() || mAccessor.getAttr<AttrType::editMode>() == EditMode::items)
    {
        return;
    }
    auto const item = getSelectionItem(juce::Desktop::getInstance().findComponentAt(event.getScreenPosition()), event);
    if(item.identifier.isEmpty() || !item.channel.has_value())
    {
        return;
    }
    Selection::selectItems(mAccessor, mLastSelectedItem, item, true, false, NotificationType::synchronous);
}

void Document::Section::updateExpandState()
{
    auto const groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
    mExpandLayoutButton.setEnabled(!groupAcsrs.empty());
    auto const shrinked = std::none_of(groupAcsrs.cbegin(), groupAcsrs.cend(), [](auto const groupAcsr)
                                       {
                                           return groupAcsr.get().template getAttr<Group::AttrType::expanded>();
                                       });
    mExpandLayoutButton.setToggleState(shrinked, juce::NotificationType::dontSendNotification);
}

void Document::Section::updateFocus()
{
    auto const closest = Selection::getClosestItem(mAccessor);
    MiscWeakAssert(!closest.has_value() || Tools::hasItem(mAccessor, *closest));
    if(!closest.has_value() || !Tools::hasItem(mAccessor, *closest))
    {
        return;
    }
    auto const farthest = Selection::getFarthestItem(mAccessor);
    MiscWeakAssert(farthest.has_value() && Tools::hasItem(mAccessor, *farthest));
    if(!farthest.has_value() || !Tools::hasItem(mAccessor, *farthest))
    {
        return;
    }

    auto const getGroupIdentifier = [&](juce::String const& identifier) -> juce::String
    {
        if(Tools::hasGroupAcsr(mAccessor, identifier))
        {
            return identifier;
        }
        if(Tools::hasTrackAcsr(mAccessor, identifier))
        {
            auto const& groupAcsr = Tools::getGroupAcsrForTrack(mAccessor, identifier);
            return groupAcsr.getAttr<Group::AttrType::identifier>();
        }
        MiscWeakAssert(false);
        return "";
    };

    auto const closestGroupId = getGroupIdentifier(*closest);
    MiscWeakAssert(mGroupSections.count(closestGroupId) > 0_z);
    if(mGroupSections.count(closestGroupId) == 0_z)
    {
        return;
    }

    auto const farthestGroupId = getGroupIdentifier(*farthest);
    MiscWeakAssert(mGroupSections.count(farthestGroupId) > 0_z);
    if(mGroupSections.count(farthestGroupId) == 0_z)
    {
        return;
    }

    auto const& closestSection = mGroupSections[closestGroupId]->getSection(*closest);
    auto const closestRange = mDraggableTable.getLocalArea(&closestSection, closestSection.getLocalBounds()).getVerticalRange();
    auto const& farthestSection = mGroupSections[closestGroupId]->getSection(*closest);
    auto const farthestRange = mDraggableTable.getLocalArea(&farthestSection, farthestSection.getLocalBounds()).getVerticalRange();

    auto const combinedRange = closestRange.getUnionWith(farthestRange);
    auto const viewRange = mViewport.getViewArea().getVerticalRange();

    if(viewRange.intersects(combinedRange))
    {
        return;
    }
    else if(viewRange.getStart() > combinedRange.getStart())
    {
        mViewport.setViewPosition({mViewport.getViewArea().getX(), combinedRange.getStart() - 8});
    }
    else if(viewRange.getEnd() < combinedRange.getEnd())
    {
        mViewport.setViewPosition({mViewport.getViewArea().getX(), viewRange.movedToEndAt(combinedRange.getEnd()).getStart() + 8});
    }
}

juce::Rectangle<int> Document::Section::getPlotBounds(juce::String const& identifier) const
{
    auto getGroupIdentifier = [&]()
    {
        if(Tools::hasTrackAcsr(mAccessor, identifier) && Tools::isTrackInGroup(mAccessor, identifier))
        {
            auto const& groupAcsr = Tools::getGroupAcsrForTrack(mAccessor, identifier);
            return groupAcsr.getAttr<Group::AttrType::identifier>();
        }
        return identifier;
    };

    auto it = mGroupSections.find(getGroupIdentifier());
    anlWeakAssert(it != mGroupSections.end());
    if(it != mGroupSections.end())
    {
        return it->second.get()->getPlotBounds(identifier);
    }
    return {};
}

void Document::Section::updateLayout()
{
    auto const createGroupSection = [&](juce::String const& identifier)
    {
        auto& groupDirector = mDirector.getGroupDirector(identifier);
        auto& transportAcsr = mAccessor.getAcsr<AcsrType::transport>();
        auto& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
        auto groupSection = std::make_unique<Group::StretchableSection>(groupDirector, mApplicationCommandManager, transportAcsr, timeZoomAcsr, [this](juce::String const& itemIdentifier, int newHeight)
                                                                        {
                                                                            Tools::resizeItem(mAccessor, itemIdentifier, newHeight, mViewport.getHeight());
                                                                        });
        if(groupSection != nullptr)
        {
            groupSection->addMouseListener(this, true);
            groupSection->onTrackInserted = [&](juce::String const& trackIdentifier, size_t index, bool copy)
            {
                if(copy)
                {
                    copyTrackToGroup(groupDirector, index, trackIdentifier);
                }
                else
                {
                    moveTrackToGroup(groupDirector, index, trackIdentifier);
                }
            };
            groupSection->onLayoutChanged = [this]()
            {
                handleAsyncUpdate();
            };
        }
        return groupSection;
    };

    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    auto const groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
    auto it = mGroupSections.begin();
    while(it != mGroupSections.end())
    {
        if(std::none_of(layout.cbegin(), layout.cend(), [&](auto const& identifer)
                        {
                            return identifer == it->first;
                        }) ||
           std::none_of(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](auto const& groupAcsr)
                        {
                            return groupAcsr.get().template getAttr<Group::AttrType::identifier>() == it->first;
                        }))
        {
            it = mGroupSections.erase(it);
        }
        else
        {
            ++it;
        }
    }

    std::vector<ConcertinaTable::ComponentRef> components;
    components.reserve(layout.size());
    for(auto const& identifier : layout)
    {
        auto sectionIt = mGroupSections.find(identifier);
        if(sectionIt == mGroupSections.cend())
        {
            // This is necessary because the layout can be initialized before the group accessors
            if(Tools::hasGroupAcsr(mAccessor, identifier))
            {
                auto groupSection = createGroupSection(identifier);
                anlStrongAssert(groupSection != nullptr);
                if(groupSection != nullptr)
                {
                    auto const result = mGroupSections.emplace(identifier, std::move(groupSection));
                    anlStrongAssert(result.second);
                    if(result.second)
                    {
                        components.push_back(*result.first->second.get());
                    }
                }
            }
        }
        else
        {
            components.push_back(*sectionIt->second.get());
        }
    }

    mDraggableTable.setComponents(components);
    auto const scrollbarWidth = mViewport.getScrollBarThickness();
    mDraggableTable.setBounds(0, 0, mViewport.getWidth() - scrollbarWidth, mDraggableTable.getHeight());

    updateExpandState();
    updateAutoresize();
    handleAsyncUpdate();
}

void Document::Section::updateAutoresize()
{
    auto const autoresize = mAccessor.getAttr<AttrType::autoresize>();
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    for(auto const& identifier : layout)
    {
        auto sectionIt = mGroupSections.find(identifier);
        if(sectionIt != mGroupSections.cend() && sectionIt->second != nullptr)
        {
            sectionIt->second->setLastItemResizable(true);
        }
    }
    if(!layout.empty())
    {
        auto sectionIt = mGroupSections.find(layout.back());
        if(sectionIt != mGroupSections.cend() && sectionIt->second != nullptr)
        {
            sectionIt->second->setLastItemResizable(!autoresize);
        }
    }
}

void Document::Section::moveTrackToGroup(Group::Director& groupDirector, size_t index, juce::String const& trackIdentifier)
{
    anlStrongAssert(Tools::hasTrackAcsr(mAccessor, trackIdentifier));
    if(!Tools::hasTrackAcsr(mAccessor, trackIdentifier))
    {
        return;
    }

    mDirector.startAction();
    auto& groupAcsr = groupDirector.getAccessor();
    auto const groupIdentifier = groupAcsr.getAttr<Group::AttrType::identifier>();
    if(mDirector.moveTrack(groupIdentifier, index, trackIdentifier, NotificationType::synchronous))
    {
        auto const references = mDirector.sanitize(NotificationType::synchronous);
        auto const sanitizedTrackIdentifier = references.count(trackIdentifier) > 0_z ? references.at(trackIdentifier) : trackIdentifier;
        mDirector.endAction(ActionState::newTransaction, juce::translate("Move Track"));
        mLastSelectedItem = {sanitizedTrackIdentifier, {}};
        Selection::selectItem(mAccessor, {sanitizedTrackIdentifier, {}}, true, false, NotificationType::synchronous);
    }
    else
    {
        mDirector.endAction(ActionState::abort);
    }
}

void Document::Section::copyTrackToGroup(Group::Director& groupDirector, size_t index, juce::String const& trackIdentifier)
{
    anlStrongAssert(Tools::hasTrackAcsr(mAccessor, trackIdentifier));
    if(!Tools::hasTrackAcsr(mAccessor, trackIdentifier))
    {
        return;
    }

    mDirector.startAction();
    auto& groupAcsr = groupDirector.getAccessor();
    auto const groupIdentifier = groupAcsr.getAttr<Group::AttrType::identifier>();
    auto const newTrackIdentifier = mDirector.copyTrack(groupIdentifier, index, trackIdentifier, NotificationType::synchronous);
    if(newTrackIdentifier.has_value())
    {
        auto const references = mDirector.sanitize(NotificationType::synchronous);
        auto const sanitizedTrackIdentifier = references.count(newTrackIdentifier.value()) > 0_z ? references.at(newTrackIdentifier.value()) : newTrackIdentifier.value();
        mDirector.endAction(ActionState::newTransaction, juce::translate("Copy Track"));
        mLastSelectedItem = {sanitizedTrackIdentifier, {}};
        Selection::selectItem(mAccessor, {sanitizedTrackIdentifier, {}}, true, false, NotificationType::synchronous);
    }
    else
    {
        mDirector.endAction(ActionState::abort);
    }
}

void Document::Section::dragOperationEnded([[maybe_unused]] juce::DragAndDropTarget::SourceDetails const& details)
{
    handleAsyncUpdate();
}

juce::ApplicationCommandTarget* Document::Section::getNextCommandTarget()
{
    return findFirstTargetParentComponent();
}

void Document::Section::applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info)
{
    switch(info.commandID)
    {
        case ApplicationCommandIDs::viewInfoBubble:
        {
            if(info.commandFlags & juce::ApplicationCommandInfo::CommandFlags::isTicked)
            {
                mToolTipBubbleWindow.startTimer(23);
            }
            else
            {
                mToolTipBubbleWindow.stopTimer();
                mToolTipBubbleWindow.removeFromDesktop();
                mToolTipBubbleWindow.setVisible(false);
            }
        }
        break;
        default:
            break;
    }
}

void Document::Section::applicationCommandListChanged()
{
    pluginListButton.setTooltip(Utils::getCommandDescriptionWithKey(mApplicationCommandManager, ApplicationCommandIDs::editNewTrack));
    Utils::notifyListener(mApplicationCommandManager, *this, {ApplicationCommandIDs::viewInfoBubble});
}

void Document::Section::handleAsyncUpdate()
{
    auto const autoresize = mAccessor.getAttr<AttrType::autoresize>();
    if(autoresize)
    {
        if(mViewport.getHeight() <= 0 || isDragAndDropActive())
        {
            return;
        }
        if(mIsLoading || std::any_of(mGroupSections.cbegin(), mGroupSections.cend(), [](auto const& pair)
                                     {
                                         return pair.second != nullptr && pair.second->isStretching();
                                     }))
        {
            triggerAsyncUpdate();
        }
        else
        {
            Tools::resizeItems(mAccessor, true, mViewport.getHeight());
        }
    }
}

ANALYSE_FILE_END
