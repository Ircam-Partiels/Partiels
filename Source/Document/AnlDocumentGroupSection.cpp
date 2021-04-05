#include "AnlDocumentGroupSection.h"

ANALYSE_FILE_BEGIN

Document::GroupSection::GroupSection(Accessor& accessor, juce::Component& separator)
: mAccessor(accessor)
, mSeparator(separator)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType type)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AttrType::file:
            case AttrType::layoutHorizontal:
                break;
            case AttrType::layoutVertical:
            {
                auto const size = mAccessor.getAttr<AttrType::layoutVertical>();
                setSize(getWidth(), size + 2);
            }
                break;
            case AttrType::layout:
            case AttrType::expanded:
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
        mAccessor.setAttr<AttrType::layoutVertical>(size, NotificationType::synchronous);
    };
    mResizerBarLeft.onMoved = onResizerMoved;
    mResizerBarRight.onMoved = onResizerMoved;
    
    addAndMakeVisible(mRuler);
    addAndMakeVisible(mScrollBar);
    addAndMakeVisible(mThumbnailDecoration);
    addAndMakeVisible(mSnapshotDecoration);
    addAndMakeVisible(mPlotDecoration);
    addAndMakeVisible(mResizerBarLeft);
    addAndMakeVisible(mResizerBarRight);
    setSize(80, 100);
    
    mBoundsListener.attachTo(mSeparator);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::GroupSection::~GroupSection()
{
    mAccessor.removeListener(mListener);
    mBoundsListener.detachFrom(mSeparator);
}

void Document::GroupSection::resized()
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
        mThumbnailDecoration.setBounds(leftSide.removeFromLeft(48));
        mSnapshotDecoration.setBounds(leftSide.withTrimmedLeft(1));
    }
    
    // Plot, Rulers and Scrollbars
    {
        auto rightSide = bounds.removeFromRight(rightSize);
        mScrollBar.setBounds(rightSide.removeFromRight(8).reduced(0, 4));
        mRuler.setBounds(rightSide.removeFromRight(16).reduced(0, 4));
        mPlotDecoration.setBounds(rightSide);
    }
}

ANALYSE_FILE_END
