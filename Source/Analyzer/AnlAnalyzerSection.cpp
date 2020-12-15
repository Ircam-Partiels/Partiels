#include "AnlAnalyzerSection.h"

ANALYSE_FILE_BEGIN

Analyzer::Section::Section(Accessor& accessor, Zoom::Accessor& timeZoomAcsr, juce::Component& separator)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAcsr)
, mSeparator(separator)
{
    mThumbnail.onRemove = [&]()
    {
        if(onRemove != nullptr)
        {
            onRemove();
        }
    };
    
    mListener.onAccessorInserted = [&](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch (type)
        {
            case AcsrType::zoom:
            {
                mScrollbar.reset();
                mRuler.reset();
                auto& zoomAcsr = mAccessor.getAccessor<AcsrType::zoom>(index);
                mScrollbar = std::make_unique<Zoom::ScrollBar>(zoomAcsr, Zoom::ScrollBar::Orientation::vertical, true);
                if(mScrollbar != nullptr)
                {
                    addAndMakeVisible(mScrollbar.get());
                }
                mRuler = std::make_unique<Zoom::Ruler>(zoomAcsr, Zoom::Ruler::Orientation::vertical);
                if(mRuler != nullptr)
                {
                    addAndMakeVisible(mRuler.get());
                    mRuler->onDoubleClick = [&]()
                    {
                        auto const range = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
                        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
                    };
                }
                resized();
            }
                break;
        }
    };
    
    mListener.onAccessorErased = [&](Accessor const& acsr, AcsrType type, size_t index)
    {
        switch (type)
        {
            case AcsrType::zoom:
            {
                if(index > 0)
                {
                    mListener.onAccessorInserted(acsr, type, index-1);
                }
            }
                break;
        }
    };
    
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
    mInstantRenderer.setTime(time);
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
    if(mScrollbar != nullptr)
    {
        mScrollbar->setBounds(scrollbarBounds);
    }
    auto const rulerBounds = rightSide.removeFromRight(16);
    if(mRuler != nullptr)
    {
        mRuler->setBounds(rulerBounds);
    }
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
