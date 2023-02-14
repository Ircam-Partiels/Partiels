#include "AnlApplicationCommandTarget.h"
#include "../Document/AnlDocumentCommandTarget.h"
#include "../Document/AnlDocumentSelection.h"
#include "../Document/AnlDocumentTools.h"
#include "../Track/AnlTrackExporter.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationTools.h"

ANALYSE_FILE_BEGIN

Application::CommandTarget::CommandTarget()
: mPluginListTable(Instance::get().getPluginListAccessor(), Instance::get().getPluginListScanner())
, mPluginListSearchPath(Instance::get().getPluginListAccessor())
, mSdifConverterWindow(juce::translate("SDIF Converter"), mSdifConverter)
, mAudioSettingsWindow(juce::translate("Audio Settings"), mAudioSettings)
{
    Instance::get().getDocumentDirector().setPluginTable(&mPluginTableContainer);
    Instance::get().getDocumentDirector().setLoaderSelector(Instance::get().getFileLoaderSelectorContainer());
    mListener.onAttrChanged = [](Accessor const& acsr, AttrType attribute)
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
            case AttrType::adaptationToSampleRate:
            case AttrType::autoLoadConvertedFile:
            case AttrType::desktopGlobalScaleFactor:
            case AttrType::routingMatrix:
            case AttrType::autoUpdate:
            case AttrType::lastVersion:
                break;
        }
    };

    mTransportListener.onAttrChanged = [](Transport::Accessor const& acsr, Transport::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Transport::AttrType::runningPlayhead:
                break;
            case Transport::AttrType::playback:
            case Transport::AttrType::startPlayhead:
            case Transport::AttrType::looping:
            case Transport::AttrType::loopRange:
            case Transport::AttrType::stopAtLoopEnd:
            case Transport::AttrType::gain:
            case Transport::AttrType::markers:
            case Transport::AttrType::magnetize:
            case Transport::AttrType::autoScroll:
            case Transport::AttrType::selection:
            {
                Instance::get().getApplicationCommandManager().commandStatusChanged();
            }
            break;
        }
    };

    Instance::get().getDocumentFileBased().addChangeListener(this);
    Instance::get().getUndoManager().addChangeListener(this);
    Instance::get().getApplicationAccessor().addListener(mListener, NotificationType::synchronous);
    Instance::get().getDocumentAccessor().getAcsr<Document::AcsrType::transport>().addListener(mTransportListener, NotificationType::synchronous);
    Instance::get().getApplicationCommandManager().registerAllCommandsForTarget(this);
}

Application::CommandTarget::~CommandTarget()
{
    Instance::get().getDocumentAccessor().getAcsr<Document::AcsrType::transport>().removeListener(mTransportListener);
    Instance::get().getApplicationAccessor().removeListener(mListener);
    Instance::get().getUndoManager().removeChangeListener(this);
    Instance::get().getDocumentFileBased().removeChangeListener(this);
    Instance::get().getDocumentDirector().setPluginTable(nullptr);
    Instance::get().getDocumentDirector().setLoaderSelector(nullptr);
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
          CommandIDs::documentNew
        , CommandIDs::documentOpen
        , CommandIDs::documentSave
        , CommandIDs::documentDuplicate
        , CommandIDs::documentConsolidate
        , CommandIDs::documentExport
        , CommandIDs::documentImport
        , CommandIDs::documentBatch
        
        , CommandIDs::editUndo
        , CommandIDs::editRedo
        , CommandIDs::editSelectNextItem
        , CommandIDs::editNewGroup
        , CommandIDs::editNewTrack
        , CommandIDs::editRemoveItem
        , CommandIDs::editLoadTemplate
        
        , CommandIDs::transportTogglePlayback
        , CommandIDs::transportToggleLooping
        , CommandIDs::transportToggleStopAtLoopEnd
        , CommandIDs::transportToggleMagnetism
        , CommandIDs::transportRewindPlayHead
        , CommandIDs::transportMovePlayHeadBackward
        , CommandIDs::transportMovePlayHeadForward
        
        , CommandIDs::viewTimeZoomIn
        , CommandIDs::viewTimeZoomOut
        , CommandIDs::viewVerticalZoomIn
        , CommandIDs::viewVerticalZoomOut
        , CommandIDs::viewInfoBubble
        
        , CommandIDs::helpOpenAudioSettings
        , CommandIDs::helpOpenPluginSettings
        , CommandIDs::helpOpenAbout
        , CommandIDs::helpOpenProjectPage
        , CommandIDs::helpAuthorize
        , CommandIDs::helpSdifConverter
        , CommandIDs::helpAutoUpdate
        , CommandIDs::helpCheckForUpdate
    });
    // clang-format on
    Instance::get().getAllCommands(commands);
}

void Application::CommandTarget::getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result)
{
    auto const& documentAcsr = Instance::get().getDocumentAccessor();
    auto const& transportAcsr = documentAcsr.getAcsr<Document::AcsrType::transport>();
    auto const isItemMode = documentAcsr.getAttr<Document::AttrType::editMode>() == Document::EditMode::items;
    switch(commandID)
    {
        case CommandIDs::documentNew:
        {
            result.setInfo(juce::translate("New..."), juce::translate("Creates a new document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('n', juce::ModifierKeys::commandModifier, 0));
            result.setActive(!documentAcsr.isEquivalentTo(Instance::get().getDocumentFileBased().getDefaultAccessor()));
        }
        break;
        case CommandIDs::documentOpen:
        {
            result.setInfo(juce::translate("Open..."), juce::translate("Opens a document or audio files"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('o', juce::ModifierKeys::commandModifier, 0));
            result.setActive(true);
        }
        break;
        case CommandIDs::documentSave:
        {
            result.setInfo(juce::translate("Save"), juce::translate("Saves the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('s', juce::ModifierKeys::commandModifier, 0));
            result.setActive(true);
        }
        break;
        case CommandIDs::documentDuplicate:
        {
            result.setInfo(juce::translate("Duplicate..."), juce::translate("Saves the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('s', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(true);
        }
        break;
        case CommandIDs::documentConsolidate:
        {
            result.setInfo(juce::translate("Consolidate..."), juce::translate("Consolidates the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('c', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
        }
        break;
        case CommandIDs::documentExport:
        {
            result.setInfo(juce::translate("Export..."), juce::translate("Exports the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('e', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(documentAcsr.getNumAcsrs<Document::AcsrType::tracks>() > 0_z);
        }
        break;
        case CommandIDs::documentImport:
        {
            result.setInfo(juce::translate("Import..."), juce::translate("Imports to the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('i', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
        }
        break;
        case CommandIDs::documentBatch:
        {
            result.setInfo(juce::translate("Batch..."), juce::translate("Batch processes a set of audio files"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('b', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(documentAcsr.getNumAcsrs<Document::AcsrType::tracks>() > 0_z);
        }
        break;

        case CommandIDs::editUndo:
        {
            auto& undoManager = Instance::get().getUndoManager();
            result.setInfo(juce::translate("Undo ") + undoManager.getUndoDescription(), juce::translate("Undoes the last action"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0));
            result.setActive(undoManager.canUndo());
        }
        break;
        case CommandIDs::editRedo:
        {
            auto& undoManager = Instance::get().getUndoManager();
            result.setInfo(juce::translate("Redo ") + undoManager.getRedoDescription(), juce::translate("Redoes the last action"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('z', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(undoManager.canRedo());
        }
        break;

        case CommandIDs::editSelectNextItem:
        {
            result.setInfo(juce::translate("Select Next Item"), juce::translate("Select Next Item"), "Select", 0);
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::tabKey, juce::ModifierKeys::noModifiers, 0));
            result.setActive(true);
        }
        break;
        case CommandIDs::editNewGroup:
        {
            result.setInfo(juce::translate("Add New Group"), juce::translate("Adds a new group"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('g', juce::ModifierKeys::commandModifier, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
        }
        break;
        case CommandIDs::editNewTrack:
        {
            result.setInfo(juce::translate("Add New Track"), juce::translate("Adds a new track"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('t', juce::ModifierKeys::commandModifier, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
        }
        break;
        case CommandIDs::editRemoveItem:
        {
            auto selectedItems = Document::Selection::getItems(documentAcsr);
            if(!std::get<0_z>(selectedItems).empty() && std::get<1_z>(selectedItems).empty())
            {
                result.setInfo(juce::translate("Remove Group(s)"), juce::translate("Removes the selected group(s)"), "Edit", 0);
            }
            else if(!std::get<0_z>(selectedItems).empty() && std::get<1_z>(selectedItems).empty())
            {
                result.setInfo(juce::translate("Remove Track(s)"), juce::translate("Removes the selected track(s)"), "Edit", 0);
            }
            else
            {
                result.setInfo(juce::translate("Remove Group(s) and Track(s)"), juce::translate("Removes the selected group(s) and track(s)"), "Edit", 0);
            }
            result.setActive(isItemMode && (!std::get<0_z>(selectedItems).empty() || !std::get<1_z>(selectedItems).empty()));
            result.defaultKeypresses.add(juce::KeyPress(0x08, juce::ModifierKeys::noModifiers, 0));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::backspaceKey, juce::ModifierKeys::noModifiers, 0));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::deleteKey, juce::ModifierKeys::noModifiers, 0));
        }
        break;
        case CommandIDs::editLoadTemplate:
        {
            result.setInfo(juce::translate("Load Template..."), juce::translate("Loads a template"), "Edit", 0);
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
        }
        break;

        case CommandIDs::transportTogglePlayback:
        {
            result.setInfo(juce::translate("Toggle Playback"), juce::translate("Starts or stops the audio playback"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::spaceKey, juce::ModifierKeys::noModifiers, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
            result.setTicked(transportAcsr.getAttr<Transport::AttrType::playback>());
        }
        break;
        case CommandIDs::transportToggleLooping:
        {
            result.setInfo(juce::translate("Toggle Loop"), juce::translate("Enables or disables the loop audio playback"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress('l', juce::ModifierKeys::commandModifier, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
            result.setTicked(transportAcsr.getAttr<Transport::AttrType::looping>());
        }
        break;
        case CommandIDs::transportToggleStopAtLoopEnd:
        {
            result.setInfo(juce::translate("Toggle Stop Playback at Loop End"), juce::translate("Enables or disables the stop playback at the end of loop if repeat disabled"), "Transport", 0);
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
            result.setTicked(transportAcsr.getAttr<Transport::AttrType::stopAtLoopEnd>());
        }
        break;
        case CommandIDs::transportToggleMagnetism:
        {
            result.setInfo(juce::translate("Toggle Magnetize"), juce::translate("Enables or disables the magnetism mechanism with markers"), "Transport", 0);
            result.setActive(!transportAcsr.getAttr<Transport::AttrType::markers>().empty());
            result.setTicked(transportAcsr.getAttr<Transport::AttrType::magnetize>());
        }
        break;
        case CommandIDs::transportRewindPlayHead:
        {
            result.setInfo(juce::translate("Rewind Playhead"), juce::translate("Moves the playhead to the start of the document"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress('w', juce::ModifierKeys::commandModifier, 0));
            auto const hasReader = !documentAcsr.getAttr<Document::AttrType::reader>().empty();
            result.setActive(hasReader && Transport::Tools::canRewindPlayhead(transportAcsr));
        }
        break;
        case CommandIDs::transportMovePlayHeadBackward:
        {
            result.setInfo(juce::translate("Move the Playhead Backward"), juce::translate("Moves the playhead to the previous marker"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::leftKey, juce::ModifierKeys::commandModifier, 0));
            auto const hasReader = !documentAcsr.getAttr<Document::AttrType::reader>().empty();
            auto const& timeZoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
            result.setActive(hasReader && Transport::Tools::canMovePlayheadBackward(transportAcsr, timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>()));
        }
        break;
        case CommandIDs::transportMovePlayHeadForward:
        {
            result.setInfo(juce::translate("Move the Playhead Forward"), juce::translate("Moves the playhead to the previous marker"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::rightKey, juce::ModifierKeys::commandModifier, 0));
            auto const hasReader = !documentAcsr.getAttr<Document::AttrType::reader>().empty();
            auto const& timeZoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
            result.setActive(hasReader && Transport::Tools::canMovePlayheadForward(transportAcsr, timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>()));
        }
        break;

        case CommandIDs::viewTimeZoomIn:
        {
            auto const& zoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
            result.setInfo(juce::translate("Time Zoom In"), juce::translate("Zooms in on the time range"), "View", 0);
            result.defaultKeypresses.add(juce::KeyPress('+', juce::ModifierKeys::commandModifier, 0));
            result.setActive(Zoom::Tools::canZoomIn(zoomAcsr));
        }
        break;
        case CommandIDs::viewTimeZoomOut:
        {
            auto const& zoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
            result.setInfo(juce::translate("Time Zoom Out"), juce::translate("Zooms out on the time range"), "View", 0);
            result.defaultKeypresses.add(juce::KeyPress('-', juce::ModifierKeys::commandModifier, 0));
            result.setActive(Zoom::Tools::canZoomOut(zoomAcsr));
        }
        break;
        case CommandIDs::viewVerticalZoomIn:
        {
            auto const selectedItems = Document::Selection::getItems(documentAcsr);
            auto const canZoom = std::any_of(std::get<0_z>(selectedItems).cbegin(), std::get<0_z>(selectedItems).cend(), [&](auto const& groupId)
                                             {
                                                 return Group::Tools::canZoomIn(Document::Tools::getGroupAcsr(documentAcsr, groupId));
                                             }) ||
                                 std::any_of(std::get<1_z>(selectedItems).cbegin(), std::get<1_z>(selectedItems).cend(), [&](auto const& trackId)
                                             {
                                                 return Track::Tools::canZoomIn(Document::Tools::getTrackAcsr(documentAcsr, trackId));
                                             });
            result.setInfo(juce::translate("Vertical Zoom In"), juce::translate("Zooms in on the vertical range"), "View", 0);
            result.defaultKeypresses.add(juce::KeyPress('+', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(canZoom);
        }
        break;
        case CommandIDs::viewVerticalZoomOut:
        {
            auto const selectedItems = Document::Selection::getItems(documentAcsr);
            auto const canZoom = std::any_of(std::get<0_z>(selectedItems).cbegin(), std::get<0_z>(selectedItems).cend(), [&](auto const& groupId)
                                             {
                                                 return Group::Tools::canZoomOut(Document::Tools::getGroupAcsr(documentAcsr, groupId));
                                             }) ||
                                 std::any_of(std::get<1_z>(selectedItems).cbegin(), std::get<1_z>(selectedItems).cend(), [&](auto const& trackId)
                                             {
                                                 return Track::Tools::canZoomOut(Document::Tools::getTrackAcsr(documentAcsr, trackId));
                                             });
            result.setInfo(juce::translate("Vertical Zoom Out"), juce::translate("Zooms out on the vertical range"), "View", 0);
            result.defaultKeypresses.add(juce::KeyPress('-', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(canZoom);
        }
        break;
        case CommandIDs::viewInfoBubble:
        {
            result.setInfo(juce::translate("Info Bubble"), juce::translate("Toggles the info bubble info"), "View", 0);
            result.defaultKeypresses.add(juce::KeyPress('i', juce::ModifierKeys::commandModifier, 0));
            result.setActive(true);
            result.setTicked(Instance::get().getApplicationAccessor().getAttr<AttrType::showInfoBubble>());
        }
        break;

        case CommandIDs::helpOpenAudioSettings:
        {
#if JUCE_MAC
            result.setInfo(juce::translate("Audio Settings..."), juce::translate("Shows the audio settings panel"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress(',', juce::ModifierKeys::commandModifier, 0));
#else
            result.setInfo(juce::translate("Audio Settings..."), juce::translate("Shows the audio settings panel"), "Help", 0);
            result.defaultKeypresses.add(juce::KeyPress('p', juce::ModifierKeys::commandModifier, 0));
#endif
        }
        break;
        case CommandIDs::helpOpenPluginSettings:
        {
            result.setInfo(juce::translate("Plugin Settings..."), juce::translate("Shows the plugin settings panel"), "Application", 0);
        }
        break;
        case CommandIDs::helpOpenAbout:
        {
#if JUCE_MAC
            result.setInfo(juce::translate("About Partiels"), juce::translate("Shows the information about the application"), "Application", 0);
#else
            result.setInfo(juce::translate("About Partiels"), juce::translate("Shows the information about the application"), "help", 0);
#endif
        }
        break;
        case CommandIDs::helpOpenProjectPage:
        {
            result.setInfo(juce::translate("Proceed to Project Page"), juce::translate("Opens the project page of the Ircam Forum in a web browser"), "Help", 0);
            result.setActive(true);
        }
        break;
        case CommandIDs::helpAuthorize:
        {
            auto const isAuthorized = Instance::get().getAuthorizationProcessor().isAuthorized();
            result.setInfo(isAuthorized ? juce::translate("Authorized") : juce::translate("Authorize"), juce::translate("Authorize the application"), "Help", 0);
            result.setActive(!isAuthorized);
        }
        break;
        case CommandIDs::helpSdifConverter:
        {
            result.setInfo(juce::translate("SDIF Converter..."), juce::translate("Shows the SDIF converter panel"), "Help", 0);
            result.setActive(true);
        }
        break;
        case CommandIDs::helpAutoUpdate:
        {
            result.setInfo(juce::translate("Automatic Check for New Version"), juce::translate("Toggles the automatic check for a new version"), "Help", 0);
            result.setActive(true);
            result.setTicked(Instance::get().getApplicationAccessor().getAttr<AttrType::autoUpdate>());
        }
        break;
        case CommandIDs::helpCheckForUpdate:
        {
            result.setInfo(juce::translate("Check for New Version"), juce::translate("Checks for a new version"), "Help", 0);
            result.setActive(true);
        }
        break;
    }

    Instance::get().getCommandInfo(commandID, result);
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
        case CommandIDs::documentNew:
        {
            Instance::get().newDocument();
            return true;
        }
        case CommandIDs::documentOpen:
        {
            auto const wildcard = Instance::getWildCardForAudioFormats() + ";" + Instance::getWildCardForDocumentFile();
            mFileChooser = std::make_unique<juce::FileChooser>(getCommandDescription(), Instance::get().getDocumentFileBased().getFile(), wildcard);
            if(mFileChooser == nullptr)
            {
                return true;
            }
            using Flags = juce::FileBrowserComponent::FileChooserFlags;
            mFileChooser->launchAsync(Flags::openMode | Flags::canSelectFiles | Flags::canSelectMultipleItems, [&](juce::FileChooser const& fileChooser)
                                      {
                                          auto const results = fileChooser.getResults();
                                          if(results.isEmpty())
                                          {
                                              return;
                                          }
                                          std::vector<juce::File> files;
                                          for(auto const& result : results)
                                          {
                                              files.push_back(result);
                                          }
                                          Instance::get().openFiles(files);
                                      });

            return true;
        }
        case CommandIDs::documentSave:
        {
            fileBased.saveAsync(true, true, [](juce::FileBasedDocument::SaveResult saveResult)
                                {
                                    juce::ignoreUnused(saveResult);
                                });
            return true;
        }
        case CommandIDs::documentDuplicate:
        {
            fileBased.saveAsInteractiveAsync(true, [](juce::FileBasedDocument::SaveResult saveResult)
                                             {
                                                 juce::ignoreUnused(saveResult);
                                             });
            return true;
        }
        case CommandIDs::documentConsolidate:
        {
            fileBased.saveAsync(true, true, [](juce::FileBasedDocument::SaveResult saveResult)
                                {
                                    if(saveResult != juce::FileBasedDocument::SaveResult::savedOk)
                                    {
                                        return;
                                    }
                                    auto const result = Instance::get().getDocumentFileBased().consolidate();
                                    auto const fileName = Instance::get().getDocumentFileBased().getFile().getFullPathName();
                                    if(result.wasOk())
                                    {
                                        auto const options = juce::MessageBoxOptions()
                                                                 .withIconType(juce::AlertWindow::InfoIcon)
                                                                 .withTitle(juce::translate("Document consolidated!"))
                                                                 .withMessage(juce::translate("The document has been consolidated with the audio files and the analyses to FLNAME.").replace("FLNAME", fileName))
                                                                 .withButton(juce::translate("Ok"));
                                        juce::AlertWindow::showAsync(options, nullptr);
                                    }
                                    else
                                    {
                                        auto const options = juce::MessageBoxOptions()
                                                                 .withIconType(juce::AlertWindow::WarningIcon)
                                                                 .withTitle(juce::translate("Document consolidation failed!"))
                                                                 .withMessage(juce::translate("The document cannot be consolidated with the audio files and the analyses to FLNAME: ERROR.").replace("FLNAME", fileName).replace("ERROR", result.getErrorMessage()))
                                                                 .withButton(juce::translate("Ok"));
                                        juce::AlertWindow::showAsync(options, nullptr);
                                    }
                                });
            return true;
        }

        case CommandIDs::documentExport:
        {
            if(!Instance::get().getAuthorizationProcessor().isAuthorized())
            {
                Instance::get().getAuthorizationProcessor().showAuthorizationPanel();
                juce::LookAndFeel::getDefaultLookAndFeel().playAlertSound();
                return true;
            }
            mExporterWindow.show(true);
            return true;
        }
        case CommandIDs::documentImport:
        {
            auto const position = Tools::getNewTrackPosition();
            mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Load file"), juce::File{}, Instance::getWildCardForImportFormats());
            if(mFileChooser == nullptr)
            {
                return true;
            }
            using Flags = juce::FileBrowserComponent::FileChooserFlags;
            mFileChooser->launchAsync(Flags::openMode | Flags::canSelectFiles, [=](juce::FileChooser const& fileChooser)
                                      {
                                          auto const results = fileChooser.getResults();
                                          if(results.isEmpty())
                                          {
                                              return;
                                          }
                                          Instance::get().importFile(position, results.getFirst());
                                      });
            return true;
        }
        case CommandIDs::documentBatch:
        {
            if(!Instance::get().getAuthorizationProcessor().isAuthorized())
            {
                Instance::get().getAuthorizationProcessor().showAuthorizationPanel();
                juce::LookAndFeel::getDefaultLookAndFeel().playAlertSound();
                return true;
            }
            mBatcherWindow.show(true);
            return true;
        }

        case CommandIDs::editUndo:
        {
            Instance::get().getUndoManager().undo();
            return true;
        }
        case CommandIDs::editRedo:
        {
            Instance::get().getUndoManager().redo();
            return true;
        }
        case CommandIDs::editSelectNextItem:
        {
            auto const nextItem = Document::Selection::getNextItem(documentAcsr, true);
            if(nextItem.has_value())
            {
                Document::Selection::selectItem(documentAcsr, {*nextItem, {}}, true, false, NotificationType::synchronous);
            }
            return true;
        }
        case CommandIDs::editNewGroup:
        {
            auto& documentDir = Instance::get().getDocumentDirector();
            documentDir.startAction();

            auto const position = Tools::getNewGroupPosition();
            auto const identifier = documentDir.addGroup(position, NotificationType::synchronous);
            if(identifier.has_value())
            {
                documentDir.endAction(ActionState::newTransaction, juce::translate("New Group"));
                Document::Selection::selectItem(documentAcsr, {*identifier, {}}, true, false, NotificationType::synchronous);
            }
            else
            {
                documentDir.endAction(ActionState::abort);
                auto const options = juce::MessageBoxOptions()
                                         .withIconType(juce::AlertWindow::WarningIcon)
                                         .withTitle(juce::translate("Group cannot be created!"))
                                         .withMessage(juce::translate("The group cannot be inserted into the document."))
                                         .withButton(juce::translate("Ok"));
                juce::AlertWindow::showAsync(options, nullptr);
            }
            return true;
        }
        case CommandIDs::editNewTrack:
        {
            auto const position = Tools::getNewTrackPosition();
            mPluginListTable.setMultipleSelectionEnabled(true);
            mPluginListTable.onPluginSelected = [this, position](std::set<Plugin::Key> keys)
            {
                mPluginListTableWindow.hide();
                Tools::addPluginTracks(position, keys);
            };
            mPluginListTableWindow.show(true);
            mPluginListTable.grabKeyboardFocus();
            return true;
        }
        case CommandIDs::editRemoveItem:
        {
            auto const selectedItems = Document::Selection::getItems(documentAcsr);
            if(!std::get<0_z>(selectedItems).empty() && std::get<1_z>(selectedItems).empty())
            {
                juce::StringArray names;
                for(auto const& groupId : std::get<0_z>(selectedItems))
                {
                    auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupId);
                    names.add(groupAcsr.getAttr<Group::AttrType::name>());
                }
                auto const options = juce::MessageBoxOptions()
                                         .withIconType(juce::AlertWindow::QuestionIcon)
                                         .withTitle(juce::translate("Remove Group(s)?"))
                                         .withMessage(juce::translate("Are you sure you want to remove the group(s) \"GROUPNAMES\" from the project? This will also remove all the contained tracks!").replace("GROUPNAMES", names.joinIntoString(", ")))
                                         .withButton(juce::translate("Remove"))
                                         .withButton(juce::translate("Cancel"));
                juce::AlertWindow::showAsync(options, [=](int windowResult)
                                             {
                                                 if(windowResult != 1)
                                                 {
                                                     return;
                                                 }
                                                 auto& documentDir = Instance::get().getDocumentDirector();
                                                 documentDir.startAction();
                                                 for(auto const& groupId : std::get<0_z>(selectedItems))
                                                 {
                                                     documentDir.removeGroup(groupId, NotificationType::synchronous);
                                                 }
                                                 documentDir.endAction(ActionState::newTransaction, juce::translate("Remove Group(s)"));
                                             });
                return true;
            }
            else if(std::get<0_z>(selectedItems).empty() && !std::get<1_z>(selectedItems).empty())
            {
                juce::StringArray names;
                for(auto const& trackId : std::get<1_z>(selectedItems))
                {
                    auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, trackId);
                    names.add(trackAcsr.getAttr<Track::AttrType::name>());
                }
                auto const options = juce::MessageBoxOptions()
                                         .withIconType(juce::AlertWindow::QuestionIcon)
                                         .withTitle(juce::translate("Remove Track?"))
                                         .withMessage(juce::translate("Are you sure you want to remove the track(s) \"TRACKNAMES\" from the project?").replace("TRACKNAMES", names.joinIntoString(", ")))
                                         .withButton(juce::translate("Remove"))
                                         .withButton(juce::translate("Cancel"));
                juce::AlertWindow::showAsync(options, [=](int windowResult)
                                             {
                                                 if(windowResult != 1)
                                                 {
                                                     return;
                                                 }
                                                 auto& documentDir = Instance::get().getDocumentDirector();
                                                 documentDir.startAction();
                                                 for(auto const& trackId : std::get<1_z>(selectedItems))
                                                 {
                                                     documentDir.removeTrack(trackId, NotificationType::synchronous);
                                                 }
                                                 documentDir.endAction(ActionState::newTransaction, juce::translate("Remove Track(s)"));
                                             });
                return true;
            }
            else if(!std::get<0_z>(selectedItems).empty() && !std::get<1_z>(selectedItems).empty())
            {
                juce::StringArray groupNames, trackNames;
                for(auto const& groupId : std::get<0_z>(selectedItems))
                {
                    auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupId);
                    groupNames.add(groupAcsr.getAttr<Group::AttrType::name>());
                }
                for(auto const& trackId : std::get<1_z>(selectedItems))
                {
                    auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, trackId);
                    trackNames.add(trackAcsr.getAttr<Track::AttrType::name>());
                }
                auto const options = juce::MessageBoxOptions()
                                         .withIconType(juce::AlertWindow::QuestionIcon)
                                         .withTitle(juce::translate("Remove Group(s) and Track(s)?"))
                                         .withMessage(juce::translate("Are you sure you want to remove the group(s) \"GROUPNAMES\" and the from the project and the track(s) \"TRACKNAMES\"?").replace("GROUPNAMES", groupNames.joinIntoString(", ")).replace("TRACKNAMES", trackNames.joinIntoString(", ")))
                                         .withButton(juce::translate("Remove"))
                                         .withButton(juce::translate("Cancel"));
                juce::AlertWindow::showAsync(options, [=](int windowResult)
                                             {
                                                 if(windowResult != 1)
                                                 {
                                                     return;
                                                 }
                                                 auto& documentDir = Instance::get().getDocumentDirector();
                                                 documentDir.startAction();
                                                 for(auto const& groupId : std::get<0_z>(selectedItems))
                                                 {
                                                     documentDir.removeGroup(groupId, NotificationType::synchronous);
                                                 }
                                                 for(auto const& trackId : std::get<1_z>(selectedItems))
                                                 {
                                                     documentDir.removeTrack(trackId, NotificationType::synchronous);
                                                 }
                                                 documentDir.endAction(ActionState::newTransaction, juce::translate("Remove Group(s) and Track(s)"));
                                             });
                return true;
            }
            return true;
        }
        case CommandIDs::editLoadTemplate:
        {
            auto const wildcard = Instance::getWildCardForDocumentFile();
            mFileChooser = std::make_unique<juce::FileChooser>(getCommandDescription(), Instance::get().getDocumentFileBased().getFile(), wildcard);
            if(mFileChooser == nullptr)
            {
                return true;
            }
            using Flags = juce::FileBrowserComponent::FileChooserFlags;
            mFileChooser->launchAsync(Flags::openMode | Flags::canSelectFiles, [=](juce::FileChooser const& fileChooser)
                                      {
                                          auto const results = fileChooser.getResults();
                                          if(results.isEmpty())
                                          {
                                              return;
                                          }

                                          auto& documentDir = Instance::get().getDocumentDirector();
                                          documentDir.startAction();
                                          auto const adaptation = Instance::get().getApplicationAccessor().getAttr<AttrType::adaptationToSampleRate>();
                                          auto const loadResults = Instance::get().getDocumentFileBased().loadTemplate(results.getFirst(), adaptation);
                                          if(loadResults.failed())
                                          {
                                              auto const options = juce::MessageBoxOptions()
                                                                       .withIconType(juce::AlertWindow::WarningIcon)
                                                                       .withTitle(juce::translate("Failed to load template!"))
                                                                       .withMessage(loadResults.getErrorMessage())
                                                                       .withButton(juce::translate("Ok"));
                                              juce::AlertWindow::showAsync(options, nullptr);
                                              documentDir.endAction(ActionState::abort);
                                          }
                                          else
                                          {
                                              documentDir.endAction(ActionState::newTransaction, juce::translate("Load template"));
                                          }
                                      });
            return true;
        }

        case CommandIDs::transportTogglePlayback:
        {
            auto constexpr attr = Transport::AttrType::playback;
            auto const state = !transportAcsr.getAttr<attr>();
            auto const& deviceManager = Instance::get().getAudioDeviceManager();
            if(state && deviceManager.getCurrentAudioDevice() == nullptr)
            {
                auto const options = juce::MessageBoxOptions()
                                         .withIconType(juce::AlertWindow::WarningIcon)
                                         .withTitle(juce::translate("Audio device not set!"))
                                         .withMessage(juce::translate("Audio playback cannot be started because the output audio device is not set. Do you want to open the audio settings panel to select the output audio device?"))
                                         .withButton(juce::translate("Open"))
                                         .withButton(juce::translate("Ignore"));
                juce::WeakReference<CommandTarget> weakReference(this);
                juce::AlertWindow::showAsync(options, [=, this](int result)
                                             {
                                                 if(weakReference.get() == nullptr)
                                                 {
                                                     return;
                                                 }
                                                 if(result == 1)
                                                 {
                                                     mAudioSettingsWindow.show();
                                                 }
                                             });
                return true;
            }
            transportAcsr.setAttr<attr>(state, NotificationType::synchronous);
            return true;
        }
        case CommandIDs::transportToggleLooping:
        {
            auto constexpr attr = Transport::AttrType::looping;
            auto const selectionRange = transportAcsr.getAttr<Transport::AttrType::selection>();
            auto const loopRange = transportAcsr.getAttr<Transport::AttrType::loopRange>();
            if(loopRange != selectionRange)
            {
                transportAcsr.setAttr<Transport::AttrType::loopRange>(selectionRange, NotificationType::synchronous);
                transportAcsr.setAttr<attr>(true, NotificationType::synchronous);
            }
            else
            {
                transportAcsr.setAttr<attr>(!transportAcsr.getAttr<attr>(), NotificationType::synchronous);
            }
            return true;
        }
        case CommandIDs::transportToggleStopAtLoopEnd:
        {
            auto constexpr attr = Transport::AttrType::stopAtLoopEnd;
            transportAcsr.setAttr<attr>(!transportAcsr.getAttr<attr>(), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::transportToggleMagnetism:
        {
            auto constexpr attr = Transport::AttrType::magnetize;
            transportAcsr.setAttr<attr>(!transportAcsr.getAttr<attr>(), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::transportRewindPlayHead:
        {
            Transport::Tools::rewindPlayhead(transportAcsr, NotificationType::synchronous);
            return true;
        }
        case CommandIDs::transportMovePlayHeadBackward:
        {
            auto const& timeZoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
            Transport::Tools::movePlayheadBackward(transportAcsr, timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::transportMovePlayHeadForward:
        {
            auto const& timeZoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
            Transport::Tools::movePlayheadForward(transportAcsr, timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), NotificationType::synchronous);
            return true;
        }

        case CommandIDs::viewTimeZoomIn:
        {
            Zoom::Tools::zoomIn(documentAcsr.getAcsr<Document::AcsrType::timeZoom>(), 0.01, NotificationType::synchronous);
            return true;
        }
        case CommandIDs::viewTimeZoomOut:
        {
            Zoom::Tools::zoomOut(documentAcsr.getAcsr<Document::AcsrType::timeZoom>(), 0.01, NotificationType::synchronous);
            return true;
        }
        case CommandIDs::viewVerticalZoomIn:
        {
            auto const selectedItems = Document::Selection::getItems(documentAcsr);
            for(auto& groupId : std::get<0_z>(selectedItems))
            {
                Group::Tools::zoomIn(Document::Tools::getGroupAcsr(documentAcsr, groupId), 0.01, NotificationType::synchronous);
            }
            for(auto& trackId : std::get<1_z>(selectedItems))
            {
                Track::Tools::zoomIn(Document::Tools::getTrackAcsr(documentAcsr, trackId), 0.01, NotificationType::synchronous);
            }
            return true;
        }
        case CommandIDs::viewVerticalZoomOut:
        {
            auto const selectedItems = Document::Selection::getItems(documentAcsr);
            for(auto& groupId : std::get<0_z>(selectedItems))
            {
                Group::Tools::zoomOut(Document::Tools::getGroupAcsr(documentAcsr, groupId), 0.01, NotificationType::synchronous);
            }
            for(auto& trackId : std::get<1_z>(selectedItems))
            {
                Track::Tools::zoomOut(Document::Tools::getTrackAcsr(documentAcsr, trackId), 0.01, NotificationType::synchronous);
            }
            return true;
        }
        case CommandIDs::viewInfoBubble:
        {
            auto& accessor = Instance::get().getApplicationAccessor();
            accessor.setAttr<AttrType::showInfoBubble>(!accessor.getAttr<AttrType::showInfoBubble>(), NotificationType::synchronous);
            return true;
        }

        case CommandIDs::helpOpenAudioSettings:
        {
            mAudioSettingsWindow.show();
            return true;
        }
        case CommandIDs::helpOpenPluginSettings:
        {
            mPluginListSearchPathWindow.show();
            return true;
        }
        case CommandIDs::helpOpenAbout:
        {
            mAboutWindow.show();
            return true;
        }
        case CommandIDs::helpOpenProjectPage:
        {
            juce::URL const url("https://forum.ircam.fr/projects/detail/partiels/");
            if(url.isWellFormed())
            {
                url.launchInDefaultBrowser();
            }
            return true;
        }
        case CommandIDs::helpAuthorize:
        {
            if(!Instance::get().getAuthorizationProcessor().isAuthorized())
            {
                Instance::get().getAuthorizationProcessor().showAuthorizationPanel();
            }
            return true;
        }
        case CommandIDs::helpSdifConverter:
        {
            mSdifConverterWindow.show();
            return true;
        }
        case CommandIDs::helpAutoUpdate:
        {
            auto const state = Instance::get().getApplicationAccessor().getAttr<AttrType::autoUpdate>();
            Instance::get().getApplicationAccessor().setAttr<AttrType::autoUpdate>(!state, NotificationType::synchronous);
            return true;
        }
        case CommandIDs::helpCheckForUpdate:
        {
            Instance::get().checkForNewVersion(true, true);
            return true;
        }
    }
    return Instance::get().perform(info);
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

ANALYSE_FILE_END
