#include "AnlDocumentSection.h"

ANALYSE_FILE_BEGIN

Document::Section::Section(Accessor& accessor)
: mAccessor(accessor)
{
    mZoomTimeRuler.setPrimaryTickInterval(0);
    mZoomTimeRuler.setTickReferenceValue(0.0);
    mZoomTimeRuler.setTickPowerInterval(10.0, 2.0);
    mZoomTimeRuler.setMaximumStringWidth(70.0);
    mZoomTimeRuler.setValueAsStringMethod([](double value)
    {
        return Format::secondsToString(value, {":", ":", ":", ""});
    });
    
    auto updateComponents = [&]()
    {
        mPlayheadContainer.setVisible(!mSections.empty());
        mZoomTimeRuler.setVisible(!mSections.empty());
        mPlot.setVisible(mSections.size() > 1);
        mViewport.setVisible(!mSections.empty());
        mZoomTimeScrollBar.setVisible(!mSections.empty());
        mResizerBar.setVisible(!mSections.empty());
        
        std::vector<ConcertinaTable::ComponentRef> components;
        components.reserve(mSections.size());
        auto const& layout = mAccessor.getAttr<AttrType::layout>();
        for(auto const& identifier : layout)
        {
            auto it = std::find_if(mSections.cbegin(), mSections.cend(), [&](auto const& section)
            {
                return section->getIdentifier() == identifier;
            });
            if(it != mSections.cend())
            {
                components.push_back(*it->get());
            }
        }
        mDraggableTable.setComponents(components);
        resized();
    };
    
    mListener.onAttrChanged = [=](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::file:
            case AttrType::isLooping:
            case AttrType::gain:
            case AttrType::isPlaybackStarted:
                break;
            case AttrType::playheadPosition:
            {
                mPlayhead.setPosition(acsr.getAttr<AttrType::playheadPosition>());
            }
                break;
            case AttrType::layoutHorizontal:
            case AttrType::layoutVertical:
            {
                resized();
            }
                break;
            case AttrType::layout:
            {
                updateComponents();
            }
                break;
        }
    };
    
    mListener.onAccessorInserted = [=](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::timeZoom:
                break;
            case AcsrType::analyzers:
            {
                auto& anlAcsr = mAccessor.getAccessor<AcsrType::analyzers>(index);
                auto& timeZoomAcsr = mAccessor.getAccessor<AcsrType::timeZoom>(0);
                
                auto newSection = std::make_unique<Analyzer::Section>(anlAcsr, timeZoomAcsr, mResizerBar);
                anlStrongAssert(newSection != nullptr);
                if(newSection != nullptr)
                {
                    newSection->onRemove = [this, ptr = newSection.get()]()
                    {
                        if(onRemoveAnalyzer != nullptr)
                        {
                            onRemoveAnalyzer(ptr->getIdentifier());
                        }
                    };
                }
                mSections.insert(mSections.begin() + static_cast<long>(index), std::move(newSection));
                updateComponents();
            }
                break;
                
            default:
                break;
        }
    };
    
    mListener.onAccessorErased = [=](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::timeZoom:
                break;
            case AcsrType::analyzers:
            {
                mSections.erase(mSections.begin() + static_cast<long>(index));
                updateComponents();
            }
                break;
                
            default:
                break;
        }
    };
    
    mZoomTimeRuler.onDoubleClick = [&]()
    {
        auto& acsr = mAccessor.getAccessor<AcsrType::timeZoom>(0);
        acsr.setAttr<Zoom::AttrType::visibleRange>(acsr.getAttr<Zoom::AttrType::globalRange>(), NotificationType::synchronous);
    };
    
    mResizerBar.onMoved = [&](int size)
    {
        mAccessor.setAttr<AttrType::layoutHorizontal>(size, NotificationType::synchronous);
    };
    
    mDraggableTable.onComponentDragged = [&](size_t previousIndex, size_t nextIndex)
    {
        auto layout = mAccessor.getAttr<AttrType::layout>();
        auto const identifier = layout[previousIndex];
        std::erase(layout, identifier);
        layout.insert(layout.begin() + static_cast<long>(nextIndex), identifier);
        mAccessor.setAttr<AttrType::layout>(layout, NotificationType::synchronous);
    };

    mBoundsListener.onComponentResized = [&](juce::Component& component)
    {
        juce::ignoreUnused(component);
        resized();
    };
    mBoundsListener.attachTo(mDraggableTable);
    mBoundsListener.attachTo(mPlot);
    mViewport.setViewedComponent(&mDraggableTable, false);
    mViewport.setScrollBarsShown(true, false, true, false);
    
    setSize(480, 200);
    mPlayheadContainer.addAndMakeVisible(mPlayhead);
    addChildComponent(mPlayheadContainer);
    addChildComponent(mZoomTimeRuler);
    addChildComponent(mPlot);
    addChildComponent(mViewport);
    addChildComponent(mZoomTimeScrollBar);
    addChildComponent(mResizerBar);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::Section::~Section()
{
    mBoundsListener.detachFrom(mDraggableTable);
    mAccessor.removeListener(mListener);
}

void Document::Section::resized()
{
    auto const scrollbarWidth = mViewport.getScrollBarThickness();
    auto bounds = getLocalBounds().withTrimmedRight(scrollbarWidth);
    auto const left = mAccessor.getAttr<AttrType::layoutHorizontal>() + 2;
    auto const right = bounds.getWidth() - 32;
    
    mZoomTimeRuler.setBounds(bounds.removeFromTop(14).withLeft(left + 1).withRight(right - 1));
    mPlayheadContainer.setBounds(bounds.removeFromTop(14).withLeft(left + 2).withRight(right + 6));
    mZoomTimeScrollBar.setBounds(bounds.removeFromBottom(8).withLeft(left + 1).withRight(right - 1));
    if(mPlot.isVisible())
    {
        mPlot.setBounds(bounds.removeFromTop(mPlot.getHeight()).withLeft(left).withRight(right + 6));
    }
    mResizerBar.setBounds(left - 2, bounds.getY() + 2, 2, mDraggableTable.getHeight() - 4);
    mDraggableTable.setBounds(bounds.withHeight(mDraggableTable.getHeight()));
    mViewport.setBounds(bounds.withTrimmedRight(-scrollbarWidth));
}

void Document::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
    auto bounds = getLocalBounds();
    g.setColour(mZoomTimeRuler.findColour(Zoom::Ruler::backgroundColourId));
    g.fillRect(bounds.removeFromTop(mZoomTimeRuler.getHeight()));
}

ANALYSE_FILE_END
