#include "AnlApplicationInterface.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Interface::Interface()
: mDocumentFileInfoPanel(Instance::get().getDocumentAccessor(), Instance::get().getDocumentFileBased(), Instance::get().getAudioFormatManager())
, mDocumentTransport(Instance::get().getDocumentAccessor())
, mDocumentSection(Instance::get().getDocumentAccessor())
{    
    addAndMakeVisible(mDocumentFileInfoPanel);
    addAndMakeVisible(mDocumentTransport);
    addAndMakeVisible(mNavigate);
    addAndMakeVisible(mInspect);
    addAndMakeVisible(mEdit);
    addAndMakeVisible(mDocumentSection);
    addAndMakeVisible(mToolTipDisplay);
    
    mNavigate.setTooltip(juce::translate("Navigate"));
    mInspect.setTooltip(juce::translate("Inspect"));
    mEdit.setTooltip(juce::translate("Edit"));
    
    auto setupImage = [](juce::ImageButton& button, juce::Image image)
    {
        button.setImages(true, true, true, image, 1.0f, juce::Colours::grey, image, 0.8f, juce::Colours::grey.brighter(), image, 0.8f, juce::Colours::grey.brighter());
    };
    setupImage(mNavigate, juce::ImageCache::getFromMemory(BinaryData::naviguer_png, BinaryData::naviguer_pngSize));
    setupImage(mInspect, juce::ImageCache::getFromMemory(BinaryData::chercher_png, BinaryData::chercher_pngSize));
    setupImage(mEdit, juce::ImageCache::getFromMemory(BinaryData::editer_png, BinaryData::editer_pngSize));
    
    mLoad.onClick = []()
    {
        using CommandIDs = CommandTarget::CommandIDs;
        Instance::get().getApplicationCommandManager().invokeDirectly(CommandIDs::DocumentOpen, true);
    };
    
    mDocumentListener.onChanged = [&](Document::Accessor const& acsr, Document::AttrType attribute)
    {
        switch(attribute)
        {
            case Document::AttrType::file:
            {
                auto const file = acsr.getAttr<Document::AttrType::file>();
                auto const& audioFormatManager = Instance::get().getAudioFormatManager();
                auto const isDocumentEnable = file.existsAsFile() && audioFormatManager.getWildcardForAllFormats().contains(file.getFileExtension());
                
                mDocumentTransport.setEnabled(isDocumentEnable);
                mDocumentFileInfoPanel.setEnabled(isDocumentEnable);
                mDocumentSection.setEnabled(isDocumentEnable);
                if(!isDocumentEnable)
                {
                    addAndMakeVisible(mLoad);
                }
                else
                {
                    removeChildComponent(&mLoad);
                }
            }
                break;
            case Document::isLooping:
            case Document::gain:
            case Document::isPlaybackStarted:
            case Document::playheadPosition:
            case Document::timeZoom:
            case Document::layout:
            case Document::layoutHorizontal:
            case Document::analyzers:
                break;
        }
    };
    
    auto& documentAccessor = Instance::get().getDocumentAccessor();
    documentAccessor.addListener(mDocumentListener, NotificationType::synchronous);
}

Application::Interface::~Interface()
{
    auto& documentAccessor = Instance::get().getDocumentAccessor();
    documentAccessor.removeListener(mDocumentListener);
}

void Application::Interface::resized()
{
    auto bounds = getLocalBounds();
    
    auto header = bounds.removeFromTop(82);
    mDocumentFileInfoPanel.setBounds(header.removeFromRight(320));
    mDocumentTransport.setBounds(header.removeFromLeft(240));
    auto buttons = header.withSizeKeepingCentre(120, 32);
    mNavigate.setBounds(buttons.removeFromLeft(32));
    buttons.removeFromLeft(12);
    mInspect.setBounds(buttons.removeFromLeft(32));
    buttons.removeFromLeft(12);
    mEdit.setBounds(buttons.removeFromLeft(32));
    
    mToolTipDisplay.setBounds(bounds.removeFromBottom(24));
    mDocumentSection.setBounds(bounds);
    
    mLoad.setBounds(bounds.withSizeKeepingCentre(200, 32));
}

ANALYSE_FILE_END
