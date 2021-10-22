#include "AnlApplicationCommandTarget.h"
#include "../Document/AnlDocumentTools.h"
#include "../Track/AnlTrackExporter.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationTools.h"

ANALYSE_FILE_BEGIN

Application::CommandTarget::CommandTarget()
: mPluginListTable(Instance::get().getPluginListAccessor(), Instance::get().getPluginListScanner())
, mPluginListSearchPath(Instance::get().getPluginListAccessor())
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
            case AttrType::adaptationToSampleRate:
            case AttrType::autoLoadConvertedFile:
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
        , CommandIDs::editNewGroup
        , CommandIDs::editNewTrack
        , CommandIDs::editRemoveItem
        , CommandIDs::editLoadTemplate
        
        , CommandIDs::transportTogglePlayback
        , CommandIDs::transportToggleLooping
        , CommandIDs::transportRewindPlayHead
        
        , CommandIDs::viewZoomIn
        , CommandIDs::viewZoomOut
        , CommandIDs::viewInfoBubble
        
        , CommandIDs::helpOpenAudioSettings
        , CommandIDs::helpOpenPluginSettings
        , CommandIDs::helpOpenAbout
        , CommandIDs::helpOpenProjectPage
        , CommandIDs::helpSdifConverter
    });
    // clang-format on
}

void Application::CommandTarget::getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result)
{
    auto const& documentAcsr = Instance::get().getDocumentAccessor();
    auto const& transportAcsr = documentAcsr.getAcsr<Document::AcsrType::transport>();
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
            auto focusedTrack = Document::Tools::getFocusedTrack(documentAcsr);
            auto focusedGroup = Document::Tools::getFocusedGroup(documentAcsr);
            if(focusedTrack.has_value())
            {
                result.setInfo(juce::translate("Remove Track"), juce::translate("Removes the selected track"), "Edit", 0);
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
        case CommandIDs::transportRewindPlayHead:
        {
            result.setInfo(juce::translate("Rewind Playhead"), juce::translate("Moves the playhead to the start of the document"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress('w', juce::ModifierKeys::commandModifier, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty() && transportAcsr.getAttr<Transport::AttrType::runningPlayhead>() > 0.0);
        }
        break;

        case CommandIDs::viewZoomIn:
        {
            auto const& zoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
            result.setInfo(juce::translate("Zoom In"), juce::translate("Opens the manual in a web browser"), "View", 0);
            result.defaultKeypresses.add(juce::KeyPress('+', juce::ModifierKeys::commandModifier, 0));
            result.setActive(zoomAcsr.getAttr<Zoom::AttrType::visibleRange>().getLength() > zoomAcsr.getAttr<Zoom::AttrType::minimumLength>());
        }
        break;
        case CommandIDs::viewZoomOut:
        {
            auto const& zoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
            result.setInfo(juce::translate("Zoom Out"), juce::translate("Opens the manual in a web browser"), "View", 0);
            result.defaultKeypresses.add(juce::KeyPress('-', juce::ModifierKeys::commandModifier, 0));
            result.setActive(zoomAcsr.getAttr<Zoom::AttrType::visibleRange>().getLength() < zoomAcsr.getAttr<Zoom::AttrType::globalRange>().getLength());
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
        case CommandIDs::helpSdifConverter:
        {
            result.setInfo(juce::translate("SDIF Converter..."), juce::translate("Shows the SDIF converter panel"), "Help", 0);
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
        case CommandIDs::documentNew:
        {
            fileBased.saveIfNeededAndUserAgreesAsync([](juce::FileBasedDocument::SaveResult saveResult)
                                                     {
                                                         if(saveResult != juce::FileBasedDocument::SaveResult::savedOk)
                                                         {
                                                             return;
                                                         }
                                                         Instance::get().openFiles({});
                                                     });
            return true;
        }
        case CommandIDs::documentOpen:
        {
            juce::WeakReference<Application::CommandTarget> safePointer(this);
            fileBased.saveIfNeededAndUserAgreesAsync([=, this, description = getCommandDescription()](juce::FileBasedDocument::SaveResult saveResult)
                                                     {
                                                         if(safePointer.get() == nullptr)
                                                         {
                                                             return;
                                                         }
                                                         if(saveResult != juce::FileBasedDocument::SaveResult::savedOk)
                                                         {
                                                             return;
                                                         }
                                                         auto const wildcard = Instance::get().getAudioFormatManager().getWildcardForAllFormats() + ";" + Instance::getFileWildCard();
                                                         mFileChooser = std::make_unique<juce::FileChooser>(description, Instance::get().getDocumentFileBased().getFile(), wildcard);
                                                         if(mFileChooser == nullptr)
                                                         {
                                                             return;
                                                         }
                                                         using Flags = juce::FileBrowserComponent::FileChooserFlags;
                                                         mFileChooser->launchAsync(Flags::openMode | Flags::canSelectFiles | Flags::canSelectMultipleItems, [](juce::FileChooser const& fileChooser)
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
                                    if(result.wasOk())
                                    {
                                        AlertWindow::showMessage(AlertWindow::MessageType::info, "Document consolidated!", "The document has been consolidated with the audio files and the analyses to FLNAME.", {{"FLNAME", Instance::get().getDocumentFileBased().getFile().getFullPathName()}});
                                    }
                                    else
                                    {
                                        AlertWindow::showMessage(AlertWindow::MessageType::warning, "Document consolidation failed!", "The document cannot be consolidated with the audio files and the analyses to FLNAME. ERROR", {{"FLNAME", Instance::get().getDocumentFileBased().getFile().getFullPathName()}, {"ERROR", result.getErrorMessage()}});
                                    }
                                });
            return true;
        }

        case CommandIDs::documentExport:
        {
            if(auto* exporter = Instance::get().getExporter())
            {
                exporter->show();
            }
            return true;
        }
        case CommandIDs::documentImport:
        {
            auto const position = Tools::getNewTrackPosition();
            mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Load file"), juce::File{}, "*.json;*.csv;*.cue;*.sdif");
            if(mFileChooser == nullptr)
            {
                return true;
            }
            using Flags = juce::FileBrowserComponent::FileChooserFlags;
            juce::WeakReference<Application::CommandTarget> safePointer(this);
            mFileChooser->launchAsync(Flags::openMode | Flags::canSelectFiles, [=, this](juce::FileChooser const& fileChooser)
                                      {
                                          auto const results = fileChooser.getResults();
                                          if(results.isEmpty())
                                          {
                                              return;
                                          }
                                          if(safePointer.get() == nullptr)
                                          {
                                              return;
                                          }
                                          auto const file = results.getFirst();
                                          if(file.hasFileExtension("sdif"))
                                          {
                                              auto selector = std::make_unique<Track::Loader::SdifArgumentSelector>(file);
                                              if(selector != nullptr)
                                              {
                                                  selector->onLoad = [=, this](Track::FileInfo fileInfo)
                                                  {
                                                      Tools::addFileTrack(position, fileInfo);
                                                      if(safePointer.get() == nullptr)
                                                      {
                                                          return;
                                                      }
                                                      mSdifSelector.reset();
                                                  };
                                                  selector->show();
                                              }
                                              mSdifSelector = std::move(selector);
                                          }
                                          else
                                          {
                                              Tools::addFileTrack(position, file);
                                          }
                                      });
            return true;
        }
        case CommandIDs::documentBatch:
        {
            if(auto* batcher = Instance::get().getBatcher())
            {
                batcher->show();
            }
            return true;
        }

        case CommandIDs::editUndo:
        {
            auto& undoManager = Instance::get().getUndoManager();
            undoManager.undo();
            return true;
        }
        case CommandIDs::editRedo:
        {
            auto& undoManager = Instance::get().getUndoManager();
            undoManager.redo();
            return true;
        }
        case CommandIDs::editNewGroup:
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
        case CommandIDs::editNewTrack:
        {
            auto const position = Tools::getNewTrackPosition();
            mPluginListTable.onPluginSelected = [this, position](Plugin::Key const& key, Plugin::Description const& description)
            {
                mPluginListTable.hide();
                Tools::addPluginTrack(position, key, description);
            };
            mPluginListTable.show();
            return true;
        }
        case CommandIDs::editRemoveItem:
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
        case CommandIDs::editLoadTemplate:
        {
            struct DocumentCtn
            {
                DocumentCtn() = default;
                ~DocumentCtn() = default;

                Document::Accessor accessor;
                juce::UndoManager undoManager;
                Document::Director director{accessor, Instance::get().getAudioFormatManager(), undoManager};
                Document::FileBased fileBased{director, Instance::getFileExtension(), Instance::getFileWildCard(), juce::translate("Open a document"), juce::translate("Save the document")};
            };

            auto tempDocument = std::make_shared<DocumentCtn>();
            if(tempDocument == nullptr)
            {
                return true;
            }

            auto& copyFileBased = tempDocument->fileBased;
            copyFileBased.loadFromUserSpecifiedFileAsync(true, [tempDocument](juce::Result loadResult)
                                                         {
                                                             anlWeakAssert(tempDocument != nullptr);
                                                             if(tempDocument == nullptr || loadResult.failed())
                                                             {
                                                                 return;
                                                             }
                                                             tempDocument->accessor.setAttr<Document::AttrType::reader>(Instance::get().getDocumentAccessor().getAttr<Document::AttrType::reader>(), NotificationType::synchronous);

                                                             auto& documentDir = Instance::get().getDocumentDirector();
                                                             documentDir.startAction();
                                                             Instance::get().getDocumentAccessor().copyFrom(tempDocument->accessor, NotificationType::synchronous);
                                                             documentDir.endAction(ActionState::newTransaction, juce::translate("Load template"));
                                                         });
            return true;
        }

        case CommandIDs::transportTogglePlayback:
        {
            auto constexpr attr = Transport::AttrType::playback;
            transportAcsr.setAttr<attr>(!transportAcsr.getAttr<attr>(), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::transportToggleLooping:
        {
            auto constexpr attr = Transport::AttrType::looping;
            transportAcsr.setAttr<attr>(!transportAcsr.getAttr<attr>(), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::transportRewindPlayHead:
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

        case CommandIDs::viewZoomIn:
        {
            auto& zoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
            auto const range = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            auto const grange = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range.expanded(grange.getLength() / -100.0), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::viewZoomOut:
        {
            auto& zoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
            auto const range = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            auto const grange = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range.expanded(grange.getLength() / 100.0), NotificationType::synchronous);
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
            if(auto* audioSettings = Instance::get().getAudioSettings())
            {
                audioSettings->show();
            }
            return true;
        }
        case CommandIDs::helpOpenPluginSettings:
        {
            mPluginListSearchPath.show();
            return true;
        }
        case CommandIDs::helpOpenAbout:
        {
            if(auto* about = Instance::get().getAbout())
            {
                about->show();
            }
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
        case CommandIDs::helpSdifConverter:
        {
            mSdifConverter.show();
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

ANALYSE_FILE_END
