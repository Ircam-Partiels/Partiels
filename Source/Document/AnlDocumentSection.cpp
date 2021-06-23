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
    mTimeRuler.onDoubleClick = [&]()
    {
        auto& acsr = mAccessor.getAcsr<AcsrType::timeZoom>();
        acsr.setAttr<Zoom::AttrType::visibleRange>(acsr.getAttr<Zoom::AttrType::globalRange>(), NotificationType::synchronous);
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
    setFocusContainerType(juce::Component::FocusContainerType::keyboardFocusContainer);

    addAndMakeVisible(mReaderLayoutButton);
    mReaderLayoutButton.setWantsKeyboardFocus(false);
    mReaderLayoutButton.setTooltip(juce::translate("Show audio reader layout panel"));
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
    mExpandLayoutButton.setTooltip(juce::translate("Expand or shrink all the groups"));
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
    mResizeLayoutButton.setTooltip(juce::translate("Optimize the height of groups and tracks to fit the height of the document"));
    mResizeLayoutButton.onClick = [this]()
    {
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
                    height -= groupHeight;
                    trackAcsr.setAttr<Track::AttrType::height>(trackHeight - 1, NotificationType::synchronous);
                }
            }
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
    setSize(480, 200);

    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::reader:
            {
                auto const result = createAudioFormatReader(acsr, mDirector.getAudioFormatManager());
                mReaderAlertMessage = std::get<1>(result).joinIntoString("");
                lookAndFeelChanged();
            }
            break;
            case AttrType::grid:
            {
                lookAndFeelChanged();
            }
            break;
            case AttrType::path:
            {
                auto const file = acsr.getAttr<AttrType::path>();
                mDocumentName.setButtonText(file != juce::File{} ? file.getFileName() : juce::translate("Untitled"));
                mDocumentName.setTooltip(file != juce::File{} ? file.getFullPathName() : juce::translate("Document not saved"));
                resized();
            }
            break;
            case AttrType::layout:
            case AttrType::viewport:
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
        juce::WeakReference<juce::Component> target(this);
        juce::MessageManager::callAsync([=, point = area.getTopLeft(), this]
                                        {
                                            if(target.get() != nullptr)
                                            {
                                                mAccessor.setAttr<AttrType::viewport>(point, NotificationType::synchronous);
                                            }
                                        });
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.addReceiver(mReceiver);
    juce::Desktop::getInstance().addFocusChangeListener(this);
}

Document::Section::~Section()
{
    juce::Desktop::getInstance().removeFocusChangeListener(this);
    mAccessor.removeReceiver(mReceiver);
    mAccessor.removeListener(mListener);
}

void Document::Section::resized()
{
    auto const scrollbarWidth = mViewport.getScrollBarThickness();
    auto bounds = getLocalBounds();

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

    {
        auto topPart = bounds.removeFromTop(28);
        mGridButton.setBounds(topPart.removeFromLeft(28).withSizeKeepingCentre(18, 18));
        mExpandLayoutButton.setBounds(topPart.removeFromLeft(28).withSizeKeepingCentre(20, 20));
        mResizeLayoutButton.setBounds(topPart.removeFromLeft(28).withSizeKeepingCentre(20, 20));
        tooltipButton.setBounds(topPart.removeFromRight(24 + scrollbarWidth).withSizeKeepingCentre(20, 20));
        mTimeRulerDecoration.setBounds(topPart.removeFromTop(14));
        mLoopBarDecoration.setBounds(topPart);
        mPlayheadBar.setBounds(mLoopBar.getLocalBounds());
    }

    mTopSeparator.setBounds(bounds.removeFromTop(1));
    auto const timeScrollBarBounds = bounds.removeFromBottom(8).withX(mTimeRulerDecoration.getX()).withRight(mTimeRulerDecoration.getRight());
    mTimeScrollBar.setBounds(timeScrollBarBounds);
    mBottomSeparator.setBounds(bounds.removeFromBottom(1));
    mViewport.setBounds(bounds);
    mDraggableTable.setBounds(0, 0, bounds.getWidth() - scrollbarWidth, mDraggableTable.getHeight());
}

void Document::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
}

void Document::Section::lookAndFeelChanged()
{
    auto* laf = dynamic_cast<IconManager::LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(laf != nullptr);
    if(laf != nullptr)
    {
        if(!mReaderAlertMessage.isEmpty())
        {
            mReaderLayoutButton.setTooltip(mReaderAlertMessage);
            laf->setButtonIcon(mReaderLayoutButton, IconManager::IconType::alert);
        }
        else
        {
            mReaderLayoutButton.setTooltip(juce::translate("Show audio reader layout panel"));
            laf->setButtonIcon(mReaderLayoutButton, IconManager::IconType::music);
        }
        laf->setButtonIcon(mExpandLayoutButton, IconManager::IconType::layers);
        laf->setButtonIcon(mResizeLayoutButton, IconManager::IconType::perspective);
        laf->setButtonIcon(tooltipButton, IconManager::IconType::conversation);
        switch(mAccessor.getAttr<AttrType::grid>())
        {
            case GridMode::hidden:
            {
                laf->setButtonIcon(mGridButton, IconManager::IconType::grid_off);
            }
            break;
            case GridMode::partial:
            {
                laf->setButtonIcon(mGridButton, IconManager::IconType::grid_partial);
            }
            break;
            case GridMode::full:
            {
                laf->setButtonIcon(mGridButton, IconManager::IconType::grid_full);
            }
            break;
        }
    }
}

void Document::Section::mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel)
{
    if(event.mods.isCommandDown())
    {
        return;
    }
    auto& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
    auto const visibleRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const offset = static_cast<double>(wheel.deltaX) * visibleRange.getLength();
    timeZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange - offset, NotificationType::synchronous);

    auto viewportPosition = mViewport.getViewPosition();
    viewportPosition.y += static_cast<int>(std::round(wheel.deltaY * 14.0f * 16.0f));
    mViewport.setViewPosition(viewportPosition);
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
    resized();
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

std::unique_ptr<juce::ComponentTraverser> Document::Section::createFocusTraverser()
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
                return child->createFocusTraverser();
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
