#include "AnlApplicationInterface.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Interface::Interface()
: mTransportDisplay(Instance::get().getDocumentAccessor().getAcsr<Document::AcsrType::transport>())
, mDocumentSection(Instance::get().getDocumentAccessor(), Instance::get().getAudioFormatManager())
{
    addAndMakeVisible(mTransportDisplay);
    addAndMakeVisible(mDocumentSection);
    addAndMakeVisible(mToolTipDisplay);

    mLoad.onClick = []()
    {
        using CommandIDs = CommandTarget::CommandIDs;
        Instance::get().getApplicationCommandManager().invokeDirectly(CommandIDs::DocumentOpen, true);
    };

    mDocumentSection.onTrackInserted = [](juce::String const& groupIdentifier, juce::String const& trackIdentifier)
    {
        auto& documentDir = Instance::get().getDocumentDirector();
        documentDir.moveTrack(AlertType::window, groupIdentifier, trackIdentifier, NotificationType::synchronous);
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
    mTransportDisplay.setBounds(bounds.removeFromTop(40).withSizeKeepingCentre(284, 40));
    mToolTipDisplay.setBounds(bounds.removeFromBottom(24));
    mLoad.setBounds(bounds.withSizeKeepingCentre(200, 32));
    mDocumentSection.setBounds(bounds);
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
