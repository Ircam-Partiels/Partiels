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
                setSize(getWidth(), size + 1);
            }
                break;
            case AttrType::layout:
            case AttrType::expanded:
                break;
        }
    };
    
    mResizerBar.onMoved = [&](int size)
    {
        mAccessor.setAttr<AttrType::layoutVertical>(size, NotificationType::synchronous);
    };
    
    addAndMakeVisible(mRuler);
    addAndMakeVisible(mScrollBar);
    addAndMakeVisible(mThumbnailDecoration);
    addAndMakeVisible(mSnapshotDecoration);
    addAndMakeVisible(mPlotDecoration);
    addAndMakeVisible(mResizerBar);
    setSize(80, 100);
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::GroupSection::~GroupSection()
{
    mAccessor.removeListener(mListener);
}

void Document::GroupSection::resized()
{
    mResizerBar.setBounds(getLocalBounds().removeFromBottom(1).reduced(2, 0));
    
    auto bounds = getLocalBounds();
    mThumbnailDecoration.setBounds(bounds.removeFromLeft(48));
    mSnapshotDecoration.setBounds(bounds.removeFromLeft(48));
    
    mScrollBar.setBounds(bounds.removeFromRight(8));
    mRuler.setBounds(bounds.removeFromRight(16));
    mPlotDecoration.setBounds(bounds);
}

ANALYSE_FILE_END
