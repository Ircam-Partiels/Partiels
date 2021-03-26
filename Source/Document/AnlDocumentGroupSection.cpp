#include "AnlDocumentGroupSection.h"

ANALYSE_FILE_BEGIN

Document::GroupSection::Container::Container(Accessor& accessor, size_t index, juce::Component& content, bool showPlayhead)
: mAccessor(accessor)
, mIndex(index)
, mContent(content)
, mZoomPlayhead(mAccessor.getAccessor<AcsrType::timeZoom>(0))
{
    addAndMakeVisible(mContent);
    if(showPlayhead)
    {
        addAndMakeVisible(mZoomPlayhead);
    }
    
    mListener.onAttrChanged = [=, this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::file:
            case AttrType::isLooping:
            case AttrType::gain:
            case AttrType::isPlaybackStarted:
                break;
            case AttrType::playheadPosition:
            {
                if(mZoomPlayhead.isVisible())
                {
                    mZoomPlayhead.setPosition(acsr.getAttr<AttrType::playheadPosition>());
                }
            }
                break;
            case AttrType::layoutHorizontal:
            case AttrType::layoutVertical:
            case AttrType::layout:
            case AttrType::expanded:
                break;
        }
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::GroupSection::Container::~Container()
{
    mAccessor.removeListener(mListener);
}

void Document::GroupSection::Container::resized()
{
    auto bounds = getLocalBounds();
    mContent.setBounds(bounds);
}

Document::GroupSection::GroupSection(Accessor& accessor, size_t index, juce::Component& separator)
: mAccessor(accessor)
, mIndex(index)
, mSeparator(separator)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType type)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AttrType::file:
            case AttrType::isLooping:
            case AttrType::gain:
            case AttrType::isPlaybackStarted:
            case AttrType::playheadPosition:
                break;
            case AttrType::layoutHorizontal:
            {
                auto const size = mAccessor.getAttr<AttrType::layoutHorizontal>();
                setSize(getWidth(), size + 2);
            }
                break;
            case AttrType::layoutVertical:
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
        mAccessor.setAttr<AttrType::layoutHorizontal>(size, NotificationType::synchronous);
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

void Document::GroupSection::paint(juce::Graphics& g)
{
    juce::ignoreUnused(g);
}

ANALYSE_FILE_END
