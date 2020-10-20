#include "AnlDocumentFileInfoPanel.h"
#include "../Application/AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Document::FileInfoPanel::Property::Property(juce::String const& text, juce::String const& tooltip)
: Tools::PropertyPanel<juce::Label>(text, tooltip)
{
    entry.setJustificationType(juce::Justification::right);
    entry.setMinimumHorizontalScale(1.0f);
}

Document::FileInfoPanel::FileInfoPanel(Accessor& accessor)
: mAccessor(accessor)
{
    using Attribute = Model::Attribute;
    mListener.onChanged = [&](Accessor& acsr, Attribute attribute)
    {
        juce::ignoreUnused(acsr);
        switch (attribute)
        {
            case Attribute::file:
            {
                mPropertyLayout3.setPanels({});
                auto const file = mAccessor.getModel().file;
                mPanelFileName.entry.setText(file.getFileName(), juce::NotificationType::dontSendNotification);
                mPanelFilePath.entry.setText(file.getParentDirectory().getFullPathName(), juce::NotificationType::dontSendNotification);
                auto& audioFormatManager = Application::Instance::get().getAudioFormatManager();
                auto* audioFormat = audioFormatManager.findFormatForFileExtension(file.getFileExtension());
                if(audioFormat == nullptr)
                {
                    return;
                }
                mPanelFileFormat.entry.setText(audioFormat->getFormatName(), juce::NotificationType::dontSendNotification);
                auto audioFormatReader = std::unique_ptr<juce::AudioFormatReader>(audioFormat->createReaderFor(file.createInputStream().release(), true));
                if(audioFormatReader == nullptr)
                {
                    return;
                }
                mPanelSampleRate.entry.setText(juce::String(audioFormatReader->sampleRate, 1) + "Hz", juce::NotificationType::dontSendNotification);
                mPanelBitPerSample.entry.setText(juce::String(audioFormatReader->bitsPerSample), juce::NotificationType::dontSendNotification);
                mPanelLengthInSamples.entry.setText(juce::String(audioFormatReader->lengthInSamples) + " samples", juce::NotificationType::dontSendNotification);
                mPanelDurationInSeconds.entry.setText(juce::String(static_cast<double>(audioFormatReader->lengthInSamples) / audioFormatReader->sampleRate, 3).trimCharactersAtEnd("0").trimCharactersAtEnd(".") + "s", juce::NotificationType::dontSendNotification);
                mPanelNumChannels.entry.setText(juce::String(audioFormatReader->numChannels), juce::NotificationType::dontSendNotification);
                
                using PanelInfo = Tools::PropertyLayout::PanelInfo;
                auto const& metadataValues = audioFormatReader->metadataValues;
                mMetaDataPanels.clear();
                std::vector<PanelInfo> panels;
                auto constexpr titleWidth = 200;
                for(auto const& key : metadataValues.getAllKeys())
                {
                    auto const& value = metadataValues[key];
                    auto property = std::make_unique<Property>(key, juce::translate("Metadata MDNM of the audio file").replace("MDNM", key));
                    anlStrongAssert(property != nullptr);
                    if(property != nullptr)
                    {
                        property->entry.setText(value, juce::NotificationType::dontSendNotification);
                        property->entry.setJustificationType(juce::Justification::right);
                        panels.push_back({*property.get(), titleWidth});
                        mMetaDataPanels.push_back(std::move(property));
                    }
                }
                mPropertyLayout3.setPanels(panels);
                resized();
            }
                break;
            case Attribute::analyzers:
                break;
        }
    };
    mAccessor.addListener(mListener, juce::NotificationType::sendNotificationSync);
    auto constexpr titleWidth = 92;
    
    mPropertyLayout1.setPanels({{mPanelFileName, titleWidth}, {mPanelFilePath, titleWidth}, {mPanelFileFormat, titleWidth}, {mPanelSampleRate, titleWidth}});
    mPropertyLayout2.setPanels({ {mPanelBitPerSample, titleWidth}, {mPanelLengthInSamples, titleWidth}, {mPanelDurationInSeconds, titleWidth}, {mPanelNumChannels, titleWidth}});
    
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
    
    auto bounds = getBounds();
    auto const width = bounds.getWidth();
    auto const numVisibleLayout = std::min(width / minimumWidth, mMetaDataPanels.empty() ? 2 : 3);
    auto const layoutWidth = width / numVisibleLayout;
    
    mPropertyLayout3.setVisible(numVisibleLayout == 3);
    mPropertyLayout2.setVisible(numVisibleLayout >= 2);
    mPropertyLayout1.setVisible(numVisibleLayout >= 1);
    if(numVisibleLayout == 3)
    {
        mPropertyLayout3.organizePanels(Tools::PropertyLayout::Orientation::vertical, layoutWidth);
        mPropertyLayout3.setBounds(bounds.removeFromRight(layoutWidth));
    }
    if(numVisibleLayout >= 2)
    {
        mPropertyLayout2.organizePanels(Tools::PropertyLayout::Orientation::vertical, layoutWidth);
        mPropertyLayout2.setBounds(bounds.removeFromRight(layoutWidth));
    }
    if(numVisibleLayout >= 1)
    {
        mPropertyLayout1.organizePanels(Tools::PropertyLayout::Orientation::vertical, bounds.getWidth());
        mPropertyLayout1.setBounds(bounds);
    }
}

ANALYSE_FILE_END
