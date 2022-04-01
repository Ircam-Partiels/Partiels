#pragma once

#include "AnlTrackProgressBar.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class PropertyGraphicalSection
    : public juce::Component
    {
    public:
        PropertyGraphicalSection(Director& director);
        ~PropertyGraphicalSection() override;

        // juce::Component
        void resized() override;

    private:
        Zoom::Accessor& getCurrentZoomAcsr();
        void setColourMap(ColourMap const& colourMap);
        void setForegroundColour(juce::Colour const& colour);
        void setBackgroundColour(juce::Colour const& colour);
        void setTextColour(juce::Colour const& colour);
        void setShadowColour(juce::Colour const& colour);
        void setPluginValueRange();
        void setResultValueRange();
        void setValueRangeMin(double value);
        void setValueRangeMax(double value);
        void setValueRange(juce::Range<double> const& range);
        void showChannelLayout();
        void updateZoomMode();

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        Zoom::Accessor::Listener mValueZoomListener{typeid(*this).name()};
        Zoom::Accessor::Listener mBinZoomListener{typeid(*this).name()};

        PropertyList mPropertyColourMap;
        PropertyColourButton mPropertyForegroundColour;
        PropertyColourButton mPropertyBackgroundColour;
        PropertyColourButton mPropertyTextColour;
        PropertyColourButton mPropertyShadowColour;
        PropertyList mPropertyValueRangeMode;
        PropertyNumber mPropertyValueRangeMin;
        PropertyNumber mPropertyValueRangeMax;
        PropertyRangeSlider mPropertyValueRange;
        PropertyTextButton mPropertyGrid;
        PropertyToggle mPropertyRangeLink;
        PropertyNumber mPropertyNumBins;
        PropertyTextButton mPropertyChannelLayout;
        ProgressBar mProgressBarRendering{mDirector, ProgressBar::Mode::rendering};

        Zoom::Grid::PropertyPanel mZoomGridPropertyPanel;
        bool mChannelLayoutActionStarted{false};
    };
} // namespace Track

ANALYSE_FILE_END
