#include "AnlDocumentSection.h"
#include "AnlDocumentAudioReader.h"
#include "AnlDocumentTools.h"

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
    juce::Component::mouseWheelMove(e, wheel);
}

Document::Section::Section(Director& director)
: mDirector(director)
, mTimeRuler(mAccessor.getAcsr<AcsrType::timeZoom>(), Zoom::Ruler::Orientation::horizontal, [](double value)
             {
                 return Format::secondsToString(value, {":", ":", ":", ""});
             })
, mLayoutNotifier(typeid(*this).name(), mAccessor, [this]()
                  {
                      updateLayout();
                  })
{
    mTimeRuler.onDoubleClick = [this]()
    {
        auto& acsr = mAccessor.getAcsr<AcsrType::timeZoom>();
        acsr.setAttr<Zoom::AttrType::visibleRange>(acsr.getAttr<Zoom::AttrType::globalRange>(), NotificationType::synchronous);
    };
    mTimeRuler.onMouseDown = [this]()
    {
        if(juce::Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isCtrlDown())
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
            juce::CallOutBox::launchAsynchronously(std::move(timeRangeEditor), bounds, nullptr).setArrowSize(0.0f);
            return false;
        }
        return true;
    };

    mDraggableTable.onComponentDropped = [&](juce::String const& identifier, size_t index, bool copy)
    {
        if(copy)
        {
            anlStrongAssert(Tools::hasGroupAcsr(mAccessor, identifier));
            if(!Tools::hasGroupAcsr(mAccessor, identifier))
            {
                return;
            }
            mDirector.startAction();
            auto const groupIdentifier = mDirector.addGroup(index, NotificationType::synchronous);
            if(!groupIdentifier.has_value())
            {
                mDirector.endAction(ActionState::abort);
            }

            auto const& groupAcsr = Tools::getGroupAcsr(mAccessor, identifier);

            Group::Accessor copyAcsr;
            copyAcsr.copyFrom(groupAcsr, NotificationType::synchronous);
            copyAcsr.setAttr<Group::AttrType::identifier>(*groupIdentifier, NotificationType::synchronous);
            copyAcsr.setAttr<Group::AttrType::layout>(std::vector<juce::String>(), NotificationType::synchronous);
            auto& newGroupAcsr = Tools::getGroupAcsr(mAccessor, *groupIdentifier);
            newGroupAcsr.copyFrom(copyAcsr, NotificationType::synchronous);

            auto const trackIdentifiers = groupAcsr.getAttr<Group::AttrType::layout>();
            for(size_t i = 0; i < trackIdentifiers.size(); ++i)
            {
                if(!mDirector.copyTrack(*groupIdentifier, i, trackIdentifiers[i], NotificationType::synchronous))
                {
                    mDirector.endAction(ActionState::abort);
                    return;
                }
            }
            mDirector.endAction(ActionState::newTransaction, juce::translate("Copy Group"));
        }
        else
        {
            mDirector.startAction();
            auto layout = copy_with_erased(mAccessor.getAttr<AttrType::layout>(), identifier);
            layout.insert(layout.begin() + static_cast<long>(index), identifier);
            mAccessor.setAttr<AttrType::layout>(layout, NotificationType::synchronous);
            mDirector.endAction(ActionState::newTransaction, juce::translate("Move Group"));
        }
    };

    mViewport.setViewedComponent(&mDraggableTable, false);
    mViewport.setScrollBarsShown(true, false, false, false);

    setWantsKeyboardFocus(true);
    setFocusContainerType(juce::Component::FocusContainerType::focusContainer);

    addAndMakeVisible(mReaderLayoutButton);
    mReaderLayoutButton.setWantsKeyboardFocus(false);
    mReaderLayoutButton.onClick = [this]()
    {
        mReaderLayoutPanel.show();
    };

    addAndMakeVisible(mDocumentName);
    mDocumentName.onClick = [this]()
    {
        auto const file = mAccessor.getAttr<AttrType::path>();
        if(file != juce::File{})
        {
            mAccessor.getAttr<AttrType::path>().revealToUser();
        }
        else if(onSaveButtonClicked != nullptr)
        {
            onSaveButtonClicked();
        }
    };

    addAndMakeVisible(mExpandLayoutButton);
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
    mResizeLayoutButton.setToggleable(true);
    mResizeLayoutButton.setTooltip(juce::translate("Optimize the height of groups and tracks to fit the height of the document"));
    mResizeLayoutButton.onClick = [this]()
    {
        auto const isShiftDown = juce::Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isShiftDown();
        auto const autoresize = mAccessor.getAttr<AttrType::autoresize>();
        if(isShiftDown && !autoresize)
        {
            updateHeights(true);
            mResizeLayoutButton.setToggleState(false, juce::NotificationType::dontSendNotification);
        }
        else if(!isShiftDown)
        {
            mAccessor.setAttr<AttrType::autoresize>(!autoresize, NotificationType::synchronous);
        }
    };

    addAndMakeVisible(mGridButton);
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

    mAddButton.setButtonText("+");
    mAddButton.setWantsKeyboardFocus(false);
    mAddButton.setMouseClickGrabsKeyboardFocus(false);
    mAddButton.onClick = [this]()
    {
        juce::PopupMenu menu;
        juce::WeakReference<juce::Component> target(this);
        menu.addItem(juce::translate("Add New Track"), [=, this]()
                     {
                         if(target.get() != nullptr && onNewTrackButtonClicked != nullptr)
                         {
                             onNewTrackButtonClicked();
                         }
                     });
        menu.addItem(juce::translate("Add New Group"), [=, this]()
                     {
                         if(target.get() != nullptr && onNewGroupButtonClicked != nullptr)
                         {
                             onNewGroupButtonClicked();
                         }
                     });
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mAddButton));
    };

    addAndMakeVisible(tooltipButton);
    addAndMakeVisible(mTransportDisplay);
    addAndMakeVisible(mTimeRulerDecoration);
    mLoopBar.addAndMakeVisible(mPlayheadBar);
    mPlayheadBar.setInterceptsMouseClicks(false, false);
    mLoopBar.addMouseListener(&mPlayheadBar, false);
    addAndMakeVisible(mLoopBarDecoration);
    addAndMakeVisible(mTopSeparator);
    addAndMakeVisible(mViewport);
    addAndMakeVisible(mBottomSeparator);
    addAndMakeVisible(mTimeScrollBar);
    addAndMakeVisible(mAddButton);
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
                break;
            case Transport::AttrType::markers:
            {
                mMagnetizeButton.setEnabled(!acsr.getAttr<Transport::AttrType::markers>().empty());
            }
            break;
            case Transport::AttrType::magnetize:
            {
                mMagnetizeButton.setToggleState(acsr.getAttr<Transport::AttrType::magnetize>(), juce::NotificationType::dontSendNotification);
            }
            break;
        }
    };

    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::reader:
            {
                auto const result = createAudioFormatReader(acsr, mDirector.getAudioFormatManager());
                auto const alertMessage = std::get<1>(result).joinIntoString("");
                if(alertMessage.isEmpty())
                {
                    mReaderLayoutButton.setTypes(Icon::Type::music);
                    mReaderLayoutButton.setTooltip(juce::translate("Show audio reader layout panel"));
                }
                else
                {
                    mReaderLayoutButton.setTypes(Icon::Type::alert);
                    mReaderLayoutButton.setTooltip(alertMessage);
                }

                if(std::get<0>(result) && std::get<0>(result)->sampleRate > 0.0)
                {
                    auto const maxTime = static_cast<double>(std::get<0>(result)->lengthInSamples) / std::get<0>(result)->sampleRate;
                    mTransportDisplay.setMaxTime(maxTime);
                }
            }
            break;
            case AttrType::grid:
            {
                switch(mAccessor.getAttr<AttrType::grid>())
                {
                    case GridMode::hidden:
                    {
                        mGridButton.setTypes(Icon::Type::gridOff);
                    }
                    break;
                    case GridMode::partial:
                    {
                        mGridButton.setTypes(Icon::Type::gridPartial);
                    }
                    break;
                    case GridMode::full:
                    {
                        mGridButton.setTypes(Icon::Type::gridFull);
                    }
                    break;
                }
            }
            break;
            case AttrType::autoresize:
            {
                auto const autoresize = acsr.getAttr<AttrType::autoresize>();
                for(auto& groupSection : mGroupSections)
                {
                    if(groupSection.second != nullptr)
                    {
                        groupSection.second->setResizable(!autoresize);
                    }
                }
                if(autoresize)
                {
                    // It ensures asynchronous access to the model even if this is not the best approach.
                    juce::WeakReference<juce::Component> weakReference(this);
                    juce::MessageManager::callAsync([=, this]()
                                                    {
                                                        if(weakReference.get() == nullptr)
                                                        {
                                                            return;
                                                        }
                                                        updateHeights();
                                                    });
                }
                mResizeLayoutButton.setToggleState(autoresize, juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::path:
            {
                auto const file = acsr.getAttr<AttrType::path>();
                mDocumentName.setButtonText(file != juce::File{} ? file.getFileName() : juce::translate("Untitled"));
                mDocumentName.setTooltip(file != juce::File{} ? file.getFullPathName() : juce::translate("Document not saved"));
                auto bounds = getLocalBounds();
                resizeHeader(bounds);
            }
            break;
            case AttrType::layout:
            case AttrType::viewport:
            case AttrType::samplerate:
                break;
        }
    };

    auto upddateHeightsAsync = [this]()
    {
        // Using a delay ensures that the group and track size animations are preserved.
        // It also ensures asynchronous access to the model even if this is not the best approach.
        juce::WeakReference<juce::Component> weakReference(this);
        juce::Timer::callAfterDelay(350, [=, this]()
                                    {
                                        if(weakReference.get() == nullptr)
                                        {
                                            return;
                                        }
                                        updateHeights();
                                    });
    };

    mListener.onAccessorInserted = [=, this](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::groups:
            {
                auto listener = std::make_unique<Group::Accessor::SmartListener>(typeid(*this).name(), mAccessor.getAcsr<AcsrType::groups>(index), [=, this](Group::Accessor const& groupAcsr, Group::AttrType groupAttribute)
                                                                                 {
                                                                                     juce::ignoreUnused(groupAcsr);
                                                                                     if(groupAttribute == Group::AttrType::expanded)
                                                                                     {
                                                                                         updateExpandState();
                                                                                         upddateHeightsAsync();
                                                                                     }
                                                                                 });
                mGroupListeners.emplace(mGroupListeners.begin() + static_cast<long>(index), std::move(listener));
                anlWeakAssert(mGroupListeners.size() == acsr.getNumAcsrs<AcsrType::groups>());
                upddateHeightsAsync();
            }
            break;
            case AcsrType::tracks:
            {
                upddateHeightsAsync();
            }
            break;
            case AcsrType::timeZoom:
            case AcsrType::transport:
                break;
        }
    };

    mListener.onAccessorErased = [=, this](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::groups:
            {
                mGroupListeners.erase(mGroupListeners.begin() + static_cast<long>(index));
                anlWeakAssert(mGroupListeners.size() == acsr.getNumAcsrs<AcsrType::groups>());
                updateExpandState();
                upddateHeightsAsync();
            }
            break;
            case AcsrType::tracks:
            {
                upddateHeightsAsync();
            }
            break;
            case AcsrType::timeZoom:
            case AcsrType::transport:
                break;
        }
    };

    mReceiver.onSignal = [&](Accessor const& acsr, SignalType signal, juce::var value)
    {
        juce::ignoreUnused(acsr);
        switch(signal)
        {
            case SignalType::viewport:
            {
                auto const x = static_cast<int>(value.getProperty("x", mViewport.getViewPositionX()));
                auto const y = static_cast<int>(value.getProperty("y", mViewport.getViewPositionY()));
                mViewport.setViewPosition(x, y);
            }
            break;
        }
    };

    mViewport.onVisibleAreaChanged = [this](juce::Rectangle<int> const& area)
    {
        juce::WeakReference<juce::Component> weakReference(this);
        juce::MessageManager::callAsync([=, point = area.getTopLeft(), this]
                                        {
                                            if(weakReference == nullptr)
                                            {
                                                return;
                                            }
                                            mAccessor.setAttr<AttrType::viewport>(point, NotificationType::synchronous);
                                        });
    };

    mAccessor.getAcsr<AcsrType::transport>().addListener(mTransportListener, NotificationType::synchronous);
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.addReceiver(mReceiver);
    juce::Desktop::getInstance().addFocusChangeListener(this);
}

Document::Section::~Section()
{
    juce::Desktop::getInstance().removeFocusChangeListener(this);
    mAccessor.removeReceiver(mReceiver);
    mAccessor.removeListener(mListener);
    mAccessor.getAcsr<AcsrType::transport>().removeListener(mTransportListener);
}

void Document::Section::resizeHeader(juce::Rectangle<int>& bounds)
{
    auto header = bounds.removeFromTop(40);
    mTransportDisplay.setBounds(header.withSizeKeepingCentre(284, 40));
    header.removeFromLeft(5);
    header = header.withRight(mTransportDisplay.getX());
    mReaderLayoutButton.setBounds(header.removeFromLeft(24).withSizeKeepingCentre(24, 24));
    header.removeFromLeft(4);
    auto const font = mDocumentName.getLookAndFeel().getTextButtonFont(mDocumentName, 24);
    auto const textWidth = font.getStringWidth(mDocumentName.getButtonText()) + static_cast<int>(std::ceil(font.getHeight()) * 2.0f);
    auto const buttonWidth = std::min(textWidth, header.getWidth());
    mDocumentName.setBounds(header.removeFromLeft(buttonWidth).withSizeKeepingCentre(buttonWidth, 24));
}

void Document::Section::resized()
{
    auto const scrollbarWidth = mViewport.getScrollBarThickness();
    auto bounds = getLocalBounds();
    resizeHeader(bounds);

    {
        auto topPart = bounds.removeFromTop(28);
        mGridButton.setBounds(topPart.removeFromLeft(28).withSizeKeepingCentre(18, 18));
        mExpandLayoutButton.setBounds(topPart.removeFromLeft(28).withSizeKeepingCentre(20, 20));
        mResizeLayoutButton.setBounds(topPart.removeFromLeft(28).withSizeKeepingCentre(20, 20));
        tooltipButton.setBounds(topPart.removeFromRight(24 + scrollbarWidth).withSizeKeepingCentre(20, 20));
        mMagnetizeButton.setBounds(tooltipButton.getBounds().translated(0, -28));
        mTimeRulerDecoration.setBounds(topPart.removeFromTop(14));
        mLoopBarDecoration.setBounds(topPart);
        mPlayheadBar.setBounds(mLoopBar.getLocalBounds());
        mTopSeparator.setBounds(bounds.removeFromTop(1));
    }

    {
        auto bottomPart = bounds.removeFromBottom(14).reduced(0, 1);
        mAddButton.setBounds(bottomPart.removeFromLeft(48));
        bottomPart.removeFromLeft(36);
        auto const timeScrollBarBounds = bottomPart.withRight(mTimeRulerDecoration.getRight());
        mTimeScrollBar.setBounds(timeScrollBarBounds);
        mBottomSeparator.setBounds(bounds.removeFromBottom(1));
    }

    mViewport.setBounds(bounds);
    mDraggableTable.setBounds(0, 0, bounds.getWidth() - scrollbarWidth, mDraggableTable.getHeight());
    updateHeights();
}

void Document::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
}

void Document::Section::updateHeights(bool force)
{
    if(mViewport.getHeight() <= 0 || (!mAccessor.getAttr<AttrType::autoresize>() && !force))
    {
        return;
    }
    auto height = mViewport.getHeight();
    auto groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
    auto const numElements = std::accumulate(groupAcsrs.cbegin(), groupAcsrs.cend(), 0_z, [this](auto v, auto const groupAcsr)
                                             {
                                                 auto const expanded = groupAcsr.get().template getAttr<Group::AttrType::expanded>();
                                                 auto const layout = copy_with_erased_if(groupAcsr.get().template getAttr<Group::AttrType::layout>(), [this](auto const& identifier)
                                                                                         {
                                                                                             return !Tools::hasTrackAcsr(mAccessor, identifier);
                                                                                         });
                                                 return v + 1_z + (expanded ? layout.size() : 0_z);
                                             });
    auto const elementHeight = static_cast<float>(height) / static_cast<float>(numElements);
    auto remainder = 0.0f;
    for(auto groupAcsr : groupAcsrs)
    {
        auto const groupHeight = std::min(static_cast<int>(std::round(elementHeight - remainder)), height);
        remainder = elementHeight - static_cast<float>(groupHeight);
        height -= groupHeight;
        groupAcsr.get().setAttr<Group::AttrType::height>(groupHeight - 1, NotificationType::synchronous);
        auto const expanded = groupAcsr.get().getAttr<Group::AttrType::expanded>();
        if(expanded)
        {
            auto const layout = copy_with_erased_if(groupAcsr.get().template getAttr<Group::AttrType::layout>(), [this](auto const& identifier)
                                                    {
                                                        return !Tools::hasTrackAcsr(mAccessor, identifier);
                                                    });
            for(auto const& identifier : layout)
            {
                auto& trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
                auto const trackHeight = std::min(static_cast<int>(std::round(elementHeight - remainder)), height);
                remainder = elementHeight - static_cast<float>(trackHeight);
                height -= trackHeight;
                trackAcsr.setAttr<Track::AttrType::height>(trackHeight - 1, NotificationType::synchronous);
            }
        }
    }
}

void Document::Section::updateExpandState()
{
    auto groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
    mExpandLayoutButton.setEnabled(!groupAcsrs.empty());
    if(std::any_of(groupAcsrs.cbegin(), groupAcsrs.cend(), [](auto const groupAcsr)
                   {
                       return groupAcsr.get().template getAttr<Group::AttrType::expanded>();
                   }))
    {
        mExpandLayoutButton.setTypes(Icon::Type::shrink);
        mExpandLayoutButton.setTooltip(juce::translate("Shrink all the groups"));
    }
    else
    {
        mExpandLayoutButton.setTypes(Icon::Type::expand);
        mExpandLayoutButton.setTooltip(juce::translate("Expand all the groups"));
    }
}

void Document::Section::mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel)
{
    if(event.mods.isCommandDown())
    {
        return;
    }
    auto const viewportOffset = mTimeRulerDecoration.getX();
    auto const viewportWidth = mTimeRulerDecoration.getWidth();
    auto const viewportActiveBounds = mViewport.getBounds().withX(viewportOffset).withWidth(viewportWidth);
    if(!viewportActiveBounds.contains(event.getPosition()))
    {
        mViewport.useMouseWheelMoveIfNeeded(event.getEventRelativeTo(&mViewport), wheel);
        return;
    }
    if(mScrollHelper.mouseWheelMove(wheel) == ScrollHelper::vertical && !event.mods.isShiftDown())
    {
        mouseMagnify(event, 1.0f + wheel.deltaY);
    }
    else
    {
        auto& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
        auto const visibleRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
        auto const delta = event.mods.isShiftDown() ? static_cast<double>(wheel.deltaY) : static_cast<double>(wheel.deltaX);
        auto const offset = delta * visibleRange.getLength();
        timeZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange - offset, NotificationType::synchronous);
    }
}

void Document::Section::mouseMagnify(juce::MouseEvent const& event, float magnifyAmount)
{
    if(event.mods.isCommandDown())
    {
        return;
    }
    auto& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
    auto const globalRange = timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
    auto const amount = static_cast<double>(1.0f - magnifyAmount) / 5.0 * globalRange.getLength();
    auto const visibleRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();

    auto const& transportAcsr = mAccessor.getAcsr<AcsrType::transport>();
    auto const anchor = transportAcsr.getAttr<Transport::AttrType::playback>() ? transportAcsr.getAttr<Transport::AttrType::runningPlayhead>() : transportAcsr.getAttr<Transport::AttrType::startPlayhead>();

    auto const amountLeft = (anchor - visibleRange.getStart()) / visibleRange.getEnd() * amount;
    auto const amountRight = (visibleRange.getEnd() - anchor) / visibleRange.getEnd() * amount;

    auto const minDistance = timeZoomAcsr.getAttr<Zoom::AttrType::minimumLength>() / 2.0;
    auto const start = std::min(anchor - minDistance, visibleRange.getStart() - amountLeft);
    auto const end = std::max(anchor + minDistance, visibleRange.getEnd() + amountRight);
    timeZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(Zoom::Range{start, end}, NotificationType::synchronous);
}

void Document::Section::moveKeyboardFocusTo(juce::String const& identifier)
{
    auto forwardToGroup = [&](juce::String const& groupIdentifier)
    {
        anlWeakAssert(Tools::hasGroupAcsr(mAccessor, groupIdentifier));
        if(Tools::hasGroupAcsr(mAccessor, groupIdentifier))
        {
            auto it = mGroupSections.find(groupIdentifier);
            anlWeakAssert(it != mGroupSections.end());
            if(it != mGroupSections.end())
            {
                it->second.get()->moveKeyboardFocusTo(identifier);
            }
        }
    };

    if(Tools::hasTrackAcsr(mAccessor, identifier) && Tools::isTrackInGroup(mAccessor, identifier))
    {
        auto const& groupAcsr = Tools::getGroupAcsrForTrack(mAccessor, identifier);
        auto const groupIdentifier = groupAcsr.getAttr<Group::AttrType::identifier>();
        forwardToGroup(groupIdentifier);
    }
    else
    {
        forwardToGroup(identifier);
    }
}

void Document::Section::showBubbleInfo(bool state)
{
    if(state)
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
    auto createGroupSection = [&](Group::Director& groupDirector)
    {
        auto& transportAcsr = mAccessor.getAcsr<AcsrType::transport>();
        auto& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
        auto groupSection = std::make_unique<Group::StrechableSection>(groupDirector, transportAcsr, timeZoomAcsr);
        if(groupSection != nullptr)
        {
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
                auto& groupDirector = mDirector.getGroupDirector(identifier);
                auto groupSection = createGroupSection(groupDirector);
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
        mDirector.endAction(ActionState::newTransaction, juce::translate("Move Track"));
        moveKeyboardFocusTo(trackIdentifier);
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
    if(mDirector.copyTrack(groupIdentifier, index, trackIdentifier, NotificationType::synchronous))
    {
        mDirector.endAction(ActionState::newTransaction, juce::translate("Copy Track"));
        moveKeyboardFocusTo(trackIdentifier);
    }
    else
    {
        mDirector.endAction(ActionState::abort);
    }
}

std::unique_ptr<juce::ComponentTraverser> Document::Section::createKeyboardFocusTraverser()
{
    class FocusTraverser
    : public juce::KeyboardFocusTraverser
    {
    public:
        FocusTraverser(Section& section)
        : mSection(section)
        {
        }

        ~FocusTraverser() override = default;

        juce::Component* getNextComponent(juce::Component* current) override
        {
            auto const contents = mSection.mDraggableTable.getComponents();
            if(contents.empty())
            {
                return juce::KeyboardFocusTraverser::getNextComponent(current);
            }
            auto it = std::find_if(contents.begin(), contents.end(), [&](auto const& content)
                                   {
                                       return content != nullptr && (content == current || content->isParentOf(current));
                                   });
            while(it != contents.end() && std::next(it) != contents.end())
            {
                it = std::next(it);
                if(*it != nullptr)
                {
                    if(auto childFocusTraverser = getChildFocusTraverser(*it))
                    {
                        childFocusTraverser->getNextComponent(nullptr);
                    }
                    return it->getComponent();
                }
            }
            return contents.begin()->getComponent();
        }

        juce::Component* getPreviousComponent(juce::Component* current) override
        {
            auto const contents = mSection.mDraggableTable.getComponents();
            if(contents.empty())
            {
                return juce::KeyboardFocusTraverser::getPreviousComponent(current);
            }
            auto it = std::find_if(contents.rbegin(), contents.rend(), [&](auto const& content)
                                   {
                                       return content != nullptr && (content == current || content->isParentOf(current));
                                   });
            while(it != contents.rend())
            {
                it = std::next(it);
                if(*it != nullptr)
                {
                    if(auto childFocusTraverser = getChildFocusTraverser(*it))
                    {
                        childFocusTraverser->getPreviousComponent(nullptr);
                    }
                    return it->getComponent();
                }
            }
            return contents.back().getComponent();
        }

        std::unique_ptr<juce::ComponentTraverser> getChildFocusTraverser(juce::Component* component)
        {
            auto* child = dynamic_cast<Group::StrechableSection*>(component);
            if(child != nullptr)
            {
                return child->createKeyboardFocusTraverser();
            }
            return nullptr;
        }

    private:
        Section& mSection;
    };

    return std::make_unique<FocusTraverser>(*this);
}

void Document::Section::globalFocusChanged(juce::Component* focusedComponent)
{
    if(mViewport.isParentOf(focusedComponent))
    {
        auto getSection = [&]() -> juce::Component*
        {
            if(dynamic_cast<Track::Section*>(focusedComponent) || dynamic_cast<Group::Section*>(focusedComponent))
            {
                return focusedComponent;
            }
            anlWeakAssert(false);
            return nullptr;
        };
        if(auto* section = getSection())
        {
            // This is used to prevent false notifications
            if(mFocusComponent == section)
            {
                return;
            }
            mFocusComponent = section;

            auto const area = mViewport.getViewArea();
            auto const relativeBounds = mDraggableTable.getLocalArea(section, section->getLocalBounds());
            if(relativeBounds.intersects(area))
            {
                return;
            }
            auto const bottomDifference = relativeBounds.getBottom() - area.getBottom();
            auto const topDifference = relativeBounds.getY() - area.getY();
            if(bottomDifference > 0)
            {
                mViewport.setViewPosition({area.getX(), area.getY() + bottomDifference + 8});
            }
            else if(topDifference < 0)
            {
                mViewport.setViewPosition({area.getX(), area.getY() + topDifference - 8});
            }
        }
    }
}

ANALYSE_FILE_END
