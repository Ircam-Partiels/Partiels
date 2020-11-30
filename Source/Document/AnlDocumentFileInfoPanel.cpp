#include "AnlDocumentFileInfoPanel.h"

ANALYSE_FILE_BEGIN

Document::FileInfoPanel::FileInfoPanel(Accessor& accessor, juce::FileBasedDocument& fileBasedDocument, juce::AudioFormatManager& audioFormatManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
, mFileBasedDocument(fileBasedDocument)
{
    using Position = Layout::PropertyPanelBase::Positioning;
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch (attribute)
        {
            case AttrType::file:
            {
                mPropertySection3.setPanels({}, Position::left);
                auto const file = acsr.getAttr<AttrType::file>();
                mPanelFilePath.entry.setText(file.getFileName(), juce::NotificationType::dontSendNotification);
                mPanelFilePath.entry.setEditable(false);
                auto* audioFormat = mAudioFormatManager.findFormatForFileExtension(file.getFileExtension());
                if(audioFormat == nullptr)
                {
                    resized();
                    return;
                }
                mPanelFileFormat.entry.setText(audioFormat->getFormatName(), juce::NotificationType::dontSendNotification);
                mPanelFileFormat.entry.setEditable(false);
                auto audioFormatReader = std::unique_ptr<juce::AudioFormatReader>(audioFormat->createReaderFor(file.createInputStream().release(), true));
                if(audioFormatReader == nullptr)
                {
                    resized();
                    return;
                }
                mPanelSampleRate.entry.setText(juce::String(audioFormatReader->sampleRate, 1) + "Hz", juce::NotificationType::dontSendNotification);
                mPanelSampleRate.entry.setEditable(false);
                mPanelBitPerSample.entry.setText(juce::String(audioFormatReader->bitsPerSample) + " (" + (audioFormatReader->usesFloatingPointData ? "float" : "int") + ")", juce::NotificationType::dontSendNotification);
                mPanelBitPerSample.entry.setEditable(false);
                mPanelLengthInSamples.entry.setText(juce::String(audioFormatReader->lengthInSamples) + " samples", juce::NotificationType::dontSendNotification);
                mPanelLengthInSamples.entry.setEditable(false);
                mPanelDurationInSeconds.entry.setText(juce::String(static_cast<double>(audioFormatReader->lengthInSamples) / audioFormatReader->sampleRate, 3).trimCharactersAtEnd("0").trimCharactersAtEnd(".") + "s", juce::NotificationType::dontSendNotification);
                mPanelDurationInSeconds.entry.setEditable(false);
                mPanelNumChannels.entry.setText(juce::String(audioFormatReader->numChannels), juce::NotificationType::dontSendNotification);
                mPanelNumChannels.entry.setEditable(false);
                
                auto const& metadataValues = audioFormatReader->metadataValues;
                mMetaDataPanels.clear();
                std::vector<Layout::PropertySection::PanelRef> panels;
                for(auto const& key : metadataValues.getAllKeys())
                {
                    auto const& value = metadataValues[key];
                    auto property = std::make_unique<Layout::PropertyLabel>(key, juce::translate("Metadata MDNM of the audio file").replace("MDNM", key));
                    anlStrongAssert(property != nullptr);
                    if(property != nullptr)
                    {
                        property->entry.setText(value, juce::NotificationType::dontSendNotification);
                        property->entry.setJustificationType(juce::Justification::right);
                        property->entry.setEditable(false);
                        panels.push_back(*property.get());
                        mMetaDataPanels.push_back(std::move(property));
                    }
                }
                mPropertySection3.setPanels(panels, Position::left);
                resized();
            }
                break;
            case AttrType::isLooping:
            case AttrType::gain:
            case AttrType::isPlaybackStarted:
            case AttrType::playheadPosition:
            case AttrType::timeZoom:
            case AttrType::layout:
            case AttrType::layoutHorizontal:
            case AttrType::analyzers:
                break;
}
        changeListenerCallback(&mFileBasedDocument);
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mPropertySection1.setPanels({mPanelProjectName, mPanelFilePath, mPanelFileFormat, mPanelSampleRate}, Position::left);
    mPropertySection2.setPanels({mPanelBitPerSample, mPanelLengthInSamples, mPanelDurationInSeconds, mPanelNumChannels}, Position::left);
    
    addAndMakeVisible(mPropertySection1);
    addAndMakeVisible(mSeparator1);
    addAndMakeVisible(mPropertySection2);
    addAndMakeVisible(mSeparator2);
    addAndMakeVisible(mPropertySection3);
    mFileBasedDocument.addChangeListener(this);
    changeListenerCallback(&mFileBasedDocument);
}

Document::FileInfoPanel::~FileInfoPanel()
{
    mFileBasedDocument.removeChangeListener(this);
    mAccessor.removeListener(mListener);
}

void Document::FileInfoPanel::resized()
{
    auto constexpr minimumWidth = 138;
    auto constexpr separatorWidth = 2;
    
    auto bounds = getLocalBounds();
    auto const width = bounds.getWidth();
    auto const numVisibleLayout = std::min(width / (minimumWidth + separatorWidth), mMetaDataPanels.empty() ? 2 : 3);
    auto const layoutWidth = numVisibleLayout > 0 ? width / numVisibleLayout : 0;
    
    mPropertySection3.setVisible(numVisibleLayout == 3);
    mSeparator2.setVisible(mPropertySection3.isVisible());
    mPropertySection2.setVisible(numVisibleLayout >= 2);
    mSeparator1.setVisible(mPropertySection2.isVisible());
    mPropertySection1.setVisible(numVisibleLayout >= 1);
    if(numVisibleLayout == 3)
    {
        mPropertySection3.setBounds(bounds.removeFromRight(layoutWidth - separatorWidth));
        mSeparator2.setBounds(bounds.removeFromRight(separatorWidth));
    }
    if(numVisibleLayout >= 2)
    {
        mPropertySection2.setBounds(bounds.removeFromRight(layoutWidth));
        mSeparator1.setBounds(bounds.removeFromRight(separatorWidth));
    }
    if(numVisibleLayout >= 1)
    {
        mPropertySection1.setBounds(bounds);
    }
}

void Document::FileInfoPanel::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    anlStrongAssert(source == &mFileBasedDocument);
    auto const file = mFileBasedDocument.getFile();
    auto const name = file.existsAsFile() ? file.getFileNameWithoutExtension() : "Unsaved";
    auto const extension = file.existsAsFile() && mFileBasedDocument.hasChangedSinceSaved() ? "*" : "";
    mPanelProjectName.entry.setText(name + extension, juce::NotificationType::dontSendNotification);
    mPanelProjectName.entry.setTooltip(file.existsAsFile() ? file.getFullPathName() : "");
}

ANALYSE_FILE_END
