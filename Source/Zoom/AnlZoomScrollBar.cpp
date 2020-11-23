#include "AnlZoomScrollBar.h"

ANALYSE_FILE_BEGIN

Zoom::ScrollBar::ScrollBar(Accessor& accessor, Orientation orientation, bool isInversed)
: mAccessor(accessor)
, mScrollBar(orientation == Orientation::vertical)
, mIsInversed(isInversed)
{
    mScrollBar.setAutoHide(false);
    addAndMakeVisible(mScrollBar);
    
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch (attribute)
        {
            case AttrType::globalRange:
            {
                auto const globalRange = acsr.getValue<AttrType::globalRange>();
                mScrollBar.setRangeLimits(globalRange, juce::NotificationType::dontSendNotification);
                mScrollBar.setSingleStepSize(globalRange.getLength() / 127.0);
            }
                break;
            case AttrType::visibleRange:
            {
                auto const range = acsr.getValue<AttrType::visibleRange>();
                if(mIsInversed)
                {
                    auto const globalRange = acsr.getValue<AttrType::globalRange>();
                    mScrollBar.setCurrentRange({globalRange.getEnd() - range.getEnd(), globalRange.getEnd() - range.getStart()}, juce::NotificationType::dontSendNotification);
                }
                else
                {
                    mScrollBar.setCurrentRange(range, juce::NotificationType::dontSendNotification);
                }
            }
                break;
            
            case AttrType::minimumLength:
            {
            }
                break;
        }
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mScrollBar.addListener(this);
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
        auto const globalRange = mAccessor.getValue<AttrType::globalRange>();
        auto const range = mScrollBar.getCurrentRange();
        mAccessor.setValue<AttrType::visibleRange>(range_type{globalRange.getEnd() - range.getEnd(), globalRange.getEnd() - range.getStart()}, NotificationType::synchronous);
    }
    else
    {
        mAccessor.setValue<AttrType::visibleRange>(mScrollBar.getCurrentRange(), NotificationType::synchronous);
    }
}

ANALYSE_FILE_END
