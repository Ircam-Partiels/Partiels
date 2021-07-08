#include "AnlApplicationCommandTarget.h"
#include "../Document/AnlDocumentTools.h"
#include "../Track/AnlTrackExporter.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::CommandTarget::CommandTarget()
: mPluginListTable(Instance::get().getPluginListAccessor(), Instance::get().getPluginListScanner())
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::recentlyOpenedFilesList:
            {
                Instance::get().getApplicationCommandManager().commandStatusChanged();
            }
            break;
            case AttrType::currentDocumentFile:
            case AttrType::windowState:
            case AttrType::colourMode:
            case AttrType::showInfoBubble:
            case AttrType::exportOptions:
                break;
        }
    };

    Instance::get().getDocumentFileBased().addChangeListener(this);
    Instance::get().getUndoManager().addChangeListener(this);
    Instance::get().getApplicationAccessor().addListener(mListener, NotificationType::synchronous);
    Instance::get().getApplicationCommandManager().registerAllCommandsForTarget(this);
    Instance::get().getApplicationCommandManager().setFirstCommandTarget(this);
}

Application::CommandTarget::~CommandTarget()
{
    Instance::get().getApplicationCommandManager().setFirstCommandTarget(nullptr);
    Instance::get().getApplicationAccessor().removeListener(mListener);
    Instance::get().getUndoManager().removeChangeListener(this);
    Instance::get().getDocumentFileBased().removeChangeListener(this);
}

juce::ApplicationCommandTarget* Application::CommandTarget::getNextCommandTarget()
{
    return nullptr;
}

void Application::CommandTarget::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    // clang-format off
    commands.addArray(
    {
          CommandIDs::DocumentNew
        , CommandIDs::DocumentOpen
        , CommandIDs::DocumentSave
        , CommandIDs::DocumentDuplicate
        , CommandIDs::DocumentConsolidate
        , CommandIDs::DocumentExport
        , CommandIDs::DocumentImport
        , CommandIDs::DocumentBatch
        
        , CommandIDs::EditUndo
        , CommandIDs::EditRedo
        , CommandIDs::EditNewGroup
        , CommandIDs::EditNewTrack
        , CommandIDs::EditRemoveItem
        , CommandIDs::EditLoadTemplate
        
        , CommandIDs::TransportTogglePlayback
        , CommandIDs::TransportToggleLooping
        , CommandIDs::TransportRewindPlayHead
        
        , CommandIDs::ViewZoomIn
        , CommandIDs::ViewZoomOut
        , CommandIDs::ViewInfoBubble
        
        , CommandIDs::HelpOpenAudioSettings
        , CommandIDs::HelpOpenAbout
        , CommandIDs::HelpOpenManual
        , CommandIDs::HelpOpenForum
    });
    // clang-format on
}

void Application::CommandTarget::getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result)
{
    auto const& documentAcsr = Instance::get().getDocumentAccessor();
    auto const& transportAcsr = documentAcsr.getAcsr<Document::AcsrType::transport>();
    switch(commandID)
    {
        case CommandIDs::DocumentNew:
        {
            result.setInfo(juce::translate("New..."), juce::translate("Create a new document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('n', juce::ModifierKeys::commandModifier, 0));
            result.setActive(!documentAcsr.isEquivalentTo(Instance::get().getDocumentFileBased().getDefaultAccessor()));
        }
        break;
        case CommandIDs::DocumentOpen:
        {
            result.setInfo(juce::translate("Open..."), juce::translate("Open a document or audio files"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('o', juce::ModifierKeys::commandModifier, 0));
            result.setActive(true);
        }
        break;
        case CommandIDs::DocumentSave:
        {
            result.setInfo(juce::translate("Save"), juce::translate("Save the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('s', juce::ModifierKeys::commandModifier, 0));
            result.setActive(true);
        }
        break;
        case CommandIDs::DocumentDuplicate:
        {
            result.setInfo(juce::translate("Duplicate..."), juce::translate("Save the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('s', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(true);
        }
        break;
        case CommandIDs::DocumentConsolidate:
        {
            result.setInfo(juce::translate("Consolidate..."), juce::translate("Consolidate the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('c', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
        }
        break;
        case CommandIDs::DocumentExport:
        {
            result.setInfo(juce::translate("Export..."), juce::translate("Export the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('e', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(documentAcsr.getNumAcsrs<Document::AcsrType::tracks>() > 0_z);
        }
        break;
        case CommandIDs::DocumentImport:
        {
            result.setInfo(juce::translate("Import..."), juce::translate("Import to the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('i', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
        }
        break;
        case CommandIDs::DocumentBatch:
        {
            result.setInfo(juce::translate("Batch..."), juce::translate("Batch processed a set of audio files "), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('b', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(documentAcsr.getNumAcsrs<Document::AcsrType::tracks>() > 0_z);
        }
        break;

        case CommandIDs::EditUndo:
        {
            auto& undoManager = Instance::get().getUndoManager();
            result.setInfo(juce::translate("Undo ") + undoManager.getUndoDescription(), juce::translate("Undo last action"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0));
            result.setActive(undoManager.canUndo());
        }
        break;
        case CommandIDs::EditRedo:
        {
            auto& undoManager = Instance::get().getUndoManager();
            result.setInfo(juce::translate("Redo ") + undoManager.getRedoDescription(), juce::translate("Redo last action"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('z', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(undoManager.canRedo());
        }
        break;

        case CommandIDs::EditNewGroup:
        {
            result.setInfo(juce::translate("Add New Group"), juce::translate("Adds a new group"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('g', juce::ModifierKeys::commandModifier, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
        }
        break;
        case CommandIDs::EditNewTrack:
        {
            result.setInfo(juce::translate("Add New Track"), juce::translate("Adds a new track"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('t', juce::ModifierKeys::commandModifier, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
        }
        break;
        case CommandIDs::EditRemoveItem:
        {
            auto focusedTrack = Document::Tools::getFocusedTrack(documentAcsr);
            auto focusedGroup = Document::Tools::getFocusedGroup(documentAcsr);
            if(focusedTrack.has_value())
            {
                result.setInfo(juce::translate("Remove Track"), juce::translate("Adds the selected track"), "Edit", 0);
            }
            else if(focusedGroup.has_value())
            {
                result.setInfo(juce::translate("Remove Group"), juce::translate("Removes the selected group"), "Edit", 0);
            }
            else
            {
                result.setInfo(juce::translate("Remove Track or Group"), juce::translate("Removes the selected track or group"), "Edit", 0);
            }
            result.setActive(focusedTrack.has_value() || focusedGroup.has_value());
            result.defaultKeypresses.add(juce::KeyPress(0x08, juce::ModifierKeys::noModifiers, 0));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::backspaceKey, juce::ModifierKeys::noModifiers, 0));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::deleteKey, juce::ModifierKeys::noModifiers, 0));
        }
        break;
        case CommandIDs::EditLoadTemplate:
        {
            result.setInfo(juce::translate("Load Template..."), juce::translate("Load a template"), "Edit", 0);
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
        }
        break;

        case CommandIDs::TransportTogglePlayback:
        {
            result.setInfo(juce::translate("Toggle Playback"), juce::translate("Start or stop the audio playback"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::spaceKey, juce::ModifierKeys::noModifiers, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
            result.setTicked(transportAcsr.getAttr<Transport::AttrType::playback>());
        }
        break;
        case CommandIDs::TransportToggleLooping:
        {
            result.setInfo(juce::translate("Toggle Loop"), juce::translate("Enable or disable the loop audio playback"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress('l', juce::ModifierKeys::commandModifier, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
            result.setTicked(transportAcsr.getAttr<Transport::AttrType::looping>());
        }
        break;
        case CommandIDs::TransportRewindPlayHead:
        {
            result.setInfo(juce::translate("Rewind Playhead"), juce::translate("Move the playhead to the start of the document"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress('w', juce::ModifierKeys::commandModifier, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty() && transportAcsr.getAttr<Transport::AttrType::runningPlayhead>() > 0.0);
        }
        break;

        case CommandIDs::ViewZoomIn:
        {
            auto const& zoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
            result.setInfo(juce::translate("Zoom In"), juce::translate("Opens the manual in a web browser"), "View", 0);
            result.defaultKeypresses.add(juce::KeyPress('+', juce::ModifierKeys::commandModifier, 0));
            result.setActive(zoomAcsr.getAttr<Zoom::AttrType::visibleRange>().getLength() > zoomAcsr.getAttr<Zoom::AttrType::minimumLength>());
        }
        break;
        case CommandIDs::ViewZoomOut:
        {
            auto const& zoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
            result.setInfo(juce::translate("Zoom Out"), juce::translate("Opens the manual in a web browser"), "View", 0);
            result.defaultKeypresses.add(juce::KeyPress('-', juce::ModifierKeys::commandModifier, 0));
            result.setActive(zoomAcsr.getAttr<Zoom::AttrType::visibleRange>().getLength() < zoomAcsr.getAttr<Zoom::AttrType::globalRange>().getLength());
        }
        break;
        case CommandIDs::ViewInfoBubble:
        {
            result.setInfo(juce::translate("Info Bubble"), juce::translate("Toggle the info bubble info"), "View", 0);
            result.defaultKeypresses.add(juce::KeyPress('i', juce::ModifierKeys::commandModifier, 0));
            result.setActive(true);
            result.setTicked(Instance::get().getApplicationAccessor().getAttr<AttrType::showInfoBubble>());
        }
        break;

        case CommandIDs::HelpOpenAudioSettings:
        {
#ifdef JUCE_MAC
            result.setInfo(juce::translate("Audio Settings..."), juce::translate("Show the audio settings panel"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress(',', juce::ModifierKeys::commandModifier, 0));
#else
            result.setInfo(juce::translate("Audio Settings..."), juce::translate("Show the audio settings panel"), "Help", 0);
            result.defaultKeypresses.add(juce::KeyPress('p', juce::ModifierKeys::commandModifier, 0));
#endif
        }
        break;
        case CommandIDs::HelpOpenAbout:
        {
#ifdef JUCE_MAC
            result.setInfo(juce::translate("About Partiels"), juce::translate("Show the information about the application"), "Application", 0);
#else
            result.setInfo(juce::translate("About Partiels"), juce::translate("Show the information about the application"), "help", 0);
#endif
        }
        break;
        case CommandIDs::HelpOpenManual:
        {
            result.setInfo(juce::translate("Open Manual"), juce::translate("Opens the manual in a web browser"), "Help", 0);
            result.setActive(true);
        }
        break;
        case CommandIDs::HelpOpenForum:
        {
            result.setInfo(juce::translate("Proceed to Forum"), juce::translate("Opens the forum page in a web browser"), "Help", 0);
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

    auto& fileBased = Instance::get().getDocumentFileBased();
    auto& documentAcsr = Instance::get().getDocumentAccessor();
    auto& transportAcsr = documentAcsr.getAcsr<Document::AcsrType::transport>();
    switch(info.commandID)
    {
        case CommandIDs::DocumentNew:
        {
            if(fileBased.saveIfNeededAndUserAgrees() != juce::FileBasedDocument::SaveResult::savedOk)
            {
                return true;
            }
            Instance::get().openFiles({});
            return true;
        }
        case CommandIDs::DocumentOpen:
        {
            if(fileBased.saveIfNeededAndUserAgrees() != juce::FileBasedDocument::SaveResult::savedOk)
            {
                return true;
            }
            auto const audioFormatWildcard = Instance::get().getAudioFormatManager().getWildcardForAllFormats() + ";" + Instance::getFileWildCard();
            juce::FileChooser fc(getCommandDescription(), fileBased.getFile(), audioFormatWildcard);
            if(!fc.browseForMultipleFilesToOpen())
            {
                return true;
            }
            std::vector<juce::File> files;
            for(auto const& result : fc.getResults())
            {
                files.push_back(result);
            }
            Instance::get().openFiles(files);
            return true;
        }
        case CommandIDs::DocumentSave:
        {
            fileBased.save(true, true);
            return true;
        }
        case CommandIDs::DocumentDuplicate:
        {
            fileBased.saveAsInteractive(true);
            return true;
        }
        case CommandIDs::DocumentConsolidate:
        {
            if(fileBased.save(true, true) == juce::FileBasedDocument::SaveResult::savedOk)
            {
                auto const result = fileBased.consolidate();
                if(result.wasOk())
                {
                    AlertWindow::showMessage(AlertWindow::MessageType::info, "Document consolidated!", "The document have been consolidated with the audio files and the analyses to FLNAME.", {{"FLNAME", fileBased.getFile().getFullPathName()}});
                }
                else
                {
                    AlertWindow::showMessage(AlertWindow::MessageType::warning, "Document consolidation failed!", "The document cannot be consolidated with the audio files and the analyses to FLNAME. ERROR", {{"FLNAME", fileBased.getFile().getFullPathName()}, {"ERROR", result.getErrorMessage()}});
                }
            }
            return true;
        }

        case CommandIDs::DocumentExport:
        {
            if(auto* exporter = Instance::get().getExporter())
            {
                exporter->show();
            }
            return true;
        }
        case CommandIDs::DocumentImport:
        {
            auto const position = getNewTrackPosition();
            juce::FileChooser fc(juce::translate("Load file"), {}, "*.json");
            if(fc.browseForFileToOpen())
            {
                addFileTrack(fc.getResult(), std::get<0>(position), std::get<1>(position));
            }
            return true;
        }
        case CommandIDs::DocumentBatch:
        {
            if(auto* batcher = Instance::get().getBatcher())
            {
                batcher->show();
            }
            return true;
        }

        case CommandIDs::EditUndo:
        {
            auto& undoManager = Instance::get().getUndoManager();
            undoManager.undo();
            return true;
        }
        case CommandIDs::EditRedo:
        {
            auto& undoManager = Instance::get().getUndoManager();
            undoManager.redo();
            return true;
        }
        case CommandIDs::EditNewGroup:
        {
            auto& documentDir = Instance::get().getDocumentDirector();
            documentDir.startAction();

            auto const index = documentAcsr.getNumAcsrs<Document::AcsrType::groups>();
            auto const focusedId = Document::Tools::getFocusedGroup(documentAcsr);
            auto const position = focusedId.has_value() ? Document::Tools::getGroupPosition(documentAcsr, *focusedId) + 1_z : index;
            auto const identifier = documentDir.addGroup(position, NotificationType::synchronous);
            if(identifier.has_value())
            {
                documentDir.endAction(ActionState::newTransaction, juce::translate("New Group"));
                if(auto* window = Instance::get().getWindow())
                {
                    window->moveKeyboardFocusTo(*identifier);
                }
            }
            else
            {
                documentDir.endAction(ActionState::abort);
                AlertWindow::showMessage(AlertWindow::MessageType::warning, "Group cannot be created!", "The group cannot be inserted into the document.");
            }
            return true;
        }
        case CommandIDs::EditNewTrack:
        {
            mPluginListTable.onPluginSelected = [this, position = getNewTrackPosition()](Plugin::Key const& key, Plugin::Description const& description)
            {
                mPluginListTable.hide();
                addPluginTrack(key, description, std::get<0>(position), std::get<1>(position));
            };
            mPluginListTable.show();
            return true;
        }
        case CommandIDs::EditRemoveItem:
        {
            auto& documentDir = Instance::get().getDocumentDirector();

            auto focusedTrack = Document::Tools::getFocusedTrack(documentAcsr);
            auto focusedGroup = Document::Tools::getFocusedGroup(documentAcsr);
            if(focusedTrack.has_value())
            {
                auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, *focusedTrack);
                auto const trackName = trackAcsr.getAttr<Track::AttrType::name>();
                if(!AlertWindow::showOkCancel(AlertWindow::MessageType::question, "Remove Track", "Are you sure you want to remove the \"TRACKNAME\" track from the project?", {{"TRACKNAME", trackName}}))
                {
                    return true;
                }

                documentDir.startAction();
                if(documentDir.removeTrack(*focusedTrack, NotificationType::synchronous))
                {
                    documentDir.endAction(ActionState::newTransaction, juce::translate("Remove Track"));
                }
                else
                {
                    documentDir.endAction(ActionState::abort);
                }
            }
            else if(focusedGroup.has_value())
            {
                auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, *focusedGroup);
                auto const groupName = groupAcsr.getAttr<Group::AttrType::name>();
                if(!AlertWindow::showOkCancel(AlertWindow::MessageType::question, "Remove Group", "Are you sure you want to remove the \"GROUPNAME\" group from the project? This will also remove all the contained analyses!", {{"GROUPNAME", groupName}}))
                {
                    return true;
                }

                documentDir.startAction();
                if(documentDir.removeGroup(*focusedGroup, NotificationType::synchronous))
                {
                    documentDir.endAction(ActionState::newTransaction, juce::translate("Remove Group").replace("GROUPNAME", groupName));
                }
                else
                {
                    documentDir.endAction(ActionState::abort);
                }
            }
            return true;
        }
        case CommandIDs::EditLoadTemplate:
        {
            Document::Accessor copyAcsr;
            juce::UndoManager copyUndoManager;
            Document::Director copyDirector{copyAcsr, Instance::get().getAudioFormatManager(), copyUndoManager};
            Document::FileBased copyFileBased{copyDirector, Instance::getFileExtension(), Instance::getFileWildCard(), "Open a document", "Save the document"};
            if(copyFileBased.loadFromUserSpecifiedFile(true).failed())
            {
                return true;
            }

            copyAcsr.setAttr<Document::AttrType::reader>(documentAcsr.getAttr<Document::AttrType::reader>(), NotificationType::synchronous);

            auto& documentDir = Instance::get().getDocumentDirector();
            documentDir.startAction();
            documentAcsr.copyFrom(copyAcsr, NotificationType::synchronous);
            documentDir.endAction(ActionState::newTransaction, juce::translate("Load template"));
            return true;
        }

        case CommandIDs::TransportTogglePlayback:
        {
            auto constexpr attr = Transport::AttrType::playback;
            transportAcsr.setAttr<attr>(!transportAcsr.getAttr<attr>(), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::TransportToggleLooping:
        {
            auto constexpr attr = Transport::AttrType::looping;
            transportAcsr.setAttr<attr>(!transportAcsr.getAttr<attr>(), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::TransportRewindPlayHead:
        {
            auto constexpr attr = Transport::AttrType::startPlayhead;
            auto const isPlaying = transportAcsr.getAttr<Transport::AttrType::playback>();
            if(isPlaying)
            {
                transportAcsr.setAttr<Transport::AttrType::playback>(false, NotificationType::synchronous);
            }
            transportAcsr.setAttr<attr>(0.0, NotificationType::synchronous);
            if(isPlaying)
            {
                transportAcsr.setAttr<Transport::AttrType::playback>(true, NotificationType::synchronous);
            }
            return true;
        }

        case CommandIDs::ViewZoomIn:
        {
            auto& zoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
            auto const range = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            auto const grange = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range.expanded(grange.getLength() / -100.0), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::ViewZoomOut:
        {
            auto& zoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
            auto const range = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            auto const grange = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range.expanded(grange.getLength() / 100.0), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::ViewInfoBubble:
        {
            auto& accessor = Instance::get().getApplicationAccessor();
            accessor.setAttr<AttrType::showInfoBubble>(!accessor.getAttr<AttrType::showInfoBubble>(), NotificationType::synchronous);
            return true;
        }

        case CommandIDs::HelpOpenAudioSettings:
        {
            if(auto* audioSettings = Instance::get().getAudioSettings())
            {
                audioSettings->show();
            }
            return true;
        }
        case CommandIDs::HelpOpenAbout:
        {
            if(auto* about = Instance::get().getAbout())
            {
                about->show();
            }
            return true;
        }
        case CommandIDs::HelpOpenManual:
        {
            juce::URL const url("https://forum.ircam.fr/contact/documentations-logiciels/");
            if(url.isWellFormed())
            {
                url.launchInDefaultBrowser();
            }
            return true;
        }
        case CommandIDs::HelpOpenForum:
        {
            juce::URL const url("https://forum.ircam.fr/projects/detail/partiels/");
            if(url.isWellFormed())
            {
                url.launchInDefaultBrowser();
            }
            return true;
        }
    }
    return false;
}

void Application::CommandTarget::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if(source == &Instance::get().getUndoManager())
    {
        Instance::get().getApplicationCommandManager().commandStatusChanged();
        return;
    }
    auto const& fileBased = Instance::get().getDocumentFileBased();
    anlStrongAssert(source == &fileBased);
    if(source != &fileBased)
    {
        return;
    }

    auto& acsr = Instance::get().getApplicationAccessor();
    auto const file = fileBased.getFile();
    acsr.setAttr<AttrType::currentDocumentFile>(file, NotificationType::synchronous);
    if(file.existsAsFile())
    {
        auto list = acsr.getAttr<AttrType::recentlyOpenedFilesList>();
        list.insert(list.begin(), file);
        acsr.setAttr<AttrType::recentlyOpenedFilesList>(list, NotificationType::synchronous);
    }
}

std::tuple<juce::String, size_t> Application::CommandTarget::getNewTrackPosition() const
{
    auto const& documentAcsr = Instance::get().getDocumentAccessor();
    auto const focusedTrack = Document::Tools::getFocusedTrack(documentAcsr);
    if(focusedTrack.has_value())
    {
        auto const& groupAcsr = Document::Tools::getGroupAcsrForTrack(documentAcsr, *focusedTrack);
        auto const groupIdentifier = groupAcsr.getAttr<Group::AttrType::identifier>();
        auto const position = Document::Tools::getTrackPosition(documentAcsr, *focusedTrack);
        return std::make_tuple(groupIdentifier, position + 1_z);
    }
    auto const focusedGroup = Document::Tools::getFocusedGroup(documentAcsr);
    if(focusedGroup.has_value())
    {
        return std::make_tuple(*focusedGroup, 0_z);
    }
    return std::make_tuple(juce::String(""), 0_z);
}

void Application::CommandTarget::addPluginTrack(Plugin::Key const& key, Plugin::Description const& description, juce::String groupIdentifier, size_t position)
{
    auto& documentAcsr = Instance::get().getDocumentAccessor();
    auto& documentDir = Instance::get().getDocumentDirector();
    documentDir.startAction();

    // Creates a group if there is none
    if(documentAcsr.getNumAcsrs<Document::AcsrType::groups>() == 0_z)
    {
        anlStrongAssert(groupIdentifier.isEmpty());

        auto const identifier = documentDir.addGroup(0, NotificationType::synchronous);
        anlStrongAssert(identifier.has_value());
        if(!identifier.has_value())
        {
            documentDir.endAction(ActionState::abort);
            AlertWindow::showMessage(AlertWindow::MessageType::warning, "Group cannot be created!", "The group necessary for the new track cannot be inserted into the document.");
            return;
        }
        groupIdentifier = *identifier;
        position = 0_z;
    }

    anlStrongAssert(documentAcsr.getNumAcsrs<Document::AcsrType::groups>() > 0_z);
    if(groupIdentifier.isEmpty())
    {
        auto const& layout = documentAcsr.getAttr<Document::AttrType::layout>();
        groupIdentifier = layout.front();
        position = 0_z;
    }

    auto const identifier = documentDir.addTrack(groupIdentifier, position, NotificationType::synchronous);
    if(identifier.has_value())
    {
        auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, *identifier);
        trackAcsr.setAttr<Track::AttrType::name>(description.name, NotificationType::synchronous);
        trackAcsr.setAttr<Track::AttrType::key>(key, NotificationType::synchronous);

        auto& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupIdentifier);
        groupAcsr.setAttr<Group::AttrType::expanded>(true, NotificationType::synchronous);

        documentDir.endAction(ActionState::newTransaction, juce::translate("New Track"));
        if(auto* window = Instance::get().getWindow())
        {
            window->moveKeyboardFocusTo(*identifier);
        }
    }
    else
    {
        documentDir.endAction(ActionState::abort);
    }
}

void Application::CommandTarget::addFileTrack(juce::File const& file, juce::String groupIdentifier, size_t position)
{
    if(!file.hasFileExtension("json"))
    {
        AlertWindow::showMessage(AlertWindow::MessageType::warning, "File forrmat not supported!", "The....");
        return;
    }

    auto& documentAcsr = Instance::get().getDocumentAccessor();
    auto& documentDir = Instance::get().getDocumentDirector();
    documentDir.startAction();

    // Creates a group if there is none
    if(documentAcsr.getNumAcsrs<Document::AcsrType::groups>() == 0_z)
    {
        anlStrongAssert(groupIdentifier.isEmpty());

        auto const identifier = documentDir.addGroup(0, NotificationType::synchronous);
        anlStrongAssert(identifier.has_value());
        if(!identifier.has_value())
        {
            documentDir.endAction(ActionState::abort);
            AlertWindow::showMessage(AlertWindow::MessageType::warning, "Group cannot be created!", "The group necessary for the new track cannot be inserted into the document.");
            return;
        }
        groupIdentifier = *identifier;
        position = 0_z;
    }

    anlStrongAssert(documentAcsr.getNumAcsrs<Document::AcsrType::groups>() > 0_z);
    if(groupIdentifier.isEmpty())
    {
        auto const& layout = documentAcsr.getAttr<Document::AttrType::layout>();
        groupIdentifier = layout.front();
        position = 0_z;
    }

    auto const identifier = documentDir.addTrack(groupIdentifier, position, NotificationType::synchronous);
    if(identifier.has_value())
    {
        auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, *identifier);
        trackAcsr.setAttr<Track::AttrType::name>(file.getFileNameWithoutExtension(), NotificationType::synchronous);
        trackAcsr.setAttr<Track::AttrType::file>(file, NotificationType::synchronous);

        auto& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupIdentifier);
        groupAcsr.setAttr<Group::AttrType::expanded>(true, NotificationType::synchronous);

        documentDir.endAction(ActionState::newTransaction, juce::translate("New Track"));
        if(auto* window = Instance::get().getWindow())
        {
            window->moveKeyboardFocusTo(*identifier);
        }
        AlertWindow::showMessage(AlertWindow::MessageType::info, "Track imported!", "The new track have been imported friom the file FLNAME into the document.", {{"FLNAME", file.getFullPathName()}});
    }
    else
    {
        documentDir.endAction(ActionState::abort);
        AlertWindow::showMessage(AlertWindow::MessageType::warning, "Track cannot be created!", "The new track cannot be created into the document.");
    }
}

ANALYSE_FILE_END
