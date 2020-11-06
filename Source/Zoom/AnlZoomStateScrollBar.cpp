#include "AnlZoomStateScrollBar.h"

ANALYSE_FILE_BEGIN

Zoom::State::ScrollBar::ScrollBar(Accessor& accessor, Orientation orientation)
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
        mAccessor.setValue<AttrType::visibleRange>(range.expanded(mIncDec.getValue() - range.getLength()), NotificationType::synchronous);
    };
    
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        if(attribute == AttrType::visibleRange)
        {
            auto const globalRange = acsr.getValue<AttrType::globalRange>();
            mScrollBar.setRangeLimits(globalRange, juce::NotificationType::dontSendNotification);
            mIncDec.setRange(globalRange.movedToStartAt(0.0), globalRange.getLength() / 127.0);
            auto const range = acsr.getValue<AttrType::visibleRange>();
            mScrollBar.setCurrentRange(range, juce::NotificationType::dontSendNotification);
            mIncDec.setValue(range.getLength());
        }
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mScrollBar.addListener(this);
}

Zoom::State::ScrollBar::~ScrollBar()
{
    mScrollBar.removeListener(this);
    mAccessor.removeListener(mListener);
}

void Zoom::State::ScrollBar::resized()
{
    auto const globalRange = mAccessor.getValue<AttrType::globalRange>();
    auto bounds = getLocalBounds();
    if(mScrollBar.isVertical())
    {
        mIncDec.setBounds(bounds.removeFromBottom(bounds.getWidth() * 2));
        mIncDec.setRange(globalRange.movedToStartAt(0.0), globalRange.getLength() / static_cast<double>(bounds.getHeight()));
    }
    else
    {
        mIncDec.setBounds(bounds.removeFromRight(bounds.getHeight() * 2));
        mIncDec.setRange(globalRange.movedToStartAt(0.0), globalRange.getLength() / static_cast<double>(bounds.getWidth()));
    }
    mScrollBar.setBounds(bounds);
}

void Zoom::State::ScrollBar::scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
    juce::ignoreUnused(scrollBarThatHasMoved, newRangeStart);
    anlStrongAssert(scrollBarThatHasMoved == &mScrollBar);
    mAccessor.setValue<AttrType::visibleRange>(range_type{mScrollBar.getCurrentRange()}, NotificationType::synchronous);
}

ANALYSE_FILE_END
