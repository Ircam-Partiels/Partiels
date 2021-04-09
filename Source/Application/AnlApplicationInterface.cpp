#include "AnlApplicationInterface.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Interface::Interface()
: mDocumentFileInfoPanel(Instance::get().getDocumentAccessor(), Instance::get().getAudioFormatManager())
, mTransportDisplay(Instance::get().getDocumentAccessor().getAcsr<Document::AcsrType::transport>())
, mDocumentSection(Instance::get().getDocumentAccessor())
{    
    addAndMakeVisible(mDocumentFileInfoPanel);
    addAndMakeVisible(mFileInfoResizer);
    addAndMakeVisible(mTransportDisplay);
//    addAndMakeVisible(mNavigate);
//    addAndMakeVisible(mInspect);
//    addAndMakeVisible(mEdit);
    addAndMakeVisible(mDocumentSection);
    addAndMakeVisible(mToolTipDisplay);

//    mNavigate.setTooltip(juce::translate("Navigate"));
//    mInspect.setTooltip(juce::translate("Inspect"));
//    mEdit.setTooltip(juce::translate("Edit"));
    
    mLoad.onClick = []()
    {
        using CommandIDs = CommandTarget::CommandIDs;
        Instance::get().getApplicationCommandManager().invokeDirectly(CommandIDs::DocumentOpen, true);
    };
    
    mDocumentSection.onRemoveTrack = [](juce::String const& identifier)
    {
        auto& documentDir = Instance::get().getDocumentDirector();
        documentDir.removeTrack(identifier, NotificationType::synchronous);
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
                
                mTransportDisplay.setEnabled(isDocumentEnable);
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
            case Document::AttrType::layoutVertical:
            case Document::AttrType::layout:
            case Document::AttrType::expanded:
                break;
        }
    };
    
    mFileInfoResizer.onMoved = [&](int size)
    {
        mDocumentFileInfoPanel.setSize(size, mDocumentFileInfoPanel.getHeight());
        resized();
    };
    
    mDocumentFileInfoPanel.setSize(240, 100);
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
    
    mTransportDisplay.setBounds(bounds.removeFromTop(40).withSizeKeepingCentre(280, 40));
//    auto buttons = header.withSizeKeepingCentre(120, 32);
//    mNavigate.setBounds(buttons.removeFromLeft(32).reduced(4));
//    buttons.removeFromLeft(12);
//    mInspect.setBounds(buttons.removeFromLeft(32).reduced(4));
//    buttons.removeFromLeft(12);
//    mEdit.setBounds(buttons.removeFromLeft(32).reduced(4));
    
    mToolTipDisplay.setBounds(bounds.removeFromBottom(24));
    mLoad.setBounds(bounds.withSizeKeepingCentre(200, 32));
    mDocumentFileInfoPanel.setBounds(bounds.removeFromRight(mDocumentFileInfoPanel.getWidth()));
    mFileInfoResizer.setBounds(bounds.removeFromRight(2));
    mDocumentSection.setBounds(bounds);
}

void Application::Interface::lookAndFeelChanged()
{
    auto* laf = dynamic_cast<IconManager::LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(laf != nullptr);
    if(laf != nullptr)
    {
//        laf->setButtonIcon(mNavigate, IconManager::IconType::navigate);
//        laf->setButtonIcon(mInspect, IconManager::IconType::search);
//        laf->setButtonIcon(mEdit, IconManager::IconType::edit);
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
