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
    
    mLoad.onClick = []()
    {
        using CommandIDs = CommandTarget::CommandIDs;
        Instance::get().getApplicationCommandManager().invokeDirectly(CommandIDs::DocumentOpen, true);
    };
    
    mDocumentSection.onRemoveAnalyzer = [](juce::String const& identifier)
    {
        auto& documentDir = Instance::get().getDocumentDirector();
        documentDir.removeAnalysis(identifier, NotificationType::synchronous);
    };
    
    mDocumentListener.onAttrChanged = [&](Document::Accessor const& acsr, Document::AttrType attribute)
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
            case Document::AttrType::isLooping:
            case Document::AttrType::gain:
            case Document::AttrType::isPlaybackStarted:
            case Document::AttrType::playheadPosition:
            case Document::AttrType::layoutHorizontal:
            case Document::AttrType::layoutVertical:
            case Document::AttrType::layout:
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
    
    auto header = bounds.removeFromTop(40);
    mDocumentFileInfoPanel.setBounds(header.removeFromRight(320));
    mDocumentTransport.setBounds(header.removeFromLeft(240));
    auto buttons = header.withSizeKeepingCentre(120, 32);
    mNavigate.setBounds(buttons.removeFromLeft(32).reduced(4));
    buttons.removeFromLeft(12);
    mInspect.setBounds(buttons.removeFromLeft(32).reduced(4));
    buttons.removeFromLeft(12);
    mEdit.setBounds(buttons.removeFromLeft(32).reduced(4));
    
    mToolTipDisplay.setBounds(bounds.removeFromBottom(24));
    mDocumentSection.setBounds(bounds);
    
    mLoad.setBounds(bounds.withSizeKeepingCentre(200, 32));
}

void Application::Interface::lookAndFeelChanged()
{
    auto* laf = dynamic_cast<IconManager::LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(laf != nullptr);
    if(laf != nullptr)
    {
        laf->setButtonIcon(mNavigate, IconManager::IconType::navigate);
        laf->setButtonIcon(mInspect, IconManager::IconType::search);
        laf->setButtonIcon(mEdit, IconManager::IconType::edit);
    }
}

void Application::Interface::parentHierarchyChanged()
{
    lookAndFeelChanged();
}

bool Application::Interface::isInterestedInFileDrag(juce::StringArray const& files)
{
    auto const audioFormatWildcard = Instance::get().getAudioFormatManager().getWildcardForAllFormats() + ";" + Instance::getFileWildCard();
    for(auto const& fileName : files)
    {
        if(audioFormatWildcard.contains(juce::File(fileName).getFileExtension()))
        {
            return true;
        }
    }
    return false;
}

void Application::Interface::fileDragEnter(juce::StringArray const& files, int x, int y)
{
    juce::ignoreUnused(files, x, y);
    mLoad.setState(juce::Button::ButtonState::buttonOver);
}

void Application::Interface::fileDragExit(juce::StringArray const& files)
{
    juce::ignoreUnused(files);
    mLoad.setState(juce::Button::ButtonState::buttonNormal);
}

void Application::Interface::filesDropped(juce::StringArray const& files, int x, int y)
{
    juce::ignoreUnused(x, y);
    mLoad.setState(juce::Button::ButtonState::buttonNormal);
    auto const audioFormatWildcard = Instance::get().getAudioFormatManager().getWildcardForAllFormats() + ";" + Instance::getFileWildCard();
    auto getFile = [&]()
    {
        for(auto const& fileName : files)
        {
            if(audioFormatWildcard.contains(juce::File(fileName).getFileExtension()))
            {
                return juce::File(fileName);
            }
        }
        return juce::File();
    };
    auto const file = getFile();
    if(file == juce::File())
    {
        return;
    }
    Instance::get().openFile(file);
}

ANALYSE_FILE_END
