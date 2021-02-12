#include "AnlAnalyzerSection.h"

ANALYSE_FILE_BEGIN

Analyzer::Section::Section(Accessor& accessor, Zoom::Accessor& timeZoomAcsr, juce::Component& separator)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAcsr)
, mSeparator(separator)
{
    mValueRuler.onDoubleClick = [&]()
    {
        auto& plotAcsr = mAccessor.getAccessor<AcsrType::plot>(0);
        auto& zoomAcsr = plotAcsr.getAccessor<Plot::AcsrType::valueZoom>(0);
        auto const& range = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
    };
    
    mBinRuler.onDoubleClick = [&]()
    {
        auto& plotAcsr = mAccessor.getAccessor<AcsrType::plot>(0);
        auto& zoomAcsr = plotAcsr.getAccessor<Plot::AcsrType::binZoom>(0);
        auto const range = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
    };
    
    mThumbnail.onRemove = [&]()
    {
        if(onRemove != nullptr)
        {
            onRemove();
        }
    };
    
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType type)
    {
        switch(type)
        {
            case AttrType::name:
            case AttrType::key:
            case AttrType::description:
            case AttrType::state:
                break;
            case AttrType::results:
            {
                auto const& results = acsr.getAttr<AttrType::results>();
                auto const numDimensions = results.empty() ? 0_z : results.front().values.size();
                switch(numDimensions)
                {
                    case 0_z:
                    {
                        mBinRuler.setVisible(false);
                        mBinScrollBar.setVisible(false);
                        
                        mValueRuler.setVisible(true);
                        mValueScrollBar.setVisible(true);
                    }
                        break;
                    case 1_z:
                    {
                        mBinRuler.setVisible(false);
                        mBinScrollBar.setVisible(false);
                        
                        mValueRuler.setVisible(true);
                        mValueScrollBar.setVisible(true);
                    }
                        break;
                    default:
                    {
                        mBinRuler.setVisible(true);
                        mBinScrollBar.setVisible(true);
                        
                        mValueRuler.setVisible(false);
                        mValueScrollBar.setVisible(false);
                    }
                        break;
                }
            }
                break;
            case AttrType::warnings:
                break;
        }
    };
    
    mPlotListener.onAttrChanged = [&](Plot::Accessor const& acsr, Plot::AttrType const attribute)
    {
        switch(attribute)
        {
            case Plot::AttrType::height:
            {
                setSize(getWidth(), acsr.getAttr<Plot::AttrType::height>() + 2);
            }
                break;
                
            default:
                break;
        }
    };
    
    auto onResizerMoved = [&](int size)
    {
        mPlotAccessor.setAttr<Plot::AttrType::height>(size, NotificationType::synchronous);
    };
    mResizerBarLeft.onMoved = onResizerMoved;
    mResizerBarRight.onMoved = onResizerMoved;
    
    addChildComponent(mValueRuler);
    addChildComponent(mValueScrollBar);
    addChildComponent(mBinRuler);
    addChildComponent(mBinScrollBar);
    addAndMakeVisible(mThumbnail);
    addAndMakeVisible(mSnapshot);
    addAndMakeVisible(mTimeRenderer);
    addAndMakeVisible(mResizerBarLeft);
    addAndMakeVisible(mResizerBarRight);
    setSize(80, 100);
    
    mSeparator.addComponentListener(this);
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mPlotAccessor.addListener(mPlotListener, NotificationType::synchronous);
}

Analyzer::Section::~Section()
{
    mPlotAccessor.removeListener(mPlotListener);
    mAccessor.removeListener(mListener);
    mSeparator.removeComponentListener(this);
}

juce::String Analyzer::Section::getIdentifier() const
{
    return mAccessor.getAttr<AttrType::identifier>();
}

void Analyzer::Section::resized()
{
    auto bounds = getLocalBounds();
    auto const leftSize = mSeparator.getScreenX() - getScreenX();
    auto const rightSize = getWidth() - leftSize - mSeparator.getWidth();
    
    // Resizers
    {
        auto resizersBounds = bounds.removeFromBottom(2);
        mResizerBarLeft.setBounds(resizersBounds.removeFromLeft(leftSize).reduced(4, 0));
        mResizerBarRight.setBounds(resizersBounds.removeFromRight(rightSize).reduced(4, 0));
    }
    
    // Thumbnail and Snapshot
    {
        auto leftSide = bounds.removeFromLeft(leftSize);
        mThumbnail.setBounds(leftSide.removeFromLeft(48));
        mSnapshot.setBounds(leftSide.withTrimmedLeft(1));
    }
    
    // Renderer, rulers and scrollbars
    {
        auto rightSide = bounds.removeFromRight(rightSize);
        auto const scrollbarBounds = rightSide.removeFromRight(8).reduced(0, 4);
        mValueScrollBar.setBounds(scrollbarBounds);
        mBinScrollBar.setBounds(scrollbarBounds);
        auto const rulerBounds = rightSide.removeFromRight(16).reduced(0, 4);
        mValueRuler.setBounds(rulerBounds);
        mBinRuler.setBounds(rulerBounds);
        mTimeRenderer.setBounds(rightSide);
    }
}

void Analyzer::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::sectionColourId, true));
}

void Analyzer::Section::componentMovedOrResized(juce::Component& component, bool wasMoved, bool wasResized)
{
    juce::ignoreUnused(wasMoved, wasResized);
    anlStrongAssert(&component == &mSeparator);
    if(&component == &mSeparator)
    {
        resized();
    }
}

ANALYSE_FILE_END
