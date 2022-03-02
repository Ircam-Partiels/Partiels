#pragma once

#include "AnlTrackDirector.h"
#include "AnlTrackProgressBar.h"
#include "AnlTrackPropertyGraphicalSection.h"
#include "AnlTrackPropertyPluginSection.h"

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
        void lookAndFeelChanged() override;

    private:
        bool canModifyProcessor();
        void applyParameterValue(Plugin::Parameter const& parameter, float value);
        void updatePresets();

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        ComponentListener mComponentListener;

        PropertyText mPropertyName;

        PropertyTextButton mPropertyResultsFile;
        juce::TextEditor mPropertyResultsFileInfo;
        PropertyList mPropertyWindowType;
        PropertyList mPropertyBlockSize;
        PropertyList mPropertyStepSize;
        std::map<std::string, std::unique_ptr<juce::Component>> mParameterProperties;
        PropertyList mPropertyPreset;
        ProgressBar mProgressBarAnalysis{mAccessor, ProgressBar::Mode::analysis};

        ConcertinaTable mProcessorSection{juce::translate("PROCESSOR"), true,
                                          juce::translate("The processor parameters of the track")};

        PropertyGraphicalSection mPropertyGraphicalSection{mDirector};
        ConcertinaTable mGraphicalSection{juce::translate("GRAPHICAL"), true,
                                          juce::translate("The graphical parameters of the track")};

        PropertyPluginSection mPropertyPluginSection{mDirector};
        ConcertinaTable mPluginSection{juce::translate("PLUGIN"), true,
                                       juce::translate("The plugin information")};

        juce::Viewport mViewport;
        std::unique_ptr<juce::FileChooser> mFileChooser;
        static auto constexpr sInnerWidth = 300;
    };
} // namespace Track

ANALYSE_FILE_END
