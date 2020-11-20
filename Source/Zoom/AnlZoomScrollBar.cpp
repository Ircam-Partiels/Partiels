#include "AnlZoomScrollBar.h"

ANALYSE_FILE_BEGIN

Zoom::ScrollBar::ScrollBar(Accessor& accessor, Orientation orientation)
: mAccessor(accessor)
, mScrollBar(orientation == Orientation::vertical)
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
                mScrollBar.setCurrentRange(range, juce::NotificationType::dontSendNotification);
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
    mAccessor.setValue<AttrType::visibleRange>(range_type{mScrollBar.getCurrentRange()}, NotificationType::synchronous);
}

ANALYSE_FILE_END
