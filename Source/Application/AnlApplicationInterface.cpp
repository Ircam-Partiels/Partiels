
#include "AnlApplicationInterface.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationCommandIDs.h"

ANALYSE_FILE_BEGIN

Application::Interface::Interface()
: mDocumentFileInfoPanel(Instance::get().getAudioFormatManager(), mDocumentAccessor)
, mDocumentReader(Instance::get().getAudioFormatManager(), mDocumentAccessor)
, mPluginListTable(Instance::get().getPluginListAccessor())
{
    addAndMakeVisible(mHeader);
    addAndMakeVisible(mDocumentFileInfoPanel);
    Instance::get().getApplicationCommandManager().registerAllCommandsForTarget(this);
    
    mPluginListTable.onPluginSelected = [&](juce::String key)
    {
        auto copy = mAnalyzerAccessor.getModel();
        copy.key = key;
        mAnalyzerAccessor.fromModel(copy, juce::NotificationType::sendNotificationSync);
        if(mAudioFormatReader != nullptr)
        {
            mAnalyzerProcessor.perform(*(mAudioFormatReader.get()));
        }
    };
}

void Application::Interface::resized()
{
    mHeader.setBounds(getLocalBounds().removeFromTop(60));
    mDocumentFileInfoPanel.setBounds(getLocalBounds().removeFromTop(102));
}

juce::ApplicationCommandTarget* Application::Interface::getNextCommandTarget()
{
    return nullptr;
}

void Application::Interface::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    commands.add(CommandIDs::Open);
    commands.add(CommandIDs::New);
}

void Application::Interface::getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result)
{
    switch (commandID)
    {
        case CommandIDs::Open:
        {
            result.setInfo(juce::translate("Open"), juce::translate("Open an audio file"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('o', juce::ModifierKeys::commandModifier, 0));
        }
            break;
        case CommandIDs::New:
        {
            result.setInfo(juce::translate("New"), juce::translate("New analyis file"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('n', juce::ModifierKeys::commandModifier, 0));
        }
            break;
            
        default:
            break;
    }
}

bool Application::Interface::perform(juce::ApplicationCommandTarget::InvocationInfo const& info)
{
    switch (info.commandID)
    {
        case CommandIDs::Open:
        {
            using AlertIconType = juce::AlertWindow::AlertIconType;
            
            auto& audioFormatManager = Instance::get().getAudioFormatManager();
            JUCE_COMPILER_WARNING("Translation required for : Open Document");
            juce::FileChooser fc(juce::translate("Open Document"), {}, audioFormatManager.getWildcardForAllFormats());
            if(!fc.browseForFileToOpen())
            {
                auto copy = mDocumentAccessor.getModel();
                copy.file = juce::File{};
                mDocumentAccessor.fromModel(copy, juce::NotificationType::sendNotificationSync);
                return true;
            }
                
            auto const file = fc.getResult();
            
            auto copy = mDocumentAccessor.getModel();
            copy.file = file;
            mDocumentAccessor.fromModel(copy, juce::NotificationType::sendNotificationSync);
            auto* audioFormat = audioFormatManager.findFormatForFileExtension(file.getFileExtension());
            if(audioFormat == nullptr)
            {
                juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, juce::translate("Can't load the audio!"), juce::translate("the file extension format ADFMMT is not supported.").replace("ADFMMT", file.getFileExtension()));
                return true;
            }
            
            mAudioFormatReader = std::unique_ptr<juce::AudioFormatReader>(audioFormat->createReaderFor(file.createInputStream().release(), true));
            if(mAudioFormatReader == nullptr)
            {
                juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, juce::translate("Can't load the audio!"), juce::translate("the file audio forrmat reader cannot be allocated.").replace("ADFMMT", file.getFileExtension()));
                return true;
            }
            return true;
        }
        case CommandIDs::New:
        {
            juce::DialogWindow::showModalDialog("Plugin List", &mPluginListTable, this, findColour(juce::ResizableWindow::backgroundColourId, true), true);
            return true;
        }
        default:
            break;
    }
    return false;
}

ANALYSE_FILE_END
