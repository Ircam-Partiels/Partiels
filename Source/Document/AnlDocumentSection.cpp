#include "AnlDocumentSection.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

Document::Section::Section(Director& director)
: mDirector(director)
{
    mTimeRuler.setPrimaryTickInterval(0);
    mTimeRuler.setTickReferenceValue(0.0);
    mTimeRuler.setTickPowerInterval(10.0, 2.0);
    mTimeRuler.setMaximumStringWidth(70.0);
    mTimeRuler.setValueAsStringMethod([](double value)
                                      {
                                          return Format::secondsToString(value, {":", ":", ":", ""});
                                      });

    mTimeRuler.onDoubleClick = [&]()
    {
        auto& acsr = mAccessor.getAcsr<AcsrType::timeZoom>();
        acsr.setAttr<Zoom::AttrType::visibleRange>(acsr.getAttr<Zoom::AttrType::globalRange>(), NotificationType::synchronous);
    };

    mDraggableTable.onComponentDropped = [&](juce::String const& identifier, size_t index)
    {
        auto layout = mAccessor.getAttr<AttrType::layout>();
        std::erase(layout, identifier);
        layout.insert(layout.begin() + static_cast<long>(index), identifier);
        mAccessor.setAttr<AttrType::layout>(layout, NotificationType::synchronous);
    };

    mTooltipButton.setClickingTogglesState(true);
    mTooltipButton.onClick = [&]()
    {
        if(mTooltipButton.getToggleState())
        {
            mToolTipBubbleWindow.startTimer(23);
        }
        else
        {
            mToolTipBubbleWindow.stopTimer();
            mToolTipBubbleWindow.removeFromDesktop();
            mToolTipBubbleWindow.setVisible(false);
        }
    };
    mTooltipButton.setToggleState(true, juce::NotificationType::sendNotification);

    mViewport.setViewedComponent(&mDraggableTable, false);
    mViewport.setScrollBarsShown(true, false, false, false);

    setWantsKeyboardFocus(true);
    setFocusContainer(true);

    setSize(480, 200);
    addAndMakeVisible(mFileInfoButtonDecoration);
    addAndMakeVisible(mTooltipButton);
    addAndMakeVisible(mTimeRulerDecoration);
    mLoopBar.addAndMakeVisible(mPlayheadBar);
    mPlayheadBar.setInterceptsMouseClicks(false, false);
    mLoopBar.addMouseListener(&mPlayheadBar, false);
    addAndMakeVisible(mLoopBarDecoration);
    addAndMakeVisible(mViewport);
    addAndMakeVisible(mTimeScrollBar);

    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::file:
                break;
            case AttrType::layout:
            {
                updateLayout();
            }
            break;
        }
    };

    mListener.onAccessorInserted = mListener.onAccessorErased = [&](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr, index);
        switch(type)
        {
            case AcsrType::timeZoom:
            case AcsrType::transport:
            case AcsrType::tracks:
                break;
            case AcsrType::groups:
            {
                updateLayout();
            }
            break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    juce::Desktop::getInstance().addFocusChangeListener(this);
}

Document::Section::~Section()
{
    juce::Desktop::getInstance().removeFocusChangeListener(this);
    mAccessor.removeListener(mListener);
}

void Document::Section::resized()
{
    auto constexpr leftSize = 96;
    auto const scrollbarWidth = mViewport.getScrollBarThickness();
    auto const rightSize = 24 + scrollbarWidth;
    auto bounds = getLocalBounds();

    auto topPart = bounds.removeFromTop(28);
    mFileInfoButtonDecoration.setBounds(topPart.removeFromLeft(leftSize));
    mTooltipButton.setBounds(topPart.removeFromRight(rightSize).reduced(4));
    mTimeRulerDecoration.setBounds(topPart.removeFromTop(14));
    mLoopBarDecoration.setBounds(topPart);
    mPlayheadBar.setBounds(mLoopBar.getLocalBounds());
    auto const timeScrollBarBounds = bounds.removeFromBottom(8).withTrimmedLeft(leftSize).withTrimmedRight(rightSize);
    mTimeScrollBar.setBounds(timeScrollBarBounds);
    mViewport.setBounds(bounds);
    mDraggableTable.setBounds(0, 0, bounds.getWidth() - scrollbarWidth, mDraggableTable.getHeight());
}

void Document::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
}

void Document::Section::colourChanged()
{
    setOpaque(findColour(ColourIds::backgroundColourId).isOpaque());
}

void Document::Section::lookAndFeelChanged()
{
    auto* laf = dynamic_cast<IconManager::LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(laf != nullptr);
    if(laf != nullptr)
    {
        laf->setButtonIcon(mTooltipButton, IconManager::IconType::conversation);
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

    if(Tools::hasTrackAcsr(mAccessor, identifier))
    {
        auto const& groupAcsr = Tools::getGroupAcsr(mAccessor, identifier);
        auto const groupIdentifier = groupAcsr.getAttr<Group::AttrType::identifier>();
        forwardToGroup(groupIdentifier);
    }
    else
    {
        forwardToGroup(identifier);
    }
}

void Document::Section::updateLayout()
{
    auto createGroupSection = [&](Group::Accessor& groupAcsr)
    {
        auto& transportAcsr = mAccessor.getAcsr<AcsrType::transport>();
        auto& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
        auto groupSection = std::make_unique<Group::StrechableSection>(groupAcsr, transportAcsr, timeZoomAcsr);
        if(groupSection != nullptr)
        {
            groupSection->onTrackInserted = [&](juce::String const& identifier)
            {
                anlStrongAssert(Tools::hasTrackAcsr(mAccessor, identifier));
                if(!Tools::hasTrackAcsr(mAccessor, identifier))
                {
                    return;
                }

                mDirector.startAction();
                if(mDirector.moveTrack(groupAcsr.getAttr<Group::AttrType::identifier>(), identifier, NotificationType::synchronous))
                {
                    auto const& trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
                    auto const trackName = trackAcsr.getAttr<Track::AttrType::name>();
                    auto const groupName = groupAcsr.getAttr<Group::AttrType::name>();
                    mDirector.endAction("Move Track " + trackName + " to Group " + groupName, ActionState::apply);
                    moveKeyboardFocusTo(identifier);
                }
                else
                {
                    mDirector.endAction("Move Track", ActionState::abort);
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
            auto groupIt = std::find_if(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](auto const& groupAcsr)
                                        {
                                            return groupAcsr.get().template getAttr<Group::AttrType::identifier>() == identifier;
                                        });
            if(groupIt != groupAcsrs.cend())
            {
                auto groupSection = createGroupSection(groupIt->get());
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

juce::KeyboardFocusTraverser* Document::Section::createFocusTraverser()
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
                    if(auto* childFocusTraverser = getChildFocusTraverser(*it))
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
                    if(auto* childFocusTraverser = getChildFocusTraverser(*it))
                    {
                        childFocusTraverser->getPreviousComponent(nullptr);
                    }
                    return it->getComponent();
                }
            }
            return contents.back().getComponent();
            ;
        }

        juce::KeyboardFocusTraverser* getChildFocusTraverser(juce::Component* component)
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

    return std::make_unique<FocusTraverser>(*this).release();
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
            if(relativeBounds.contains(area))
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
