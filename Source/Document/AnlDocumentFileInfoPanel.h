#pragma once

#include "AnlDocumentModel.h"
#include "../Tools/AnlPropertyLayout.h"

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
        
        class Property
        : public Tools::PropertyPanel<juce::Label>
        {
        public:
            Property(juce::String const& text, juce::String const& tooltip);
            ~Property() override = default;
        };
        
        Accessor& mAccessor;
        juce::AudioFormatManager& mAudioFormatManager;
        juce::FileBasedDocument& mFileBasedDocument;
        Accessor::Listener mListener;
        
        Property mPanelFileName {juce::translate("Project"), juce::translate("The name of the project")};
        Property mPanelFilePath {juce::translate("File"), juce::translate("The path of the audio file")};
        Property mPanelFileFormat {juce::translate("Format"), juce::translate("The format of the audio file")};
        Property mPanelSampleRate {juce::translate("Sample Rate"), juce::translate("The sample rate of the audio file")};
        
        Property mPanelBitPerSample {juce::translate("Bits"), juce::translate("The number of bits per samples of the audio file")};
        Property mPanelLengthInSamples {juce::translate("Length"), juce::translate("The length of the audio file in samples")};
        Property mPanelDurationInSeconds {juce::translate("Duration"), juce::translate("The duration of the audio file in seconds")};
        Property mPanelNumChannels {juce::translate("Channels"), juce::translate("The number of channels of the audio file")};
        
        std::vector<std::unique_ptr<Property>> mMetaDataPanels;
        
        Tools::PropertyLayout mPropertyLayout1;
        Tools::ColouredPanel mSeparator1;
        Tools::PropertyLayout mPropertyLayout2;
        Tools::ColouredPanel mSeparator2;
        Tools::PropertyLayout mPropertyLayout3;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileInfoPanel)
    };
}

ANALYSE_FILE_END
