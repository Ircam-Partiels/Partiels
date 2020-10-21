#pragma once

#include "AnlDocumentModel.h"
#include "../Tools/AnlPropertyLayout.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    //! @todo Implement tag system
    class FileInfoPanel
    : public juce::Component
    {
    public:
        FileInfoPanel(Accessor& accessor);
        ~FileInfoPanel() override;
        
        void resized() override;
    private:
        
        class Property
        : public Tools::PropertyPanel<juce::Label>
        {
        public:
            Property(juce::String const& text, juce::String const& tooltip);
            ~Property() override = default;
        };
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        Property mPanelFileName {juce::translate("Name"), juce::translate("The name of the audio file")};
        Property mPanelFilePath {juce::translate("Path"), juce::translate("The path of the audio file")};
        Property mPanelFileFormat {juce::translate("Format"), juce::translate("The format of the audio file")};
        Property mPanelSampleRate {juce::translate("Sample Rate"), juce::translate("The sample rate of the audio file")};
        
        Property mPanelBitPerSample {juce::translate("Bits"), juce::translate("The number of bits per samples of the audio file")};
        Property mPanelLengthInSamples {juce::translate("Length"), juce::translate("The length of the audio file in samples")};
        Property mPanelDurationInSeconds {juce::translate("Duration"), juce::translate("The duration of the audio file in seconds")};
        Property mPanelNumChannels {juce::translate("Channels"), juce::translate("The number of channels of the audio file")};
        
        std::vector<std::unique_ptr<Property>> mMetaDataPanels;
        
        Tools::PropertyLayout mPropertyLayout1;
        Tools::PropertyLayout mPropertyLayout2;
        Tools::PropertyLayout mPropertyLayout3;
        
        JUCE_LEAK_DETECTOR(FileInfoPanel)
    };
}

ANALYSE_FILE_END
