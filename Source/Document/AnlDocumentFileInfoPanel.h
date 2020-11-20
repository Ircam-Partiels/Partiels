#pragma once

#include "AnlDocumentModel.h"
#include "../Tools/AnlPropertyLayout.h"
#include "../Tools/AnlColouredPanel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    //! @todo Implement tag system
    //! @todo Add popup window that diisplays the full text when the mouse is over after a few ms
    class FileInfoPanel
    : public juce::Component
    , private juce::ChangeListener
    {
    public:
        FileInfoPanel(Accessor& accessor, juce::FileBasedDocument& fileBasedDocument, juce::AudioFormatManager& audioFormatManager);
        ~FileInfoPanel() override;
        
        // juce::Component
        void resized() override;
        
    private:
        
        // juce::ChangeListener
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;
        
        Accessor& mAccessor;
        juce::AudioFormatManager& mAudioFormatManager;
        juce::FileBasedDocument& mFileBasedDocument;
        Accessor::Listener mListener;
        
        Tools::PropertyLabel mPanelProjectName {juce::translate("Project"), juce::translate("The name of the project")};
        Tools::PropertyLabel mPanelFilePath {juce::translate("File"), juce::translate("The path of the audio file")};
        Tools::PropertyLabel mPanelFileFormat {juce::translate("Format"), juce::translate("The format of the audio file")};
        Tools::PropertyLabel mPanelSampleRate {juce::translate("Sample Rate"), juce::translate("The sample rate of the audio file")};
        
        Tools::PropertyLabel mPanelBitPerSample {juce::translate("Bits"), juce::translate("The number of bits per samples of the audio file")};
        Tools::PropertyLabel mPanelLengthInSamples {juce::translate("Length"), juce::translate("The length of the audio file in samples")};
        Tools::PropertyLabel mPanelDurationInSeconds {juce::translate("Duration"), juce::translate("The duration of the audio file in seconds")};
        Tools::PropertyLabel mPanelNumChannels {juce::translate("Channels"), juce::translate("The number of channels of the audio file")};
        
        std::vector<std::unique_ptr<Tools::PropertyLabel>> mMetaDataPanels;
        
        Tools::PropertyLayout mPropertyLayout1;
        Tools::ColouredPanel mSeparator1;
        Tools::PropertyLayout mPropertyLayout2;
        Tools::ColouredPanel mSeparator2;
        Tools::PropertyLayout mPropertyLayout3;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileInfoPanel)
    };
}

ANALYSE_FILE_END
