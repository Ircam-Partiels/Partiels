#include "AnlZoomStateTimeRuler.h"

ANALYSE_FILE_BEGIN

Zoom::TimeRuler::TimeRuler(Accessor& accessor)
: mPrimaryRuler(accessor, Ruler::Orientation::horizontal)
, mSecondaryRuler(accessor, Ruler::Orientation::horizontal)
{
    mPrimaryRuler.setPrimaryTickInterval(0);
    mPrimaryRuler.setTickReferenceValue(0.0);
    mPrimaryRuler.setTickPowerInterval(10.0, 2.0);
    mPrimaryRuler.setMaximumStringWidth(65.0);
    mPrimaryRuler.setValueAsStringMethod([](double value)
    {
        auto time = value;
        auto const hours = static_cast<int>(std::floor(time / 3600.0));
        time -= static_cast<double>(hours) * 3600.0;
        auto const minutes = static_cast<int>(std::floor(time / 60.0));
        time -= static_cast<double>(minutes) * 60.0;
        auto const seconds = static_cast<int>(std::floor(time));
        time -= static_cast<double>(seconds);
        auto const ms = static_cast<int>(std::floor(time * 1000.0));
        
        return juce::String::formatted("%02d:%02d:%02d:%03d", hours, minutes, seconds, ms);
    });
    
    mPrimaryRuler.onDoubleClick = [this]()
    {
        if(onDoubleClick != nullptr)
        {
            onDoubleClick();
        }
    };
    addAndMakeVisible(mPrimaryRuler);
    
    mSecondaryRuler.setPrimaryTickInterval(0);
    mSecondaryRuler.setTickReferenceValue(0.0);
    mSecondaryRuler.setTickPowerInterval(10.0, 2.0);
    mSecondaryRuler.setMaximumStringWidth(65.0);
    mSecondaryRuler.setValueAsStringMethod([](double value)
    {
        return juce::String(value, 3);
    });

    mSecondaryRuler.onDoubleClick = [this]()
    {
        if(onDoubleClick != nullptr)
        {
            onDoubleClick();
        }
    };
    addAndMakeVisible(mSecondaryRuler);
}

void Zoom::TimeRuler::paint(juce::Graphics &g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
}

void Zoom::TimeRuler::resized()
{
    auto bounds = getLocalBounds();
    auto const rulerHeight = bounds.getHeight() / 2;
    mPrimaryRuler.setBounds(bounds.removeFromTop(rulerHeight));
    mSecondaryRuler.setBounds(bounds.removeFromTop(rulerHeight));
}
ANALYSE_FILE_END
