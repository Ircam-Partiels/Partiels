#pragma once

#include "AnlTrackProgressBar.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class PropertyGraphicalSection
    : public juce::Component
    {
    public:
        PropertyGraphicalSection(Director& director, PresetList::Accessor& presetListAcsr);
        ~PropertyGraphicalSection() override;

        // juce::Component
        void resized() override;

    private:
        Zoom::Accessor& getCurrentZoomAcsr();
        void addExtraThresholdProperties();
        void setColourMap(ColourMap const& colourMap);
        void setForegroundColour(juce::Colour const& colour);
        void setDurationColour(juce::Colour const& colour);
        void setBackgroundColour(juce::Colour const& colour);
        void setTextColour(juce::Colour const& colour);
        void setShadowColour(juce::Colour const& colour);
        void setUnit(juce::String const& unit);
        void setLabelJustification(LabelLayout::Justification justification);
        void setLabelPosition(float position);
        void setPluginValueRange();
        void setResultValueRange();
        void setValueRangeMin(double value);
        void setValueRangeMax(double value);
        void setValueRange(juce::Range<double> const& range);
        void showChannelLayout();
        void updateZoomMode();
        void updateExtraTheshold();
        void updateGridPanel();
        void loadGraphicsPreset();
        void saveGraphicsPreset();
        void saveAsDefaultGraphicsPreset();
        void deleteDefaultGraphicsPreset();
        void updateGraphicsPresetState();
        void syncGraphicsSettingsFromAttributes();

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        PresetList::Accessor& mPresetListAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        Zoom::Accessor::Listener mValueZoomListener{typeid(*this).name()};
        Zoom::Accessor::Listener mBinZoomListener{typeid(*this).name()};

        Zoom::Grid::PropertyPanel mZoomGridPropertyPanel;
        FloatingWindowContainer mZoomGridPropertyWindow;

        PropertyList mPropertyColourMap;
        PropertyColourButton mPropertyForegroundColour;
        PropertyColourButton mPropertyDurationColour;
        PropertyColourButton mPropertyBackgroundColour;
        PropertyColourButton mPropertyTextColour;
        PropertyColourButton mPropertyShadowColour;
        PropertyList mPropertyFontName;
        PropertyList mPropertyFontStyle;
        PropertyList mPropertyFontSize;
        PropertyNumber mPropertyLineWidth;
        PropertyText mPropertyUnit;
        PropertyList mPropertyLabelJustification;
        PropertyNumber mPropertyLabelPosition;
        PropertyList mPropertyValueRangeMode;
        PropertyNumber mPropertyValueRangeMin;
        PropertyNumber mPropertyValueRangeMax;
        PropertyRangeSlider mPropertyValueRange;
        PropertyToggle mPropertyValueRangeLogScale;
        std::vector<std::unique_ptr<PropertySlider>> mPropertyExtraThresholds;
        PropertyTextButton mPropertyGrid;
        PropertyToggle mPropertyRangeLink;
        PropertyNumber mPropertyNumBins;
        PropertyTextButton mPropertyChannelLayout;
        PropertyToggle mPropertyShowInGroup;
        PropertyList mPropertyGraphicsPreset;
        ProgressBar mProgressBarRendering{mDirector, ProgressBar::Mode::rendering};

        std::unique_ptr<juce::FileChooser> mFileChooser;
        bool mChannelLayoutActionStarted{false};

        enum MenuGraphicsPresetId : int
        {
            loadPresetId = 1,
            savePresetId,
            saveDefaultPresetId,
            deleteDefaultPresetId
        };
    };
} // namespace Track

ANALYSE_FILE_END
