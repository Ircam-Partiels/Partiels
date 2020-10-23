#include "AnlDocumentFileInfoPanel.h"

ANALYSE_FILE_BEGIN

Document::FileInfoPanel::Property::Property(juce::String const& text, juce::String const& tooltip)
: Tools::PropertyPanel<juce::Label>(text, tooltip)
{
    entry.setJustificationType(juce::Justification::right);
    entry.setMinimumHorizontalScale(1.0f);
}

Document::FileInfoPanel::FileInfoPanel(juce::AudioFormatManager& audioFormatManager, Accessor& accessor)
: mAudioFormatManager(audioFormatManager)
, mAccessor(accessor)
{
    using Attribute = Model::Attribute;
    using Position = Tools::PropertyPanelBase::Positioning;
    mListener.onChanged = [&](Accessor& acsr, Attribute attribute)
    {
        switch (attribute)
        {
            case Attribute::file:
            {
                mPropertyLayout3.setPanels({}, Position::left);
                auto const file = acsr.getModel().file;
                mPanelFileName.entry.setText(file.getFileName(), juce::NotificationType::dontSendNotification);
                mPanelFilePath.entry.setText(file.getParentDirectory().getFullPathName(), juce::NotificationType::dontSendNotification);
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
                    auto property = std::make_unique<Property>(key, juce::translate("Metadata MDNM of the audio file").replace("MDNM", key));
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
            case Attribute::analyzers:
                break;
            case Attribute::loop:
                break;
        }
    };
    mAccessor.addListener(mListener, juce::NotificationType::sendNotificationSync);
    mPropertyLayout1.setPanels({mPanelFileName, mPanelFilePath, mPanelFileFormat, mPanelSampleRate}, Position::left);
    mPropertyLayout2.setPanels({mPanelBitPerSample, mPanelLengthInSamples, mPanelDurationInSeconds, mPanelNumChannels}, Position::left);
    
    addAndMakeVisible(mPropertyLayout1);
    addAndMakeVisible(mPropertyLayout2);
    addAndMakeVisible(mPropertyLayout3);
}

Document::FileInfoPanel::~FileInfoPanel()
{
    mAccessor.removeListener(mListener);
}

void Document::FileInfoPanel::resized()
{
    auto constexpr minimumWidth = 140;
    
    auto bounds = getLocalBounds();
    auto const width = bounds.getWidth();
    auto const numVisibleLayout = std::min(width / minimumWidth, mMetaDataPanels.empty() ? 2 : 3);
    auto const layoutWidth = numVisibleLayout > 0 ? width / numVisibleLayout : 0;
    
    mPropertyLayout3.setVisible(numVisibleLayout == 3);
    mPropertyLayout2.setVisible(numVisibleLayout >= 2);
    mPropertyLayout1.setVisible(numVisibleLayout >= 1);
    if(numVisibleLayout == 3)
    {
        mPropertyLayout3.setBounds(bounds.removeFromRight(layoutWidth));
    }
    if(numVisibleLayout >= 2)
    {
        mPropertyLayout2.setBounds(bounds.removeFromRight(layoutWidth));
    }
    if(numVisibleLayout >= 1)
    {
        mPropertyLayout1.setBounds(bounds);
    }
}

ANALYSE_FILE_END
