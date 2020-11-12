#include "AnlZoomScrollBar.h"

ANALYSE_FILE_BEGIN

Zoom::ScrollBar::ScrollBar(Accessor& accessor, Orientation orientation)
: mAccessor(accessor)
, mScrollBar(orientation == Orientation::vertical)
{
    mScrollBar.setAutoHide(false);
    addAndMakeVisible(mScrollBar);
    mIncDec.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    mIncDec.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, false, 0, 0);
    addAndMakeVisible(mIncDec);
    
    mIncDec.onValueChange = [&]()
    {
        auto const range = mAccessor.getValue<AttrType::visibleRange>();
        auto const globalRange = mAccessor.getValue<AttrType::globalRange>();
        if(globalRange.getLength() - mIncDec.getValue() < mIncDec.getInterval())
        {
            mAccessor.setValue<AttrType::visibleRange>(globalRange.expanded(-mIncDec.getInterval()), NotificationType::synchronous);
        }
        else
        {
             mAccessor.setValue<AttrType::visibleRange>(range.expanded(-(mIncDec.getValue() - range.getLength())), NotificationType::synchronous);
        }
    };
    
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch (attribute)
        {
            case AttrType::globalRange:
            {
                auto const globalRange = acsr.getValue<AttrType::globalRange>();
                mScrollBar.setRangeLimits(globalRange, juce::NotificationType::dontSendNotification);
//                mIncDec.setRange(globalRange.movedToStartAt(0.0), globalRange.getLength() / 127.0);
            }
                break;
            case AttrType::visibleRange:
            {
                auto const range = acsr.getValue<AttrType::visibleRange>();
                mScrollBar.setCurrentRange(range, juce::NotificationType::dontSendNotification);
//                mIncDec.setValue(range.getLength(), juce::NotificationType::dontSendNotification);
            }
                break;
            
            case AttrType::minimumLength:
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
    auto bounds = getLocalBounds();
    if(mScrollBar.isVertical())
    {
        mIncDec.setBounds(bounds.removeFromBottom(bounds.getWidth() * 2));
    }
    else
    {
        mIncDec.setBounds(bounds.removeFromRight(bounds.getHeight() * 2));
    }
    mScrollBar.setBounds(bounds);
}

void Zoom::ScrollBar::scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
    juce::ignoreUnused(scrollBarThatHasMoved, newRangeStart);
    anlStrongAssert(scrollBarThatHasMoved == &mScrollBar);
    mAccessor.setValue<AttrType::visibleRange>(range_type{mScrollBar.getCurrentRange()}, NotificationType::synchronous);
}

ANALYSE_FILE_END
