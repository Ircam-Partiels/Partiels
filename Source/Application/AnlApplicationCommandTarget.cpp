#include "AnlApplicationCommandTarget.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::CommandTarget::CommandTarget()
{
}

juce::ApplicationCommandTarget* Application::CommandTarget::getNextCommandTarget()
{
    return nullptr;
}

void Application::CommandTarget::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    commands.addArray({
        CommandIDs::Open,
        CommandIDs::New,
        CommandIDs::Save,
        CommandIDs::Duplicate,
        CommandIDs::Consolidate,
        CommandIDs::Close,
        CommandIDs::OpenRecent
    });
}

void Application::CommandTarget::getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result)
{
    JUCE_COMPILER_WARNING("todo - active states");
    switch (commandID)
    {
        case CommandIDs::Open:
        {
            result.setInfo(juce::translate("Open..."), juce::translate("Open a document or an audio file"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('o', juce::ModifierKeys::commandModifier, 0));
            result.setActive(true);
        }
            break;
        case CommandIDs::OpenRecent:
        {
            result.setInfo(juce::translate("Open Recent"), juce::translate("Open a recent document"), "Application", 0);
            result.setActive(!Instance::get().getAccessor().getModel().recentlyOpenedFilesList.empty());
        }
            break;
        case CommandIDs::New:
        {
            result.setInfo(juce::translate("New"), juce::translate("Create a new document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('n', juce::ModifierKeys::commandModifier, 0));
            result.setActive(true);
        }
            break;
        case CommandIDs::Save:
        {
            result.setInfo(juce::translate("Save"), juce::translate("Save the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('s', juce::ModifierKeys::commandModifier, 0));
            result.setActive(true);
        }
            break;
        case CommandIDs::Duplicate:
        {
            result.setInfo(juce::translate("Duplicate..."), juce::translate("Save the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('s', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(true);
        }
            break;
        case CommandIDs::Consolidate:
        {
            result.setInfo(juce::translate("Consolidate..."), juce::translate("Consolidate the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('c', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(true);
        }
            break;
        case CommandIDs::Close:
        {
            result.setInfo(juce::translate("Close"), juce::translate("Close the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('w', juce::ModifierKeys::commandModifier, 0));
            result.setActive(true);
        }
            break;
    }
}

bool Application::CommandTarget::perform(juce::ApplicationCommandTarget::InvocationInfo const& info)
{
    auto getCommandDescription = [&]()
    {
        juce::ApplicationCommandInfo result(info.commandID);
        getCommandInfo(info.commandID, result);
        return result.description;
    };
    
    switch (info.commandID)
    {
        case CommandIDs::Open:
        {
            auto const audioFormatWildcard = Instance::get().getAudioFormatManager().getWildcardForAllFormats();
            juce::FileChooser fc(getCommandDescription(), {}, Instance::getFileWildCard() + ";" + audioFormatWildcard);
            if(!fc.browseForFileToOpen())
            {
                return true;
            }
            
            auto const file = fc.getResult();
            if(file.getFileExtension() == Instance::getFileWildCard())
            {
                
            }
            else if(audioFormatWildcard.contains(file.getFileExtension()))
            {
                auto& acsr = Instance::get().getDocumentAccessor();
                auto copy = acsr.getModel();
                copy.file = file;
                acsr.fromModel(copy, juce::NotificationType::sendNotificationSync);
            }
            else
            {
                
            }
            return true;
        }
        case CommandIDs::OpenRecent:
        {
            return true;
        }
        case CommandIDs::New:
        {
            return true;
        }
        case CommandIDs::Save:
        {
            return true;
        }
        case CommandIDs::Duplicate:
        {
            return true;
        }
        case CommandIDs::Consolidate:
        {
            return true;
        }
        case CommandIDs::Close:
        {
            return true;
        }
    }
    return false;
}

ANALYSE_FILE_END
