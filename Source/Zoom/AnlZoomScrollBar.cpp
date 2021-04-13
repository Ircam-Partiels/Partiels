#include "AnlZoomScrollBar.h"

ANALYSE_FILE_BEGIN

Zoom::ScrollBar::ScrollBar(Accessor& accessor, Orientation orientation, bool isInversed)
: mAccessor(accessor)
, mScrollBar(orientation == Orientation::vertical)
, mIsInversed(isInversed)
{
    mScrollBar.setAutoHide(false);
    addAndMakeVisible(mScrollBar);
    
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::globalRange:
            {
                auto const globalRange = acsr.getAttr<AttrType::globalRange>();
                mScrollBar.setRangeLimits(globalRange, juce::NotificationType::dontSendNotification);
                mScrollBar.setSingleStepSize(globalRange.getLength() / 127.0);
            }
            case AttrType::visibleRange:
            {
                auto const range = acsr.getAttr<AttrType::visibleRange>();
                if(mIsInversed)
                {
                    auto const globalRange = acsr.getAttr<AttrType::globalRange>();
                    mScrollBar.setCurrentRange({globalRange.getEnd() - range.getEnd(), globalRange.getEnd() - range.getStart()}, juce::NotificationType::dontSendNotification);
                }
                else
                {
                    mScrollBar.setCurrentRange(range, juce::NotificationType::dontSendNotification);
                }
                repaint();
            }
                break;
            
            case AttrType::minimumLength:
            case AttrType::anchor:
                break;
        }
    };
    
    mScrollBar.addListener(this);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Zoom::ScrollBar::~ScrollBar()
{
    mScrollBar.removeListener(this);
    mAccessor.removeListener(mListener);
}

void Zoom::ScrollBar::resized()
{
    mScrollBar.setBounds(getLocalBounds());
}

void Zoom::ScrollBar::scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
    juce::ignoreUnused(scrollBarThatHasMoved, newRangeStart);
    anlStrongAssert(scrollBarThatHasMoved == &mScrollBar);
    if(mIsInversed)
    {
        auto const globalRange = mAccessor.getAttr<AttrType::globalRange>();
        auto const range = mScrollBar.getCurrentRange();
        mAccessor.setAttr<AttrType::visibleRange>(Range{globalRange.getEnd() - range.getEnd(), globalRange.getEnd() - range.getStart()}, NotificationType::synchronous);
    }
    else
    {
        mAccessor.setAttr<AttrType::visibleRange>(mScrollBar.getCurrentRange(), NotificationType::synchronous);
    }
}

ANALYSE_FILE_END
