#include "AnlDocumentGroupSection.h"

ANALYSE_FILE_BEGIN

Document::GroupSection::GroupSection(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType type)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AttrType::file:
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
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::GroupSection::~GroupSection()
{
    mAccessor.removeListener(mListener);
}

void Document::GroupSection::resized()
{
    auto bounds = getLocalBounds();
    auto resizersBounds = bounds.removeFromBottom(2);
    
    mThumbnailDecoration.setBounds(bounds.removeFromLeft(48));
    bounds.removeFromLeft(1);
    mSnapshotDecoration.setBounds(bounds.removeFromLeft(48));
    bounds.removeFromLeft(1);
    mResizerBarLeft.setBounds(resizersBounds.removeFromLeft(bounds.getX()).reduced(2, 0));
    
    mScrollBar.setBounds(bounds.removeFromRight(8));
    mRuler.setBounds(bounds.removeFromRight(16));
    mPlotDecoration.setBounds(bounds);
    mResizerBarRight.setBounds(resizersBounds.reduced(2, 0));
}

ANALYSE_FILE_END
