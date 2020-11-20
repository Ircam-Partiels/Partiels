#include "AnlDocumentFileInfoPanel.h"

ANALYSE_FILE_BEGIN

Document::FileInfoPanel::FileInfoPanel(Accessor& accessor, juce::FileBasedDocument& fileBasedDocument, juce::AudioFormatManager& audioFormatManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
, mFileBasedDocument(fileBasedDocument)
{
    using Position = Tools::PropertyPanelBase::Positioning;
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch (attribute)
        {
            case AttrType::file:
            {
                mPropertyLayout3.setPanels({}, Position::left);
                auto const file = acsr.getValue<AttrType::file>();
                mPanelFilePath.entry.setText(file.getFileName(), juce::NotificationType::dontSendNotification);
                auto* audioFormat = mAudioFormatManager.findFormatForFileExtension(file.getFileExtension());
                if(audioFormat == nullptr)
                {
                    resized();
                    return;
                }
                mPanelFileFormat.entry.setText(audioFormat->getFormatName(), juce::NotificationType::dontSendNotification);
                auto audioFormatReader = std::unique_ptr<juce::AudioFormatReader>(audioFormat->createReaderFor(file.createInputStream().release(), true));
                if(audioFormatReader == nullptr)
                {
                    resized();
                    return;
                }
                mPanelSampleRate.entry.setText(juce::String(audioFormatReader->sampleRate, 1) + "Hz", juce::NotificationType::dontSendNotification);
                mPanelBitPerSample.entry.setText(juce::String(audioFormatReader->bitsPerSample) + " (" + (audioFormatReader->usesFloatingPointData ? "float" : "int") + ")", juce::NotificationType::dontSendNotification);
                mPanelLengthInSamples.entry.setText(juce::String(audioFormatReader->lengthInSamples) + " samples", juce::NotificationType::dontSendNotification);
                mPanelDurationInSeconds.entry.setText(juce::String(static_cast<double>(audioFormatReader->lengthInSamples) / audioFormatReader->sampleRate, 3).trimCharactersAtEnd("0").trimCharactersAtEnd(".") + "s", juce::NotificationType::dontSendNotification);
                mPanelNumChannels.entry.setText(juce::String(audioFormatReader->numChannels), juce::NotificationType::dontSendNotification);
                
                auto const& metadataValues = audioFormatReader->metadataValues;
                mMetaDataPanels.clear();
                std::vector<Tools::PropertyLayout::PanelRef> panels;
                for(auto const& key : metadataValues.getAllKeys())
                {
                    auto const& value = metadataValues[key];
                    auto property = std::make_unique<Tools::PropertyLabel>(key, juce::translate("Metadata MDNM of the audio file").replace("MDNM", key));
                    anlStrongAssert(property != nullptr);
                    if(property != nullptr)
                    {
                        property->entry.setText(value, juce::NotificationType::dontSendNotification);
                        property->entry.setJustificationType(juce::Justification::right);
                        panels.push_back(*property.get());
                        mMetaDataPanels.push_back(std::move(property));
                    }
                }
                mPropertyLayout3.setPanels(panels, Position::left);
                resized();
            }
                break;
//            case AttrType::analyzers:
//                break;
            case AttrType::isLooping:
                break;
        }
        changeListenerCallback(&mFileBasedDocument);
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mPropertyLayout1.setPanels({mPanelProjectName, mPanelFilePath, mPanelFileFormat, mPanelSampleRate}, Position::left);
    mPropertyLayout2.setPanels({mPanelBitPerSample, mPanelLengthInSamples, mPanelDurationInSeconds, mPanelNumChannels}, Position::left);
    
    addAndMakeVisible(mPropertyLayout1);
    addAndMakeVisible(mSeparator1);
    addAndMakeVisible(mPropertyLayout2);
    addAndMakeVisible(mSeparator2);
    addAndMakeVisible(mPropertyLayout3);
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
    
    mPropertyLayout3.setVisible(numVisibleLayout == 3);
    mSeparator2.setVisible(mPropertyLayout3.isVisible());
    mPropertyLayout2.setVisible(numVisibleLayout >= 2);
    mSeparator1.setVisible(mPropertyLayout2.isVisible());
    mPropertyLayout1.setVisible(numVisibleLayout >= 1);
    if(numVisibleLayout == 3)
    {
        mPropertyLayout3.setBounds(bounds.removeFromRight(layoutWidth - separatorWidth));
        mSeparator2.setBounds(bounds.removeFromRight(separatorWidth));
    }
    if(numVisibleLayout >= 2)
    {
        mPropertyLayout2.setBounds(bounds.removeFromRight(layoutWidth));
        mSeparator1.setBounds(bounds.removeFromRight(separatorWidth));
    }
    if(numVisibleLayout >= 1)
    {
        mPropertyLayout1.setBounds(bounds);
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
