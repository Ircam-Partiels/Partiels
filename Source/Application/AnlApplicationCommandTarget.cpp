#include "AnlApplicationCommandTarget.h"
#include "../Document/AnlDocumentSelection.h"
#include "../Document/AnlDocumentTools.h"
#include "../Track/AnlTrackExporter.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationTools.h"

ANALYSE_FILE_BEGIN

Application::CommandTarget::CommandTarget()
{
    mListener.onAttrChanged = []([[maybe_unused]] Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::recentlyOpenedFilesList:
            case AttrType::defaultTemplateFile:
            case AttrType::currentTranslationFile:
            case AttrType::silentFileManagement:
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
            case AttrType::timeZoomAnchorOnPlayhead:
                break;
        }
    };

    mTransportListener.onAttrChanged = []([[maybe_unused]] Transport::Accessor const& acsr, Transport::AttrType attribute)
    {
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

    mDocumentListener.onAccessorInserted = []([[maybe_unused]] Document::Accessor const& acsr, Document::AcsrType type, [[maybe_unused]] size_t index)
    {
        switch(type)
        {
            case Document::AcsrType::tracks:
            case Document::AcsrType::groups:
            {
                Instance::get().getApplicationCommandManager().commandStatusChanged();
            }
            break;
            case Document::AcsrType::timeZoom:
            case Document::AcsrType::transport:
                break;
        }
    };

    mDocumentListener.onAccessorErased = []([[maybe_unused]] Document::Accessor const& acsr, Document::AcsrType type, [[maybe_unused]] size_t index)
    {
        switch(type)
        {
            case Document::AcsrType::tracks:
            case Document::AcsrType::groups:
            {
                Instance::get().getApplicationCommandManager().commandStatusChanged();
            }
            break;
            case Document::AcsrType::timeZoom:
            case Document::AcsrType::transport:
                break;
        }
    };

    mDocumentListener.onAttrChanged = []([[maybe_unused]] Document::Accessor const& acsr, [[maybe_unused]] Document::AttrType attribute)
    {
        Instance::get().getApplicationCommandManager().commandStatusChanged();
    };

    Instance::get().getDocumentFileBased().addChangeListener(this);
    Instance::get().getUndoManager().addChangeListener(this);
    Instance::get().getOscSender().addChangeListener(this);
    Instance::get().getApplicationAccessor().addListener(mListener, NotificationType::synchronous);
    Instance::get().getDocumentAccessor().getAcsr<Document::AcsrType::transport>().addListener(mTransportListener, NotificationType::synchronous);
    Instance::get().getDocumentAccessor().addListener(mDocumentListener, NotificationType::synchronous);
    Instance::get().getApplicationCommandManager().registerAllCommandsForTarget(this);
}

Application::CommandTarget::~CommandTarget()
{
    Instance::get().getDocumentAccessor().removeListener(mDocumentListener);
    Instance::get().getDocumentAccessor().getAcsr<Document::AcsrType::transport>().removeListener(mTransportListener);
    Instance::get().getApplicationAccessor().removeListener(mListener);
    Instance::get().getOscSender().removeChangeListener(this);
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
        , CommandIDs::editSelectNextItem
        , CommandIDs::editNewGroup
        , CommandIDs::editNewTrack
        , CommandIDs::editRemoveItem
        , CommandIDs::editLoadTemplate
        
        , CommandIDs::frameSelectAll
        , CommandIDs::frameDelete
        , CommandIDs::frameCopy
        , CommandIDs::frameCut
        , CommandIDs::framePaste
        , CommandIDs::frameDuplicate
        , CommandIDs::frameInsert
        , CommandIDs::frameBreak
        , CommandIDs::frameResetDurationToZero
        , CommandIDs::frameResetDurationToFull
        , CommandIDs::frameSystemCopy
        , CommandIDs::frameToggleDrawing
        
        , CommandIDs::transportTogglePlayback
        , CommandIDs::transportToggleLooping
        , CommandIDs::transportToggleStopAtLoopEnd
        , CommandIDs::transportToggleMagnetism
        , CommandIDs::transportRewindPlayHead
        , CommandIDs::transportMovePlayHeadBackward
        , CommandIDs::transportMovePlayHeadForward
        , CommandIDs::transportOscConnected
        
        , CommandIDs::viewTimeZoomIn
        , CommandIDs::viewTimeZoomOut
        , CommandIDs::viewVerticalZoomIn
        , CommandIDs::viewVerticalZoomOut
        , CommandIDs::viewTimeZoomAnchorOnPlayhead
        , CommandIDs::viewInfoBubble
        , CommandIDs::viewShowItemProperties
        
        , CommandIDs::helpOpenAudioSettings
        , CommandIDs::helpOpenOscSettings
        , CommandIDs::helpOpenPluginSettings
        , CommandIDs::helpOpenAbout
        , CommandIDs::helpOpenProjectPage
        , CommandIDs::helpSdifConverter
        , CommandIDs::helpAutoUpdate
        , CommandIDs::helpCheckForUpdate
        , CommandIDs::helpOpenKeyMappings
    });
    // clang-format on
    Instance::get().getAllCommands(commands);
}

void Application::CommandTarget::getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result)
{
    auto const& documentAcsr = Instance::get().getDocumentAccessor();
    auto const& transportAcsr = documentAcsr.getAcsr<Document::AcsrType::transport>();
    auto const& timeZoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
    auto const& selection = transportAcsr.getAttr<Transport::AttrType::selection>();
    auto const isItemMode = documentAcsr.getAttr<Document::AttrType::editMode>() == Document::EditMode::items;
    auto const isFrameMode = documentAcsr.getAttr<Document::AttrType::editMode>() == Document::EditMode::frames;
    switch(commandID)
    {
        case CommandIDs::documentNew:
        {
            result.setInfo(juce::translate("New..."), juce::translate("Creates a new document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('n', juce::ModifierKeys::commandModifier, 0));
            result.setActive(!documentAcsr.isEquivalentTo(Instance::get().getDocumentFileBased().getDefaultAccessor()));
            break;
        }
        case CommandIDs::documentOpen:
        {
            result.setInfo(juce::translate("Open..."), juce::translate("Opens a document or audio files"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('o', juce::ModifierKeys::commandModifier, 0));
            result.setActive(true);
            break;
        }
        case CommandIDs::documentSave:
        {
            result.setInfo(juce::translate("Save"), juce::translate("Saves the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('s', juce::ModifierKeys::commandModifier, 0));
            result.setActive(true);
            break;
        }
        case CommandIDs::documentDuplicate:
        {
            result.setInfo(juce::translate("Duplicate..."), juce::translate("Saves the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('s', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(true);
            break;
        }
        case CommandIDs::documentConsolidate:
        {
            result.setInfo(juce::translate("Consolidate..."), juce::translate("Consolidates the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('c', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
            break;
        }
        case CommandIDs::documentExport:
        {
            result.setInfo(juce::translate("Export..."), juce::translate("Exports the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('e', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(documentAcsr.getNumAcsrs<Document::AcsrType::tracks>() > 0_z);
            break;
        }
        case CommandIDs::documentImport:
        {
            result.setInfo(juce::translate("Import..."), juce::translate("Imports to the document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('i', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
            break;
        }
        case CommandIDs::documentBatch:
        {
            result.setInfo(juce::translate("Batch..."), juce::translate("Batch processes a set of audio files"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('b', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(documentAcsr.getNumAcsrs<Document::AcsrType::tracks>() > 0_z);
            break;
        }

        case CommandIDs::editUndo:
        {
            auto& undoManager = Instance::get().getUndoManager();
            auto const actionName = undoManager.canUndo() ? juce::String(" ") + undoManager.getUndoDescription() : juce::String();
            result.setInfo(juce::translate("Undo") + actionName, juce::translate("Undoes the last action"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0));
            result.setActive(undoManager.canUndo());
            break;
        }
        case CommandIDs::editRedo:
        {
            auto& undoManager = Instance::get().getUndoManager();
            auto const actionName = undoManager.canRedo() ? juce::String(" ") + undoManager.getRedoDescription() : juce::String();
            result.setInfo(juce::translate("Redo") + actionName, juce::translate("Redoes the last action"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('z', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(undoManager.canRedo());
            break;
        }

        case CommandIDs::editSelectNextItem:
        {
            result.setInfo(juce::translate("Select Next Item"), juce::translate("Select Next Item"), "Select", 0);
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::tabKey, juce::ModifierKeys::noModifiers, 0));
            result.setActive(true);
            break;
        }
        case CommandIDs::editNewGroup:
        {
            result.setInfo(juce::translate("Add New Group"), juce::translate("Adds a new group"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('g', juce::ModifierKeys::commandModifier, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
            break;
        }
        case CommandIDs::editNewTrack:
        {
            auto const* window = Instance::get().getWindow();
            auto const hidePanel = window != nullptr && window->getInterface().isPluginListTablePanelVisible();
            result.setInfo(hidePanel ? juce::translate("Hide New Track Panel") : juce::translate("Show New Track Panel"), juce::translate("Shows or hides the list of plugins to add a new track"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('t', juce::ModifierKeys::commandModifier, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
            break;
        }
        case CommandIDs::editRemoveItem:
        {
            auto selectedItems = Document::Selection::getItems(documentAcsr);
            if(!std::get<0_z>(selectedItems).empty() && std::get<1_z>(selectedItems).empty())
            {
                result.setInfo(juce::translate("Remove Group(s)"), juce::translate("Removes the selected group(s)"), "Edit", 0);
            }
            else if(std::get<0_z>(selectedItems).empty() && !std::get<1_z>(selectedItems).empty())
            {
                result.setInfo(juce::translate("Remove Track(s)"), juce::translate("Removes the selected track(s)"), "Edit", 0);
            }
            else
            {
                result.setInfo(juce::translate("Remove Group(s) and Track(s)"), juce::translate("Removes the selected group(s) and track(s)"), "Edit", 0);
            }
            result.setActive(isItemMode && (!std::get<0_z>(selectedItems).empty() || !std::get<1_z>(selectedItems).empty()));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::backspaceKey, juce::ModifierKeys::noModifiers, 0));
#ifndef JUCE_MAC
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::deleteKey, juce::ModifierKeys::noModifiers, 0));
#endif
            break;
        }
        case CommandIDs::editLoadTemplate:
        {
            result.setInfo(juce::translate("Load Template..."), juce::translate("Loads a template"), "Edit", 0);
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
            break;
        }

        case CommandIDs::frameSelectAll:
        {
            result.setInfo(juce::translate("Select All Frames"), juce::translate("Select all frame(s)"), "Select", 0);
            result.defaultKeypresses.add(juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0));
            result.setActive(isFrameMode && timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>() != selection);
            break;
        }
        case CommandIDs::frameDelete:
        {
            result.setInfo(juce::translate("Delete Frame(s)"), juce::translate("Delete the selected frame(s)"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::backspaceKey, juce::ModifierKeys::noModifiers, 0));
#ifndef JUCE_MAC
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::deleteKey, juce::ModifierKeys::noModifiers, 0));
#endif
            result.setActive(isFrameMode && Document::Tools::containsFrames(documentAcsr, selection));
            break;
        }
        case CommandIDs::frameCopy:
        {
            result.setInfo(juce::translate("Copy Frame(s)"), juce::translate("Copy the selected frame(s) to the application clipboard"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0));
            result.setActive(isFrameMode && Document::Tools::containsFrames(documentAcsr, selection));
            break;
        }
        case CommandIDs::frameCut:
        {
            result.setInfo(juce::translate("Cut Frame(s)"), juce::translate("Cut the selected frame(s) to the application clipboard"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('x', juce::ModifierKeys::commandModifier, 0));
#ifndef JUCE_MAC
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::deleteKey, juce::ModifierKeys::shiftModifier, 0));
#endif
            result.setActive(isFrameMode && Document::Tools::containsFrames(documentAcsr, selection));
            break;
        }
        case CommandIDs::framePaste:
        {
            result.setInfo(juce::translate("Paste Frame(s)"), juce::translate("Paste frame(s) from the application clipboard"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('v', juce::ModifierKeys::commandModifier, 0));
#ifndef JUCE_MAC
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::insertKey, juce::ModifierKeys::shiftModifier, 0));
#endif
            result.setActive(isFrameMode && !Document::Tools::isClipboardEmpty(documentAcsr, mClipboard));
            break;
        }
        case CommandIDs::frameDuplicate:
        {
            result.setInfo(juce::translate("Duplicate Frame(s)"), juce::translate("Duplicate the selected frame(s)"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('d', juce::ModifierKeys::commandModifier, 0));
            result.setActive(isFrameMode && Document::Tools::containsFrames(documentAcsr, selection));
            break;
        }
        case CommandIDs::frameInsert:
        {
            result.setInfo(juce::translate("Insert Frame(s)"), juce::translate("Insert frame(s)"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('i', juce::ModifierKeys::noModifiers, 0));
            result.setActive(isFrameMode && (!Document::Tools::matchesFrame(documentAcsr, selection.getStart()) || !Document::Tools::matchesFrame(documentAcsr, selection.getEnd())));
            break;
        }
        case CommandIDs::frameBreak:
        {
            result.setInfo(juce::translate("Break Frame(s)"), juce::translate("Break frame(s)"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('b', juce::ModifierKeys::noModifiers, 0));
            result.setActive(isFrameMode && Document::Tools::canBreak(documentAcsr, selection.getStart()));
            break;
        }
        case CommandIDs::frameResetDurationToZero:
        {
            result.setInfo(juce::translate("Reset Frame(s) duration to zero"), juce::translate("Reset the selected frame(s) duration to zero"), "Edit", 0);
            result.setActive(isFrameMode && Document::Tools::containsFrames(documentAcsr, selection));
            break;
        }
        case CommandIDs::frameResetDurationToFull:
        {
            result.setInfo(juce::translate("Reset Frame(s) duration to full"), juce::translate("Reset the selected frame(s) duration to full"), "Edit", 0);
            result.setActive(isFrameMode && Document::Tools::containsFrames(documentAcsr, selection));
            break;
        }
        case CommandIDs::frameSystemCopy:
        {
            result.setInfo(juce::translate("Copy Frame(s) to System Clipboard"), juce::translate("Copy frame(s) to system clipboard"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('c', juce::ModifierKeys::altModifier, 0));
            result.setActive(isFrameMode && Document::Tools::containsFrames(documentAcsr, selection));
            break;
        }
        case CommandIDs::frameToggleDrawing:
        {
            result.setInfo(juce::translate("Toggle Drawing Mode"), juce::translate("Switch between drawing mode and navigation mode"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('e', juce::ModifierKeys::commandModifier, 0));
            result.setTicked(documentAcsr.getAttr<Document::AttrType::drawingState>());
            result.setActive(true);
            break;
        }

        case CommandIDs::transportTogglePlayback:
        {
            result.setInfo(juce::translate("Toggle Playback"), juce::translate("Starts or stops the audio playback"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::spaceKey, juce::ModifierKeys::noModifiers, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
            result.setTicked(transportAcsr.getAttr<Transport::AttrType::playback>());
            break;
        }
        case CommandIDs::transportToggleLooping:
        {
            result.setInfo(juce::translate("Toggle Loop"), juce::translate("Enables or disables the loop audio playback"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress('l', juce::ModifierKeys::commandModifier, 0));
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
            result.setTicked(transportAcsr.getAttr<Transport::AttrType::looping>());
            break;
        }
        case CommandIDs::transportToggleStopAtLoopEnd:
        {
            result.setInfo(juce::translate("Toggle Stop Playback at Loop End"), juce::translate("Enables or disables the stop playback at the end of loop if repeat disabled"), "Transport", 0);
            result.setActive(!documentAcsr.getAttr<Document::AttrType::reader>().empty());
            result.setTicked(transportAcsr.getAttr<Transport::AttrType::stopAtLoopEnd>());
            break;
        }
        case CommandIDs::transportToggleMagnetism:
        {
            result.setInfo(juce::translate("Toggle Magnetize"), juce::translate("Toggles the magnetism mechanism with markers"), "Transport", 0);
            result.setActive(!transportAcsr.getAttr<Transport::AttrType::markers>().empty());
            result.setTicked(transportAcsr.getAttr<Transport::AttrType::magnetize>());
            break;
        }
        case CommandIDs::transportRewindPlayHead:
        {
            result.setInfo(juce::translate("Rewind Playhead"), juce::translate("Moves the playhead to the start of the document"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress('w', juce::ModifierKeys::commandModifier, 0));
            auto const hasReader = !documentAcsr.getAttr<Document::AttrType::reader>().empty();
            result.setActive(hasReader && Transport::Tools::canRewindPlayhead(transportAcsr));
            break;
        }
        case CommandIDs::transportMovePlayHeadBackward:
        {
            result.setInfo(juce::translate("Move the Playhead Backward"), juce::translate("Moves the playhead to the previous marker"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::leftKey, juce::ModifierKeys::commandModifier, 0));
            auto const hasReader = !documentAcsr.getAttr<Document::AttrType::reader>().empty();
            result.setActive(hasReader && Transport::Tools::canMovePlayheadBackward(transportAcsr, timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>()));
            break;
        }
        case CommandIDs::transportMovePlayHeadForward:
        {
            result.setInfo(juce::translate("Move the Playhead Forward"), juce::translate("Moves the playhead to the previous marker"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::rightKey, juce::ModifierKeys::commandModifier, 0));
            auto const hasReader = !documentAcsr.getAttr<Document::AttrType::reader>().empty();
            result.setActive(hasReader && Transport::Tools::canMovePlayheadForward(transportAcsr, timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>()));
            break;
        }
        case CommandIDs::transportOscConnected:
        {
            result.setInfo(juce::translate("OSC connected"), juce::translate("State of the OSC connection"), "Transport", 0);
            result.setTicked(Instance::get().getOscSender().isConnected());
            result.setActive(true);
            break;
        }

        case CommandIDs::viewTimeZoomIn:
        {
            auto const& zoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
            result.setInfo(juce::translate("Time Zoom In"), juce::translate("Zooms in on the time range"), "View", 0);
            result.defaultKeypresses.add(juce::KeyPress('+', juce::ModifierKeys::commandModifier, 0));
            result.setActive(Zoom::Tools::canZoomIn(zoomAcsr));
            break;
        }
        case CommandIDs::viewTimeZoomOut:
        {
            auto const& zoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
            result.setInfo(juce::translate("Time Zoom Out"), juce::translate("Zooms out on the time range"), "View", 0);
            result.defaultKeypresses.add(juce::KeyPress('-', juce::ModifierKeys::commandModifier, 0));
            result.setActive(Zoom::Tools::canZoomOut(zoomAcsr));
            break;
        }
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
            break;
        }
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
            break;
        }
        case CommandIDs::viewTimeZoomAnchorOnPlayhead:
        {
            result.setInfo(juce::translate("Anchor Time Zoom on Playhead"), juce::translate("Anchors the time zoom on the start playhead"), "View", 0);
            result.setActive(true);
            result.setTicked(Instance::get().getApplicationAccessor().getAttr<AttrType::timeZoomAnchorOnPlayhead>());
            break;
        }
        case CommandIDs::viewInfoBubble:
        {
            result.setInfo(juce::translate("Info Bubble"), juce::translate("Toggles display of the information bubble"), "View", 0);
            result.defaultKeypresses.add(juce::KeyPress('i', juce::ModifierKeys::commandModifier, 0));
            result.setActive(true);
            result.setTicked(Instance::get().getApplicationAccessor().getAttr<AttrType::showInfoBubble>());
            break;
        }
        case CommandIDs::viewShowItemProperties:
        {
            auto const groups = Document::Selection::getGroups(documentAcsr);
            auto const tracks = Document::Selection::getTracks(documentAcsr);
            if(!groups.empty() && tracks.empty())
            {
                result.setInfo(juce::translate("Show group(s) properties"), juce::translate("Shows the group(s) properties window"), "View", 0);
            }
            else if(groups.empty() && !tracks.empty())
            {
                result.setInfo(juce::translate("Show track(s) properties"), juce::translate("Shows the track(s) properties window"), "View", 0);
            }
            else
            {
                result.setInfo(juce::translate("Show group(s) and track(s) properties"), juce::translate("Shows the group(s) and track(s) properties window"), "View", 0);
            }
            result.defaultKeypresses.add(juce::KeyPress('p', juce::ModifierKeys::commandModifier | juce::ModifierKeys::altModifier, 0));
            result.setActive(!groups.empty() || !tracks.empty());
            break;
        }

        case CommandIDs::helpOpenAudioSettings:
        {
#if JUCE_MAC
            result.setInfo(juce::translate("Audio Settings..."), juce::translate("Shows the audio settings panel"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress(',', juce::ModifierKeys::commandModifier, 0));
#else
            result.setInfo(juce::translate("Audio Settings..."), juce::translate("Shows the audio settings panel"), "Help", 0);
            result.defaultKeypresses.add(juce::KeyPress('p', juce::ModifierKeys::commandModifier, 0));
#endif
            break;
        }
        case CommandIDs::helpOpenOscSettings:
        {
#if JUCE_MAC
            result.setInfo(juce::translate("OSC Settings..."), juce::translate("Shows the OSC settings panel"), "Application", 0);
#else
            result.setInfo(juce::translate("OSC Settings..."), juce::translate("Shows the OSC settings panel"), "Help", 0);
#endif
            break;
        }
        case CommandIDs::helpOpenPluginSettings:
        {
#if JUCE_MAC
            result.setInfo(juce::translate("Plugin Settings..."), juce::translate("Shows the plugin settings panel"), "Application", 0);
#else
            result.setInfo(juce::translate("Plugin Settings..."), juce::translate("Shows the plugin settings panel"), "Help", 0);
#endif
            break;
        }
        case CommandIDs::helpOpenAbout:
        {
#if JUCE_MAC
            result.setInfo(juce::translate("About Partiels"), juce::translate("Shows the information about the application"), "Application", 0);
#else
            result.setInfo(juce::translate("About Partiels"), juce::translate("Shows the information about the application"), "help", 0);
#endif
            break;
        }
        case CommandIDs::helpOpenProjectPage:
        {
            result.setInfo(juce::translate("Proceed to Project Page"), juce::translate("Opens the project page of the Ircam Forum in a web browser"), "Help", 0);
            result.setActive(true);
            break;
        }
        case CommandIDs::helpSdifConverter:
        {
            result.setInfo(juce::translate("SDIF Converter..."), juce::translate("Shows the SDIF converter panel"), "Help", 0);
            result.setActive(true);
            break;
        }
        case CommandIDs::helpAutoUpdate:
        {
            result.setInfo(juce::translate("Automatic Check for New Version"), juce::translate("Toggles the automatic check for a new version"), "Help", 0);
            result.setActive(true);
            result.setTicked(Instance::get().getApplicationAccessor().getAttr<AttrType::autoUpdate>());
            break;
        }
        case CommandIDs::helpCheckForUpdate:
        {
            result.setInfo(juce::translate("Check for New Version"), juce::translate("Checks for a new version"), "Help", 0);
            result.setActive(true);
            break;
        }
        case CommandIDs::helpOpenKeyMappings:
        {
            result.setInfo(juce::translate("Key Mappings..."), juce::translate("Shows the key mappings panel"), "Application", 0);
            result.setActive(true);
            break;
        }
    }

    Instance::get().getCommandInfo(commandID, result);
}

bool Application::CommandTarget::perform(juce::ApplicationCommandTarget::InvocationInfo const& info)
{
    auto const getCommandDescription = [&]()
    {
        juce::ApplicationCommandInfo result(info.commandID);
        getCommandInfo(info.commandID, result);
        return result.description;
    };

    auto const getTransportAcsr = []() -> Transport::Accessor&
    {
        return Instance::get().getDocumentAccessor().getAcsr<Document::AcsrType::transport>();
    };

    auto const performForAllTracks = [](std::function<void(Track::Accessor&, std::set<size_t>)> fn)
    {
        for(auto const& trackAcsr : Instance::get().getDocumentAccessor().getAcsrs<Document::AcsrType::tracks>())
        {
            if(Track::Tools::getFrameType(trackAcsr.get()) != Track::FrameType::vector)
            {
                fn(trackAcsr.get(), Document::Tools::getEffectiveSelectedChannelsForTrack(Instance::get().getDocumentAccessor(), trackAcsr.get()));
            }
        }
    };

    auto& undoManager = Instance::get().getUndoManager();
    auto& fileBased = Instance::get().getDocumentFileBased();
    auto& documentAcsr = Instance::get().getDocumentAccessor();
    auto& documentDir = Instance::get().getDocumentDirector();
    auto& transportAcsr = documentAcsr.getAcsr<Document::AcsrType::transport>();
    auto& timeZoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();

    auto const playhead = transportAcsr.getAttr<Transport::AttrType::startPlayhead>();
    auto const selection = transportAcsr.getAttr<Transport::AttrType::selection>();
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
            fileBased.saveAsync(true, true, nullptr);
            return true;
        }
        case CommandIDs::documentDuplicate:
        {
            fileBased.saveAsInteractiveAsync(true, nullptr);
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
            if(auto* window = Instance::get().getWindow())
            {
                window->getInterface().showExporterPanel();
            }
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
            if(auto* window = Instance::get().getWindow())
            {
                window->getInterface().showBatcherPanel();
            }
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
            documentDir.startAction();

            auto const position = Tools::getNewGroupPosition();
            auto const identifier = documentDir.addGroup(position, NotificationType::synchronous);
            if(identifier.has_value())
            {
                auto const references = documentDir.sanitize(NotificationType::synchronous);
                auto const groupIdentifier = references.count(identifier.value()) > 0_z ? references.at(identifier.value()) : identifier.value();
                documentDir.endAction(ActionState::newTransaction, juce::translate("New Group"));
                Document::Selection::selectItem(documentAcsr, {groupIdentifier, {}}, true, false, NotificationType::synchronous);
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
            if(auto* window = Instance::get().getWindow())
            {
                window->getInterface().togglePluginListTablePanel();
            }
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
                juce::AlertWindow::showAsync(options, [=, &documentDir](int windowResult)
                                             {
                                                 if(windowResult != 1)
                                                 {
                                                     return;
                                                 }
                                                 documentDir.startAction();
                                                 for(auto const& groupId : std::get<0_z>(selectedItems))
                                                 {
                                                     documentDir.removeGroup(groupId, NotificationType::synchronous);
                                                 }
                                                 [[maybe_unused]] auto const references = documentDir.sanitize(NotificationType::synchronous);
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
                juce::AlertWindow::showAsync(options, [=, &documentDir](int windowResult)
                                             {
                                                 if(windowResult != 1)
                                                 {
                                                     return;
                                                 }
                                                 documentDir.startAction();
                                                 for(auto const& trackId : std::get<1_z>(selectedItems))
                                                 {
                                                     documentDir.removeTrack(trackId, NotificationType::synchronous);
                                                 }
                                                 [[maybe_unused]] auto const references = documentDir.sanitize(NotificationType::synchronous);
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
                juce::AlertWindow::showAsync(options, [=, &documentDir](int windowResult)
                                             {
                                                 if(windowResult != 1)
                                                 {
                                                     return;
                                                 }
                                                 documentDir.startAction();
                                                 for(auto const& groupId : std::get<0_z>(selectedItems))
                                                 {
                                                     documentDir.removeGroup(groupId, NotificationType::synchronous);
                                                 }
                                                 for(auto const& trackId : std::get<1_z>(selectedItems))
                                                 {
                                                     documentDir.removeTrack(trackId, NotificationType::synchronous);
                                                 }
                                                 [[maybe_unused]] auto const references = documentDir.sanitize(NotificationType::synchronous);
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
            mFileChooser->launchAsync(Flags::openMode | Flags::canSelectFiles, [=, &documentDir](juce::FileChooser const& fileChooser)
                                      {
                                          auto const results = fileChooser.getResults();
                                          if(results.isEmpty())
                                          {
                                              return;
                                          }

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

        case CommandIDs::frameSelectAll:
        {
            transportAcsr.setAttr<Transport::AttrType::selection>(timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::frameDelete:
        {
            undoManager.beginNewTransaction(juce::translate("Delete Frame(s)"));
            performForAllTracks([&](Track::Accessor& trackAcsr, std::set<size_t> selectedChannels)
                                {
                                    auto const trackId = trackAcsr.getAttr<Track::AttrType::identifier>();
                                    auto const fn = documentDir.getSafeTrackAccessorFn(trackId);
                                    for(auto const& index : selectedChannels)
                                    {
                                        undoManager.perform(std::make_unique<Track::Result::Modifier::ActionErase>(fn, index, selection).release());
                                    }
                                });
            undoManager.perform(std::make_unique<Document::FocusRestorer>(documentAcsr).release());
            undoManager.perform(std::make_unique<Transport::Action::Restorer>(getTransportAcsr, playhead, selection).release());
            return true;
        }
        case CommandIDs::frameCopy:
        {
            mClipboard = Document::Tools::Clipboard();
            performForAllTracks([&](Track::Accessor& trackAcsr, std::set<size_t> selectedChannels)
                                {
                                    auto const trackId = trackAcsr.getAttr<Track::AttrType::identifier>();
                                    auto& trackData = mClipboard.data[trackId];
                                    for(auto const& index : selectedChannels)
                                    {
                                        trackData[index] = Track::Result::Modifier::copyFrames(trackAcsr, index, selection);
                                    }
                                });
            return true;
        }
        case CommandIDs::frameCut:
        {
            perform({CommandIDs::frameCopy});
            perform({CommandIDs::frameDelete});
            undoManager.setCurrentTransactionName(juce::translate("Cut Frame(s)"));
            return true;
        }
        case CommandIDs::framePaste:
        {
            undoManager.beginNewTransaction(juce::translate("Paste Frame(s)"));
            performForAllTracks([&](Track::Accessor& trackAcsr, [[maybe_unused]] std::set<size_t> selectedChannels)
                                {
                                    auto const trackId = trackAcsr.getAttr<Track::AttrType::identifier>();
                                    auto const trackIt = mClipboard.data.find(trackId);
                                    if(trackIt == mClipboard.data.cend())
                                    {
                                        return;
                                    }
                                    auto const fn = documentDir.getSafeTrackAccessorFn(trackId);
                                    auto const trackData = trackIt->second;
                                    for(auto const& data : trackData)
                                    {
                                        undoManager.perform(std::make_unique<Track::Result::Modifier::ActionPaste>(fn, data.first, data.second, playhead).release());
                                    }
                                });
            undoManager.perform(std::make_unique<Document::FocusRestorer>(documentAcsr).release());
            undoManager.perform(std::make_unique<Transport::Action::Restorer>(getTransportAcsr, playhead, selection.movedToStartAt(playhead)).release());
            return true;
        }
        case CommandIDs::frameDuplicate:
        {
            perform({CommandIDs::frameCopy});
            transportAcsr.setAttr<Transport::AttrType::startPlayhead>(mClipboard.range.getEnd(), NotificationType::synchronous);
            perform({CommandIDs::framePaste});
            undoManager.setCurrentTransactionName(juce::translate("Duplicate Frame(s)"));
            return true;
        }
        case CommandIDs::frameInsert:
        {
            auto const insertFrame = [&](Track::Accessor& trackAcsr, size_t const channel, double const time)
            {
                if(!Track::Result::Modifier::matchFrame(trackAcsr, channel, time))
                {
                    auto const trackId = trackAcsr.getAttr<Track::AttrType::identifier>();
                    auto const fn = documentDir.getSafeTrackAccessorFn(trackId);
                    auto const data = Track::Result::Modifier::createFrame(trackAcsr, channel, time);
                    undoManager.perform(std::make_unique<Track::Result::Modifier::ActionPaste>(fn, channel, data, time).release());
                }
            };
            auto const isPlaying = transportAcsr.getAttr<Transport::AttrType::playback>();
            auto const runningPlayhead = transportAcsr.getAttr<Transport::AttrType::runningPlayhead>();
            undoManager.beginNewTransaction(juce::translate("Insert Frame(s)"));
            performForAllTracks([&](Track::Accessor& trackAcsr, std::set<size_t> selectedChannels)
                                {
                                    for(auto const& channel : selectedChannels)
                                    {
                                        if(isPlaying)
                                        {
                                            insertFrame(trackAcsr, channel, runningPlayhead);
                                        }
                                        else
                                        {
                                            insertFrame(trackAcsr, channel, selection.getStart());
                                            if(!selection.isEmpty())
                                            {
                                                insertFrame(trackAcsr, channel, selection.getEnd());
                                            }
                                        }
                                    }
                                });
            undoManager.perform(std::make_unique<Document::FocusRestorer>(documentAcsr).release());
            undoManager.perform(std::make_unique<Transport::Action::Restorer>(getTransportAcsr, playhead, selection.movedToStartAt(playhead)).release());
            return true;
        }
        case CommandIDs::frameBreak:
        {
            auto const breakFrame = [&](Track::Accessor& trackAcsr, size_t const channel, double const time)
            {
                if(Track::Result::Modifier::canBreak(trackAcsr, channel, time))
                {
                    auto const trackId = trackAcsr.getAttr<Track::AttrType::identifier>();
                    auto const fn = documentDir.getSafeTrackAccessorFn(trackId);
                    Track::Result::ChannelData const data = std::vector<Track::Result::Data::Point>{{time, 0.0, std::optional<float>{}, std::vector<float>{}}};
                    undoManager.perform(std::make_unique<Track::Result::Modifier::ActionPaste>(fn, channel, data, time).release());
                }
            };
            auto const isPlaying = transportAcsr.getAttr<Transport::AttrType::playback>();
            auto const runningPlayhead = transportAcsr.getAttr<Transport::AttrType::runningPlayhead>();
            undoManager.beginNewTransaction(juce::translate("Break Frame(s)"));
            performForAllTracks([&](Track::Accessor& trackAcsr, std::set<size_t> selectedChannels)
                                {
                                    if(Track::Tools::getFrameType(trackAcsr) == Track::FrameType::value)
                                    {
                                        for(auto const& channel : selectedChannels)
                                        {
                                            if(isPlaying)
                                            {
                                                breakFrame(trackAcsr, channel, runningPlayhead);
                                            }
                                            else
                                            {
                                                breakFrame(trackAcsr, channel, selection.getStart());
                                            }
                                        }
                                    }
                                });
            undoManager.perform(std::make_unique<Document::FocusRestorer>(documentAcsr).release());
            undoManager.perform(std::make_unique<Transport::Action::Restorer>(getTransportAcsr, playhead, selection.movedToStartAt(playhead)).release());
            return true;
        }
        case CommandIDs::frameResetDurationToZero:
        {
            undoManager.beginNewTransaction(juce::translate("Reset Frame(s) duration to zero"));
            performForAllTracks([&](Track::Accessor& trackAcsr, std::set<size_t> selectedChannels)
                                {
                                    auto const trackId = trackAcsr.getAttr<Track::AttrType::identifier>();
                                    auto const fn = documentDir.getSafeTrackAccessorFn(trackId);
                                    for(auto const& channel : selectedChannels)
                                    {
                                        undoManager.perform(std::make_unique<Track::Result::Modifier::ActionResetDuration>(fn, channel, selection, Track::Result::Modifier::DurationResetMode::toZero, 0.0).release());
                                    }
                                });
            undoManager.perform(std::make_unique<Document::FocusRestorer>(documentAcsr).release());
            undoManager.perform(std::make_unique<Transport::Action::Restorer>(getTransportAcsr, playhead, selection).release());
            return true;
        }
        case CommandIDs::frameResetDurationToFull:
        {
            undoManager.beginNewTransaction(juce::translate("Reset Frame(s) duration to full"));
            auto const timeEnd = timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>().getEnd();
            performForAllTracks([&](Track::Accessor& trackAcsr, std::set<size_t> selectedChannels)
                                {
                                    auto const trackId = trackAcsr.getAttr<Track::AttrType::identifier>();
                                    auto const fn = documentDir.getSafeTrackAccessorFn(trackId);
                                    for(auto const& channel : selectedChannels)
                                    {
                                        undoManager.perform(std::make_unique<Track::Result::Modifier::ActionResetDuration>(fn, channel, selection, Track::Result::Modifier::DurationResetMode::toFull, timeEnd).release());
                                    }
                                });
            undoManager.perform(std::make_unique<Document::FocusRestorer>(documentAcsr).release());
            undoManager.perform(std::make_unique<Transport::Action::Restorer>(getTransportAcsr, playhead, selection).release());
            return true;
        }
        case CommandIDs::frameSystemCopy:
        {
            auto const& trackAcsrs = documentAcsr.getAcsrs<Document::AcsrType::tracks>();
            for(auto const& trackAcsr : trackAcsrs)
            {
                if(Track::Tools::getFrameType(trackAcsr.get()) != Track::FrameType::vector)
                {
                    std::atomic<bool> shouldAbort{false};
                    auto const selectedChannels = Document::Tools::getEffectiveSelectedChannelsForTrack(documentAcsr, trackAcsr);
                    if(!selectedChannels.empty())
                    {
                        juce::String clipboardResults;
                        Track::Exporter::toJson(trackAcsr.get(), selection, selectedChannels, clipboardResults, false, shouldAbort);
                        juce::SystemClipboard::copyTextToClipboard(clipboardResults);
                        return true;
                    }
                }
            }
            return true;
        }
        case CommandIDs::frameToggleDrawing:
        {
            documentAcsr.setAttr<Document::AttrType::drawingState>(!documentAcsr.getAttr<Document::AttrType::drawingState>(), NotificationType::synchronous);
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
                juce::AlertWindow::showAsync(options, [](int result)
                                             {
                                                 if(result == 1)
                                                 {
                                                     if(auto* window = Instance::get().getWindow())
                                                     {
                                                         window->getInterface().showAudioSettingsPanel();
                                                     }
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
            Transport::Tools::movePlayheadBackward(transportAcsr, timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::transportMovePlayHeadForward:
        {
            Transport::Tools::movePlayheadForward(transportAcsr, timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::transportOscConnected:
        {
            return true;
        }

        case CommandIDs::viewTimeZoomIn:
        {
            auto const& accessor = Instance::get().getApplicationAccessor();
            if(accessor.getAttr<AttrType::timeZoomAnchorOnPlayhead>())
            {
                Zoom::Tools::zoomIn(documentAcsr.getAcsr<Document::AcsrType::timeZoom>(), 0.05, playhead, NotificationType::synchronous);
            }
            else
            {
                Zoom::Tools::zoomIn(documentAcsr.getAcsr<Document::AcsrType::timeZoom>(), 0.01, NotificationType::synchronous);
            }
            return true;
        }
        case CommandIDs::viewTimeZoomOut:
        {
            auto const& accessor = Instance::get().getApplicationAccessor();
            if(accessor.getAttr<AttrType::timeZoomAnchorOnPlayhead>())
            {
                Zoom::Tools::zoomOut(documentAcsr.getAcsr<Document::AcsrType::timeZoom>(), 0.05, playhead, NotificationType::synchronous);
            }
            else
            {
                Zoom::Tools::zoomOut(documentAcsr.getAcsr<Document::AcsrType::timeZoom>(), 0.01, NotificationType::synchronous);
            }
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
        case CommandIDs::viewTimeZoomAnchorOnPlayhead:
        {
            auto& accessor = Instance::get().getApplicationAccessor();
            accessor.setAttr<AttrType::timeZoomAnchorOnPlayhead>(!accessor.getAttr<AttrType::timeZoomAnchorOnPlayhead>(), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::viewInfoBubble:
        {
            auto& accessor = Instance::get().getApplicationAccessor();
            accessor.setAttr<AttrType::showInfoBubble>(!accessor.getAttr<AttrType::showInfoBubble>(), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::viewShowItemProperties:
        {
            static auto constexpr offset = juce::Point<int>(20, 20);
            auto* window = Instance::get().getWindow();
            if(window != nullptr)
            {
                auto position = window->getScreenPosition();
                position += offset;
                for(auto& groupId : Document::Selection::getGroups(documentAcsr))
                {
                    if(auto var = std::make_unique<juce::DynamicObject>())
                    {
                        var->setProperty("x", position.x);
                        var->setProperty("y", position.y);
                        Document::Tools::getGroupAcsr(documentAcsr, groupId).sendSignal(Group::SignalType::showProperties, var.release(), NotificationType::synchronous);
                    }
                    position += offset;
                }
                for(auto& trackId : Document::Selection::getTracks(documentAcsr))
                {
                    if(auto var = std::make_unique<juce::DynamicObject>())
                    {
                        var->setProperty("x", position.x);
                        var->setProperty("y", position.y);
                        Document::Tools::getTrackAcsr(documentAcsr, trackId).sendSignal(Track::SignalType::showProperties, var.release(), NotificationType::synchronous);
                    }
                    position += offset;
                }
            }
            return true;
        }

        case CommandIDs::helpOpenAudioSettings:
        {
            if(auto* window = Instance::get().getWindow())
            {
                window->getInterface().showAudioSettingsPanel();
            }
            return true;
        }
        case CommandIDs::helpOpenOscSettings:
        {
            if(auto* window = Instance::get().getWindow())
            {
                window->getInterface().showOscSettingsPanel();
            }
            return true;
        }
        case CommandIDs::helpOpenPluginSettings:
        {
            if(auto* window = Instance::get().getWindow())
            {
                window->getInterface().showPluginSearchPathPanel();
            }
            return true;
        }
        case CommandIDs::helpOpenAbout:
        {
            if(auto* window = Instance::get().getWindow())
            {
                window->getInterface().showAboutPanel();
            }
            return true;
        }
        case CommandIDs::helpOpenProjectPage:
        {
            juce::URL const url("https://github.com/Ircam-Partiels/Partiels");
            if(url.isWellFormed())
            {
                url.launchInDefaultBrowser();
            }
            return true;
        }
        case CommandIDs::helpSdifConverter:
        {
            if(auto* window = Instance::get().getWindow())
            {
                window->getInterface().showConverterPanel();
            }
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
        case CommandIDs::helpOpenKeyMappings:
        {
            if(auto* window = Instance::get().getWindow())
            {
                window->getInterface().showKeyMappingsPanel();
            }
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
    if(source == &Instance::get().getOscSender())
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

void Application::CommandTarget::selectDefaultTemplateFile()
{
    auto const wildcard = Instance::getWildCardForDocumentFile();
    auto const file = Instance::get().getDocumentFileBased().getFile();
    mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Select a template..."), file, wildcard);
    if(mFileChooser == nullptr)
    {
        return;
    }
    using Flags = juce::FileBrowserComponent::FileChooserFlags;
    mFileChooser->launchAsync(Flags::openMode | Flags::canSelectFiles, [](juce::FileChooser const& fileChooser)
                              {
                                  auto const results = fileChooser.getResults();
                                  if(results.isEmpty())
                                  {
                                      return;
                                  }
                                  auto& appAccessor = Instance::get().getApplicationAccessor();
                                  appAccessor.setAttr<AttrType::defaultTemplateFile>(results.getFirst(), NotificationType::synchronous);
                              });
}

ANALYSE_FILE_END
