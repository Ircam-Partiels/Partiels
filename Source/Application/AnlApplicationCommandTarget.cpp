#include "AnlApplicationCommandTarget.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

static void showUnsupportedAction()
{
//    juce::ImageComponent:: getFromMemory::get
//    juce::LocalisedStrings::setCurrentMappings(new juce::LocalisedStrings(juce::String::createStringFromData(BinaryData::Fr_txt, BinaryData::Fr_txtSize), false));
}

Application::CommandTarget::CommandTarget()
{
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::recentlyOpenedFilesList:
                Instance::get().getApplicationCommandManager().commandStatusChanged();
                break;
            case AttrType::currentDocumentFile:
                break;
            case AttrType::windowState:
                break;
        }
    };
    
    Instance::get().getDocumentFileBased().addChangeListener(this);
    Instance::get().getApplicationAccessor().addListener(mListener, NotificationType::synchronous);
    Instance::get().getApplicationCommandManager().registerAllCommandsForTarget(this);
}

Application::CommandTarget::~CommandTarget()
{
    Instance::get().getApplicationAccessor().removeListener(mListener);
    Instance::get().getDocumentFileBased().removeChangeListener(this);
}

juce::ApplicationCommandTarget* Application::CommandTarget::getNextCommandTarget()
{
    return nullptr;
}

void Application::CommandTarget::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    commands.addArray(
    {
        CommandIDs::Open
        , CommandIDs::New
        , CommandIDs::Save
        , CommandIDs::Duplicate
        , CommandIDs::Consolidate
        
        , CommandIDs::TogglePlayback
        , CommandIDs::ToggleLooping
        , CommandIDs::MovePlayHeadToBeginning
        , CommandIDs::MovePlayHeadToEnd

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
            result.setActive(!Instance::get().getApplicationAccessor().getAttr<AttrType::recentlyOpenedFilesList>().empty());
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
            result.setActive(false);
        }
            break;
            
        case CommandIDs::TogglePlayback:
        {
            result.setInfo(juce::translate("Toggle Playback"), TRANS("Start or stop the audio playback"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::spaceKey, juce::ModifierKeys::noModifiers, 0));
            result.setActive(Instance::get().getDocumentAccessor().getAttr<Document::AttrType::file>() != juce::File());
            result.setTicked(Instance::get().getDocumentAccessor().getAttr<Document::AttrType::isPlaybackStarted>());
        }
            break;
        case CommandIDs::ToggleLooping:
        {
            result.setInfo(juce::translate("Toggle Loop"), TRANS("Enable or disable the loop audio playback"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress('l', juce::ModifierKeys::commandModifier, 0));
            result.setActive(Instance::get().getDocumentAccessor().getAttr<Document::AttrType::file>() != juce::File());
            result.setTicked(Instance::get().getDocumentAccessor().getAttr<Document::AttrType::isLooping>());
        }
            break;
        case CommandIDs::MovePlayHeadToBeginning:
        {
            result.setInfo(juce::translate("Rewind Playhead"), TRANS("Move the playhead to the start of the document"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress('w', juce::ModifierKeys::commandModifier, 0));
            result.setActive(Instance::get().getDocumentAccessor().getAttr<Document::AttrType::file>() != juce::File() && Instance::get().getDocumentAccessor().getAttr<Document::AttrType::playheadPosition>() > 0.0);
        }
            break;
        case CommandIDs::MovePlayHeadToEnd:
        {
            result.setInfo(juce::translate("Unspool Playhead"), TRANS("Move the playhead to the end of the document"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress('q', juce::ModifierKeys::commandModifier, 0));
            JUCE_COMPILER_WARNING("fix that")
            result.setActive(Instance::get().getDocumentAccessor().getAttr<Document::AttrType::file>() != juce::File() && Instance::get().getDocumentAccessor().getAttr<Document::AttrType::playheadPosition>() < 10000.0);
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
            Instance::get().getDocumentAccessor().fromContainer({}, NotificationType::synchronous);
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
            auto constexpr attr = Document::AttrType::isPlaybackStarted;
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            documentAcsr.setAttr<attr>(!documentAcsr.getAttr<attr>(), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::ToggleLooping:
        {
            auto constexpr attr = Document::AttrType::isLooping;
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            documentAcsr.setAttr<attr>(!documentAcsr.getAttr<attr>(), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::MovePlayHeadToBeginning:
        {
            auto constexpr attr = Document::AttrType::playheadPosition;
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            documentAcsr.setAttr<attr>(0.0, NotificationType::synchronous);
            return true;
        }
        case CommandIDs::MovePlayHeadToEnd:
        {
            auto constexpr attr = Document::AttrType::playheadPosition;
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            JUCE_COMPILER_WARNING("fix this")
            documentAcsr.setAttr<attr>(10000.0, NotificationType::synchronous);
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
    
    auto const file = fileBased.getFile();
    Instance::get().getApplicationAccessor().setAttr<AttrType::currentDocumentFile>(file, NotificationType::synchronous);
    if(file.existsAsFile())
    {
        auto list = Instance::get().getApplicationAccessor().getAttr<AttrType::recentlyOpenedFilesList>();
        list.insert(list.begin(), file);
        Instance::get().getApplicationAccessor().setAttr<AttrType::recentlyOpenedFilesList>(list, NotificationType::synchronous);
    }
}

ANALYSE_FILE_END
