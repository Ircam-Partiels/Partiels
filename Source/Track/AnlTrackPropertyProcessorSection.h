#pragma once

#include "AnlTrackProgressBar.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class PropertyProcessorSection
    : public juce::Component
    {
    public:
        PropertyProcessorSection(Director& director);
        ~PropertyProcessorSection() override;

        // juce::Component
        void resized() override;

    private:
        void askToModifyProcessor(std::function<bool(bool)> prepare, std::function<void(void)> perform);
        void applyParameterValue(Plugin::Parameter const& parameter, float value);
        void setInputTrack(juce::String const& identifier);
        void setWindowType(Plugin::WindowType const& windowType);
        void setBlockSize(size_t const blockSize);
        void setStepSize(size_t const stepSize);
        void restoreDefaultPreset();
        void loadPreset();
        void savePreset();
        void changePreset(size_t index);
        void updateState();

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        HierarchyManager::Listener mHierarchyListener;

        PropertyTextButton mPropertyResultsFile;
        juce::Label mPropertyResultsFileInfo;
        PropertyList mPropertyWindowType;
        PropertyList mPropertyBlockSize;
        PropertyList mPropertyStepSize;
        PropertyList mPropertyInputTrack;
        std::map<int, juce::String> mPropertyInputTrackList;
        std::map<std::string, std::unique_ptr<juce::Component>> mParameterProperties;
        PropertyList mPropertyPreset;
        ProgressBar mProgressBarAnalysis{mDirector, ProgressBar::Mode::analysis};

        std::unique_ptr<juce::FileChooser> mFileChooser;
    };
} // namespace Track

ANALYSE_FILE_END
