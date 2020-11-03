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
        auto copy = mAccessor.getModel();
        copy.range = copy.range.expanded(mIncDec.getValue() - copy.range.getLength());
        mAccessor.fromModel(copy, juce::NotificationType::sendNotificationSync);
    };
    
    mListener.onChanged = [&](Accessor const& acsr, Attribute attribute)
    {
        if(attribute == Zoom::State::Model::Attribute::range)
        {
            auto const globalRange = std::get<0>(acsr.getContraints());
            mScrollBar.setRangeLimits(globalRange, juce::NotificationType::dontSendNotification);
            mIncDec.setRange(globalRange.movedToStartAt(0.0), globalRange.getLength() / 127.0);
            auto const range = acsr.getModel().range;
            mScrollBar.setCurrentRange(range, juce::NotificationType::dontSendNotification);
            mIncDec.setValue(range.getLength());
        }
    };
    
    mAccessor.addListener(mListener, juce::NotificationType::sendNotificationSync);
    mScrollBar.addListener(this);
}

Zoom::State::ScrollBar::~ScrollBar()
{
    mScrollBar.removeListener(this);
    mAccessor.removeListener(mListener);
}

void Zoom::State::ScrollBar::resized()
{
    auto const globalRange = std::get<0>(mAccessor.getContraints());
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
    mAccessor.fromModel({mScrollBar.getCurrentRange()}, juce::NotificationType::sendNotificationSync);
}

ANALYSE_FILE_END
