#pragma once

#include "AnlTrackDirector.h"
#include "AnlTrackModel.h"
#include "AnlTrackProgressBar.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class PropertyPanel
    : public FloatingWindowContainer
    , public juce::DragAndDropContainer
    {
    public:
        PropertyPanel(Director& director);
        ~PropertyPanel() override;

        // juce::Component
        void resized() override;

    private:
        bool canModifyProcessor();
        void applyParameterValue(Plugin::Parameter const& parameter, float value);
        void updatePresets();
        void updateZoomMode();
        void showChannelLayout();

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        Zoom::Accessor::Listener mValueZoomListener{typeid(*this).name()};
        Zoom::Accessor::Listener mBinZoomListener{typeid(*this).name()};
        Zoom::Grid::Accessor::Listener mGridListener{typeid(*this).name()};
        BoundsListener mBoundsListener;

        PropertyText mPropertyName;

        PropertyTextButton mPropertyResultsFile;
        juce::TextEditor mPropertyResultsFileInfo;
        PropertyList mPropertyWindowType;
        PropertyList mPropertyWindowSize;
        PropertyList mPropertyWindowOverlapping;
        PropertyNumber mPropertyBlockSize;
        PropertyNumber mPropertyStepSize;
        std::map<std::string, std::unique_ptr<juce::Component>> mParameterProperties;
        PropertyList mPropertyPreset;
        ProgressBar mProgressBarAnalysis{mAccessor, ProgressBar::Mode::analysis};

        PropertyList mPropertyColourMap;
        PropertyColourButton mPropertyForegroundColour;
        PropertyColourButton mPropertyBackgroundColour;
        PropertyColourButton mPropertyTextColour;
        PropertyColourButton mPropertyShadowColour;
        PropertyList mPropertyValueRangeMode;
        PropertyNumber mPropertyValueRangeMin;
        PropertyNumber mPropertyValueRangeMax;
        PropertyRangeSlider mPropertyValueRange;
        PropertyNumber mPropertyTickReference;
        PropertyList mPropertyTickBase;
        PropertyToggle mPropertyRangeLink;
        PropertyNumber mPropertyNumBins;
        PropertyTextButton mPropertyChannelLayout;
        ProgressBar mProgressBarRendering{mAccessor, ProgressBar::Mode::rendering};

        PropertyLabel mPropertyPluginName{"Name", "The name of the plugin"};
        PropertyLabel mPropertyPluginFeature{"Feature", "The feature of the plugin"};
        PropertyLabel mPropertyPluginMaker{"Maker", "The maker of the plugin"};
        PropertyLabel mPropertyPluginVersion{"Version", "The version of the plugin"};
        PropertyLabel mPropertyPluginCategory{"Category", "The category of the plugin"};
        juce::TextEditor mPropertyPluginDetails;

        ConcertinaTable mProcessorSection{juce::translate("PROCESSOR"), true,
                                          juce::translate("The processor parameters of the track")};
        ConcertinaTable mGraphicalSection{juce::translate("GRAPHICAL"), true,
                                          juce::translate("The graphical parameters of the track")};
        ConcertinaTable mPluginSection{juce::translate("PLUGIN"), true,
                                       juce::translate("The plugin information")};

        using GridBaseInfo = std::tuple<size_t, double, double>;
        static constexpr std::array<GridBaseInfo, 6> sGridBaseInfoArray{
            GridBaseInfo{0_z, 10.0, 4.0}, GridBaseInfo{0_z, 2.0, 5.0}, GridBaseInfo{3_z, 2.0, 5.0}, GridBaseInfo{0_z, 4.0, 4.0}, GridBaseInfo{0_z, 10.0, 2.0}, GridBaseInfo{0_z, 6.0, 5.0}};
        juce::Viewport mViewport;
        bool mChannelLayoutActionStarted{false};
        static auto constexpr sInnerWidth = 300;
    };
} // namespace Track

ANALYSE_FILE_END
