#include "AnlDocumentSection.h"
#include "AnlDocumentAudioReader.h"
#include "../Plugin/AnlPluginListScanner.h"

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
    
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::file:
            case AttrType::isLooping:
            case AttrType::gain:
            case AttrType::isPlaybackStarted:
            case AttrType::playheadPosition:
                break;
            case AttrType::layoutHorizontal:
            {
                resized();
            }
                break;
        }
    };
    
    mListener.onAccessorInserted = [&](Accessor const& acsr, AcsrType type, size_t index)
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
                    newSection->onRemove = [this](juce::String const identifier)
                    {
                        if(onRemoveAnalyzer != nullptr)
                        {
                            onRemoveAnalyzer(identifier);
                        }
                    };
                    mSections.insert(mSections.begin() + static_cast<long>(index), std::move(newSection));
                }
                std::vector<ConcertinaPanel::ComponentRef> components;
                for(auto const& section : mSections)
                {
                    components.push_back(*section.get());
                }
                mConcertinalPanel.setComponents(components);
            }
                break;
                
            default:
                break;
        }
        
    };
    
    mListener.onAccessorErased = [&](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::timeZoom:
                break;
            case AcsrType::analyzers:
            {
                mSections.erase(mSections.begin() + static_cast<long>(index));
                std::vector<ConcertinaPanel::ComponentRef> components;
                for(auto const& section : mSections)
                {
                    components.push_back(*section.get());
                }
                mConcertinalPanel.setComponents(components);
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
    
    mConcertinalPanel.onResized = [&]()
    {
        mViewport.autoScroll(0, mConcertinalPanel.getMouseXYRelative().getY(), -10, 10);
        resized();
    };
    mViewport.setViewedComponent(&mConcertinalPanel, false);
    mViewport.setScrollBarsShown(true, false, true, false);
    
    setSize(480, 200);
    addAndMakeVisible(mZoomTimeRuler);
    addAndMakeVisible(mViewport);
    addAndMakeVisible(mZoomTimeScrollBar);
    addAndMakeVisible(mResizerBar);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::Section::~Section()
{
    mAccessor.removeListener(mListener);
}

void Document::Section::resized()
{
    auto const scrollbarWidth = mViewport.getScrollBarThickness();
    auto bounds = getLocalBounds().withTrimmedRight(scrollbarWidth);
    auto const left = mAccessor.getAttr<AttrType::layoutHorizontal>() + 2;
    auto const right = bounds.getWidth() - 32;
    
    mZoomTimeRuler.setBounds(bounds.removeFromTop(14).withLeft(left).withRight(right));
    mZoomTimeScrollBar.setBounds(bounds.removeFromBottom(8).withLeft(left).withRight(right));
    
    mResizerBar.setBounds(left - 2, bounds.getY() + 2, 2, mConcertinalPanel.getHeight() - 4);
    mConcertinalPanel.setBounds(bounds.withHeight(mConcertinalPanel.getHeight()));
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
