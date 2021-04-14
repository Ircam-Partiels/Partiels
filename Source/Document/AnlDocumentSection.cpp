#include "AnlDocumentSection.h"

ANALYSE_FILE_BEGIN

Document::Section::Section(Accessor& accessor, juce::AudioFormatManager& audioFormatManager)
: mAccessor(accessor)
, mFileInfoPanel(accessor, audioFormatManager)
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
    
    mDraggableTable.onComponentDragged = [&](juce::String const& identifier, size_t index)
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
}

Document::Section::~Section()
{
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

void Document::Section::updateLayout()
{
    auto createGroupSection = [&](Group::Accessor& groupAcsr)
    {
        auto& transportAcsr = mAccessor.getAcsr<AcsrType::transport>();
        auto& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
        auto groupSection = std::make_unique<Group::StrechableSection>(groupAcsr, transportAcsr, timeZoomAcsr);
        if(groupSection != nullptr)
        {
            groupSection->onRemoveGroup = [&]()
            {
                if(onRemoveGroup != nullptr)
                {
                    onRemoveGroup(groupAcsr.getAttr<Group::AttrType::identifier>());
                }
            };
            groupSection->onRemoveTrack = [&](juce::String const& identifier)
            {
                if(onRemoveTrack != nullptr)
                {
                    onRemoveTrack(identifier);
                }
            };
            groupSection->onTrackInserted = [&](juce::String const& identifier)
            {
                if(onTrackInserted != nullptr)
                {
                    onTrackInserted(groupAcsr.getAttr<Group::AttrType::identifier>(), identifier);
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
        })
        || std::none_of(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](auto const& groupAcsr)
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

ANALYSE_FILE_END
