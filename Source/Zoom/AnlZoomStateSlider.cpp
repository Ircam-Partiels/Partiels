#include "AnlZoomStateSlider.h"

ANALYSE_FILE_BEGIN

Zoom::State::Slider::Slider(Accessor& accessor, Orientation orientation)
: mAccessor(accessor)
{
    using SliderStyle = juce::Slider::SliderStyle;
    using TextEntryBoxPosition = juce::Slider::TextEntryBoxPosition;
    
    mSlider.setSliderStyle(orientation == Orientation::horizontal ? SliderStyle::TwoValueHorizontal : SliderStyle::TwoValueVertical);
    mSlider.setTextBoxStyle(TextEntryBoxPosition::NoTextBox, false, 0, 0);
    mSlider.setScrollWheelEnabled(true);
    mSlider.setIncDecButtonsMode(juce::Slider::IncDecButtonMode::incDecButtonsDraggable_AutoDirection);
    addAndMakeVisible(mSlider);
    
    mIncDec.setSliderStyle(SliderStyle::IncDecButtons);
    mIncDec.setTextBoxStyle(TextEntryBoxPosition::NoTextBox, false, 0, 0);
    addAndMakeVisible(mIncDec);
    
    mSlider.onValueChange = [&]()
    {
        mAccessor.fromModel({{mSlider.getMinValue(), mSlider.getMaxValue()}}, juce::NotificationType::sendNotificationSync);
    };
    
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
            mSlider.setRange(globalRange, 0.0);
            mIncDec.setRange(globalRange.movedToStartAt(0.0), globalRange.getLength() / 127.0);
            auto const range = acsr.getModel().range;
            mSlider.setMinAndMaxValues(range.getStart(), range.getEnd(), juce::NotificationType::dontSendNotification);
            mIncDec.setValue(range.getLength());
        }
    };
    
    mAccessor.addListener(mListener, juce::NotificationType::sendNotificationSync);
}

Zoom::State::Slider::~Slider()
{
    mAccessor.removeListener(mListener);
}

void Zoom::State::Slider::resized()
{
    auto const globalRange = std::get<0>(mAccessor.getContraints());
    auto bounds = getLocalBounds();
    if(mSlider.isVertical())
    {
        mIncDec.setBounds(bounds.removeFromBottom(bounds.getWidth() * 2));
        mIncDec.setRange(globalRange.movedToStartAt(0.0), globalRange.getLength() / static_cast<double>(bounds.getHeight()));
    }
    else
    {
        mIncDec.setBounds(bounds.removeFromRight(bounds.getHeight() * 2));
        mIncDec.setRange(globalRange.movedToStartAt(0.0), globalRange.getLength() / static_cast<double>(bounds.getWidth()));
    }
    mSlider.setBounds(bounds);
}

ANALYSE_FILE_END
