#pragma once

#include "AnlBasics.h"
#include "AnlConcertinaTable.h"
#include "AnlPropertyComponent.h"

ANALYSE_FILE_BEGIN

class AudioFileInfoPanel
: public juce::Component
{
public:
    AudioFileInfoPanel();
    ~AudioFileInfoPanel() override = default;

    void setAudioFormatReader(juce::File const& file, juce::AudioFormatReader const* reader);
    juce::File getFile() const;

    // juce::Component
    void resized() override;

private:
    juce::File mFile;
    PropertyTextButton mFilePath;
    PropertyLabel mFileFormat{juce::translate("Format"), juce::translate("The format of the audio file")};
    PropertyLabel mSampleRate{juce::translate("Sample Rate"), juce::translate("The sample rate of the audio file")};

    PropertyLabel mBitPerSample{juce::translate("Bits"), juce::translate("The number of bits per samples of the audio file")};
    PropertyLabel mLengthInSamples{juce::translate("Length"), juce::translate("The length of the audio file in samples")};
    PropertyLabel mDurationInSeconds{juce::translate("Duration"), juce::translate("The duration of the audio file in seconds")};
    PropertyLabel mNumChannels{juce::translate("Channels"), juce::translate("The number of channels of the audio file")};

    std::vector<std::unique_ptr<PropertyLabel>> mMetaDataPanels;

    ConcertinaTable mConcertinaTable{"", false};
    juce::Viewport mViewport;
};

ANALYSE_FILE_END
