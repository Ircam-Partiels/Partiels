#include "AnlAudioFileInfoPanel.h"

ANALYSE_FILE_BEGIN

AudioFileInfoPanel::AudioFileInfoPanel()
: mFilePath("File", "Path", [this]()
            {
                if(juce::File::isAbsolutePath(mFilePath.entry.getTooltip()))
                {
                    juce::File(mFilePath.entry.getTooltip()).revealToUser();
                }
            })
{
    mFilePath.entry.addMouseListener(this, true);
    mViewport.setViewedComponent(&mConcertinaTable, false);
    addAndMakeVisible(mViewport);
    setSize(300, 400);
    setWantsKeyboardFocus(false);
}

juce::File AudioFileInfoPanel::getFile() const
{
    return mFile;
}

void AudioFileInfoPanel::setAudioFormatReader(juce::File const& file, juce::AudioFormatReader const* reader)
{
    mFile = file;
    mConcertinaTable.setComponents({});
    mFilePath.entry.setEnabled(file != juce::File{});
    mFilePath.entry.setButtonText(file.getFileName());
    mFilePath.entry.setTooltip(file.getFullPathName());

    if(reader == nullptr)
    {
        resized();
        return;
    }
    mFileFormat.entry.setText(reader->getFormatName(), juce::NotificationType::dontSendNotification);
    mFileFormat.entry.setEditable(false);
    mSampleRate.entry.setText(juce::String(reader->sampleRate, 1) + "Hz", juce::NotificationType::dontSendNotification);
    mSampleRate.entry.setEditable(false);
    mBitPerSample.entry.setText(juce::String(reader->bitsPerSample) + " (" + (reader->usesFloatingPointData ? "float" : "int") + ")", juce::NotificationType::dontSendNotification);
    mBitPerSample.entry.setEditable(false);
    mLengthInSamples.entry.setText(juce::String(reader->lengthInSamples) + " samples", juce::NotificationType::dontSendNotification);
    mLengthInSamples.entry.setEditable(false);
    mDurationInSeconds.entry.setText(juce::String(static_cast<double>(reader->lengthInSamples) / reader->sampleRate, 3).trimCharactersAtEnd("0").trimCharactersAtEnd(".") + "s", juce::NotificationType::dontSendNotification);
    mDurationInSeconds.entry.setEditable(false);
    mNumChannels.entry.setText(juce::String(reader->numChannels), juce::NotificationType::dontSendNotification);
    mNumChannels.entry.setEditable(false);

    auto const& metadataValues = reader->metadataValues;
    mMetaDataPanels.clear();
    std::vector<ConcertinaTable::ComponentRef> panels{mFilePath, mFileFormat, mSampleRate, mBitPerSample, mLengthInSamples, mDurationInSeconds, mNumChannels};

    for(auto const& key : metadataValues.getAllKeys())
    {
        auto const& value = metadataValues[key];
        auto property = std::make_unique<PropertyLabel>(key, juce::translate("Metadata MDNM of the audio file").replace("MDNM", key));
        MiscWeakAssert(property != nullptr);
        if(property != nullptr)
        {
            property->entry.setText(value, juce::NotificationType::dontSendNotification);
            property->entry.setJustificationType(juce::Justification::right);
            property->entry.setEditable(false);
            panels.push_back(*property.get());
            mMetaDataPanels.push_back(std::move(property));
        }
    }
    mConcertinaTable.setComponents(panels);
    resized();
}

void AudioFileInfoPanel::resized()
{
    auto const scrollbarThickness = mViewport.getScrollBarThickness();
    mConcertinaTable.setBounds(getLocalBounds().withHeight(mConcertinaTable.getHeight()).withTrimmedRight(scrollbarThickness));
    mViewport.setBounds(getLocalBounds());
}

ANALYSE_FILE_END
