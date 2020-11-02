#include "AnlApplicationCommandTarget.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::CommandTarget::CommandTarget()
{
    mListener.onChanged = [&](Accessor const& acsr, Attribute attribute)
    {
        juce::ignoreUnused(acsr);
        if(attribute == Attribute::recentlyOpenedFilesList)
        {
            Instance::get().getApplicationCommandManager().commandStatusChanged();
        }
    };
    
    Instance::get().getDocumentFileBased().addChangeListener(this);
    Instance::get().getAccessor().addListener(mListener, juce::NotificationType::dontSendNotification);
    Instance::get().getApplicationCommandManager().registerAllCommandsForTarget(this);
}

Application::CommandTarget::~CommandTarget()
{
    Instance::get().getAccessor().removeListener(mListener);
    Instance::get().getDocumentFileBased().removeChangeListener(this);
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
        
        CommandIDs::TogglePlayback,
        CommandIDs::ToggleLooping,
        CommandIDs::MovePlayHeadToBeginning,
        CommandIDs::MovePlayHeadToEnd,
        
        CommandIDs::OpenRecent
    });
}

void Application::CommandTarget::getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result)
{
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
            result.setActive(Instance::get().getDocumentFileBased().hasChangedSinceSaved());
        }
            break;
        case CommandIDs::Duplicate:
        {
            result.setInfo(juce::translate("Duplicate..."), juce::translate("Save the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('s', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(Instance::get().getDocumentAccessor().getModel().file != juce::File());
        }
            break;
        case CommandIDs::Consolidate:
        {
            result.setInfo(juce::translate("Consolidate..."), juce::translate("Consolidate the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('c', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(false);
        }
            break;
            
        case CommandIDs::TogglePlayback:
        {
            result.setInfo(juce::translate("Toggle Playback"), TRANS("Start or stop the audio playback"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::spaceKey, juce::ModifierKeys::noModifiers, 0));
            result.setActive(Instance::get().getDocumentAccessor().getModel().file != juce::File());
            result.setTicked(Instance::get().getDocumentAccessor().getModel().isPlaybackStarted);
        }
            break;
        case CommandIDs::ToggleLooping:
        {
            result.setInfo(juce::translate("Toggle Loop"), TRANS("Enable or disable the loop audio playback"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress('l', juce::ModifierKeys::commandModifier, 0));
            result.setActive(Instance::get().getDocumentAccessor().getModel().file != juce::File());
            result.setTicked(Instance::get().getDocumentAccessor().getModel().isLooping);
        }
            break;
        case CommandIDs::MovePlayHeadToBeginning:
        {
            result.setInfo(juce::translate("Rewind Playhead"), TRANS("Move the playhead to the start of the document"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress('w', juce::ModifierKeys::commandModifier, 0));
            result.setActive(Instance::get().getDocumentAccessor().getModel().file != juce::File() && Instance::get().getDocumentAccessor().getModel().playheadPosition > 0.0);
        }
            break;
        case CommandIDs::MovePlayHeadToEnd:
        {
            result.setInfo(juce::translate("Unspool Playhead"), TRANS("Move the playhead to the end of the document"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress('q', juce::ModifierKeys::commandModifier, 0));
            JUCE_COMPILER_WARNING("fix that")
            result.setActive(Instance::get().getDocumentAccessor().getModel().file != juce::File() && Instance::get().getDocumentAccessor().getModel().playheadPosition < 10000.0);
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
    
    auto& fileBased = Instance::get().getDocumentFileBased();
    switch (info.commandID)
    {
        case CommandIDs::Open:
        {
            if(fileBased.saveIfNeededAndUserAgrees() != juce::FileBasedDocument::SaveResult::savedOk)
            {
                return true;
            }
            auto const audioFormatWildcard = Instance::get().getAudioFormatManager().getWildcardForAllFormats();
            juce::FileChooser fc(getCommandDescription(), fileBased.getFile(), Instance::getFileWildCard() + ";" + audioFormatWildcard);
            if(!fc.browseForFileToOpen())
            {
                return true;
            }
            Instance::get().openFile(fc.getResult());
            return true;
        }
        case CommandIDs::OpenRecent:
        {
            // Managed 
            return true;
        }
        case CommandIDs::New:
        {
            if(fileBased.saveIfNeededAndUserAgrees() != juce::FileBasedDocument::SaveResult::savedOk)
            {
                return true;
            }
            fileBased.setFile({});
            Instance::get().getDocumentAccessor().fromModel({}, juce::NotificationType::sendNotificationSync);
            return true;
        }
        case CommandIDs::Save:
        {
            fileBased.save(true, true);
            return true;
        }
        case CommandIDs::Duplicate:
        {
            fileBased.saveAsInteractive(true);
            return true;
        }
        case CommandIDs::Consolidate:
        {
            JUCE_COMPILER_WARNING("todo");
            anlWeakAssert(false && "todo");
            return true;
        }
            
        case CommandIDs::TogglePlayback:
        {
            auto copy = Instance::get().getDocumentAccessor().getModel();
            copy.isPlaybackStarted = !copy.isPlaybackStarted;
            Instance::get().getDocumentAccessor().fromModel(copy, juce::NotificationType::sendNotificationSync);
            return true;
        }
        case CommandIDs::ToggleLooping:
        {
            auto copy = Instance::get().getDocumentAccessor().getModel();
            copy.isLooping = !copy.isLooping;
            Instance::get().getDocumentAccessor().fromModel(copy, juce::NotificationType::sendNotificationSync);
            return true;
        }
        case CommandIDs::MovePlayHeadToBeginning:
        {
            auto copy = Instance::get().getDocumentAccessor().getModel();
            copy.playheadPosition = 0.0;
            Instance::get().getDocumentAccessor().fromModel(copy, juce::NotificationType::sendNotificationSync);
            return true;
        }
        case CommandIDs::MovePlayHeadToEnd:
        {
            auto copy = Instance::get().getDocumentAccessor().getModel();
            copy.playheadPosition = 10000.0;
            Instance::get().getDocumentAccessor().fromModel(copy, juce::NotificationType::sendNotificationSync);
            return true;
        }
    }
    return false;
}

void Application::CommandTarget::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    auto const& fileBased = Instance::get().getDocumentFileBased();
    anlStrongAssert(source == &fileBased);
    Instance::get().getApplicationCommandManager().commandStatusChanged();
    
    auto copy = Instance::get().getAccessor().getModel();
    copy.currentDocumentFile = fileBased.getFile();
    if(fileBased.getFile().existsAsFile())
    {
        auto& list = copy.recentlyOpenedFilesList;
        list.erase(std::unique(list.begin(), list.end()), list.end());
        list.insert(list.begin(), fileBased.getFile());
    }
    Instance::get().getAccessor().fromModel(copy, juce::NotificationType::sendNotificationSync);
}

ANALYSE_FILE_END
