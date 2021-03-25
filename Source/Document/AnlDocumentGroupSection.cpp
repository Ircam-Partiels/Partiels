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
            case AttrType::playheadPosition:
            {
                if(mZoomPlayhead.isVisible())
                {
                    mZoomPlayhead.setPosition(acsr.getAttr<AttrType::playheadPosition>());
                }
            }
                break;
            case AttrType::groups:
            {
                repaint();
            }
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

void Document::GroupSection::Container::paint(juce::Graphics& g)
{
    auto const& groups = mAccessor.getAttr<AttrType::groups>();
    if(mIndex < groups.size())
    {
        g.fillAll(groups[mIndex].colour);
    }
}

Document::GroupSection::GroupSection(Accessor& accessor, size_t index, juce::Component& separator)
: mAccessor(accessor)
, mIndex(index)
, mSeparator(separator)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType type)
    {
        switch(type)
        {
            case AttrType::groups:
            {
                auto const& groups = mAccessor.getAttr<AttrType::groups>();
                if(mIndex < groups.size())
                {
                    setSize(getWidth(), groups[mIndex].height + 2);
                }
                
            }
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
        auto groups = mAccessor.getAttr<AttrType::groups>();
        if(mIndex < groups.size())
        {
            groups[mIndex].height = size;
            mAccessor.setAttr<AttrType::groups>(groups, NotificationType::synchronous);
        }
    };
    mResizerBarLeft.onMoved = onResizerMoved;
    mResizerBarRight.onMoved = onResizerMoved;
    
    addAndMakeVisible(mRuler);
    addAndMakeVisible(mScrollBar);
    addAndMakeVisible(mThumbnail);
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

juce::String Document::GroupSection::getIdentifier() const
{
    auto const& groups = mAccessor.getAttr<AttrType::groups>();
    if(mIndex < groups.size())
    {
        return groups[mIndex].identifier;
    }
    return "";
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
        mThumbnail.setBounds(leftSide.removeFromLeft(48));
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
