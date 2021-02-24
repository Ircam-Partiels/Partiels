#include "AnlGroupSection.h"

ANALYSE_FILE_BEGIN

Group::Section::Section(Accessor& accessor, Zoom::Accessor& timeZoomAcsr, juce::Component& separator)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAcsr)
, mSeparator(separator)
{
//    mThumbnail.onRemove = [&]()
//    {
//        if(onRemove != nullptr)
//        {
//            onRemove();
//        }
//    };
    
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType type)
    {
        switch(type)
        {
            case AttrType::name:
                break;
            case AttrType::height:
            {
                setSize(getWidth(), acsr.getAttr<AttrType::height>() + 2);
            }
                break;
            case AttrType::layout:
                break;
}
    };
    
    mBoundsListener.onComponentMoved = [&](juce::Component& component)
    {
        anlStrongAssert(&component == &mSeparator);
        if(&component == &mSeparator)
        {
            resized();
        }
    };
    
    auto onResizerMoved = [&](int size)
    {
        mAccessor.setAttr<AttrType::height>(size, NotificationType::synchronous);
    };
    mResizerBarLeft.onMoved = onResizerMoved;
    mResizerBarRight.onMoved = onResizerMoved;
    
//    addAndMakeVisible(mThumbnail);
//    addAndMakeVisible(mSnapshot);
    addAndMakeVisible(mPlot);
    addAndMakeVisible(mResizerBarLeft);
    addAndMakeVisible(mResizerBarRight);
    setSize(80, 100);
    
    mBoundsListener.attachTo(mSeparator);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::Section::~Section()
{
    mAccessor.removeListener(mListener);
    mBoundsListener.detachFrom(mSeparator);
}

void Group::Section::resized()
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
//    {
//        auto leftSide = bounds.removeFromLeft(leftSize);
//        mThumbnail.setBounds(leftSide.removeFromLeft(48));
//        mSnapshot.setBounds(leftSide.withTrimmedLeft(1));
//    }
    
    // Plot, Rulers and Scrollbars
    {
        auto rightSide = bounds.removeFromRight(rightSize);
//        auto const scrollbarBounds = rightSide.removeFromRight(8).reduced(0, 4);
//        mValueScrollBar.setBounds(scrollbarBounds);
//        mBinScrollBar.setBounds(scrollbarBounds);
//        auto const rulerBounds = rightSide.removeFromRight(16).reduced(0, 4);
//        mValueRuler.setBounds(rulerBounds);
//        mBinRuler.setBounds(rulerBounds);
        mPlot.setBounds(rightSide);
    }
}

void Group::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::sectionColourId, true));
}

ANALYSE_FILE_END
