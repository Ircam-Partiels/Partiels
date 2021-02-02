#include "AnlAnalyzerSection.h"

ANALYSE_FILE_BEGIN

Analyzer::Section::Section(Accessor& accessor, Zoom::Accessor& timeZoomAcsr, juce::Component& separator)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAcsr)
, mSeparator(separator)
{
    mValueRuler.onDoubleClick = [&]()
    {
        auto& zoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
        auto const& range = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
    };
    
    mBinRuler.onDoubleClick = [&]()
    {
        auto& zoomAcsr = mAccessor.getAccessor<AcsrType::binZoom>(0);
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
            case AttrType::zoomMode:
            case AttrType::colour:
            case AttrType::colourMap:
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
    
    addChildComponent(mValueRuler);
    addChildComponent(mValueScrollBar);
    addChildComponent(mBinRuler);
    addChildComponent(mBinScrollBar);
    addAndMakeVisible(mThumbnail);
    addAndMakeVisible(mInstantRenderer);
    addAndMakeVisible(mTimeRenderer);
    setSize(80, 100);
    
    mSeparator.addComponentListener(this);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Analyzer::Section::~Section()
{
    mAccessor.removeListener(mListener);
    mSeparator.removeComponentListener(this);
}

void Analyzer::Section::setTime(double time)
{
    mAccessor.setTime(time, NotificationType::synchronous);
}

void Analyzer::Section::resized()
{
    auto bounds = getLocalBounds();
    auto const leftSize = mSeparator.getScreenX() - getScreenX();
    auto const rightSize = getWidth() - leftSize - mSeparator.getWidth();
    
    auto leftSide = bounds.removeFromLeft(leftSize);
    mThumbnail.setBounds(leftSide.removeFromLeft(24));
    mInstantRenderer.setBounds(leftSide);
    
    auto rightSide = bounds.removeFromRight(rightSize);
    auto const scrollbarBounds = rightSide.removeFromRight(8);
    mValueScrollBar.setBounds(scrollbarBounds);
    mBinScrollBar.setBounds(scrollbarBounds);
    auto const rulerBounds = rightSide.removeFromRight(16);
    mValueRuler.setBounds(rulerBounds);
    mBinRuler.setBounds(rulerBounds);
    mTimeRenderer.setBounds(rightSide);
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
