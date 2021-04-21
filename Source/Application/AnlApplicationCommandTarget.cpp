#include "AnlApplicationCommandTarget.h"
#include "../Document/AnlDocumentTools.h"
#include "AnlApplicationInstance.h"
#include <ImagesData.h>

ANALYSE_FILE_BEGIN

void Application::CommandTarget::showUnsupportedAction()
{
    juce::ImageComponent imageComponent;
    auto const image = juce::ImageCache::getFromMemory(ImagesData::MagicWord_jpeg, ImagesData::MagicWord_jpegSize);
    imageComponent.setImage(image);
    imageComponent.setImagePlacement(juce::RectanglePlacement::fillDestination);
    imageComponent.setSize(image.getWidth() / 2, image.getHeight() / 2);

    juce::TextButton button("OK");
    button.setBounds({480, 270, 80, 32});
    imageComponent.addAndMakeVisible(button);

    auto const& laf = juce::Desktop::getInstance().getDefaultLookAndFeel();
    auto const bgColor = laf.findColour(juce::ResizableWindow::backgroundColourId);

    juce::DialogWindow::LaunchOptions o;
    o.dialogTitle = juce::translate("Action Unavailable!");
    o.content.setNonOwned(&imageComponent);
    o.componentToCentreAround = nullptr;
    o.dialogBackgroundColour = bgColor;
    o.escapeKeyTriggersCloseButton = true;
    o.useNativeTitleBar = false;
    o.resizable = false;
    o.useBottomRightCornerResizer = false;
    auto* window = o.launchAsync();
    if(window != nullptr)
    {
        button.onClick = [&]()
        {
            window->exitModalState(0);
        };
        window->runModalLoop();
    }
}

Application::CommandTarget::CommandTarget()
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
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
    Instance::get().getUndoManager().addChangeListener(this);
    Instance::get().getApplicationAccessor().addListener(mListener, NotificationType::synchronous);
    Instance::get().getApplicationCommandManager().registerAllCommandsForTarget(this);
}

Application::CommandTarget::~CommandTarget()
{
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
        , CommandIDs::DocumentOpenTemplate
        , CommandIDs::DocumentSaveTemplate
        
        , CommandIDs::EditUndo
        , CommandIDs::EditRedo
        , CommandIDs::EditNewGroup
        , CommandIDs::EditNewTrack
        
        , CommandIDs::TransportTogglePlayback
        , CommandIDs::TransportToggleLooping
        , CommandIDs::TransportRewindPlayHead
        
        , CommandIDs::ZoomIn
        , CommandIDs::ZoomOut
        
        , CommandIDs::HelpOpenAudioSettings
        , CommandIDs::HelpOpenAbout
        , CommandIDs::HelpOpenManual
        , CommandIDs::HelpOpenForum
    });
    // clang-format on
}

void Application::CommandTarget::getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result)
{
    auto const& docAcsr = Instance::get().getDocumentAccessor();
    auto const& transportAcsr = docAcsr.getAcsr<Document::AcsrType::transport>();
    switch(commandID)
    {
        case CommandIDs::DocumentNew:
        {
            result.setInfo(juce::translate("New..."), juce::translate("Create a new document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('n', juce::ModifierKeys::commandModifier, 0));
            result.setActive(!docAcsr.isEquivalentTo(Document::FileBased::getDefaultContainer()));
        }
        break;
        case CommandIDs::DocumentOpen:
        {
            result.setInfo(juce::translate("Open..."), juce::translate("Open a document or an audio file"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('o', juce::ModifierKeys::commandModifier, 0));
            result.setActive(true);
        }
        break;
        case CommandIDs::DocumentOpenRecent:
        {
            auto const& appAcsr = Instance::get().getApplicationAccessor();
            result.setInfo(juce::translate("Open Recent"), juce::translate("Open a recent document"), "Application", 0);
            result.setActive(!appAcsr.getAttr<AttrType::recentlyOpenedFilesList>().empty());
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
            result.setActive(docAcsr.getAttr<Document::AttrType::file>() != juce::File());
        }
        break;
        case CommandIDs::DocumentOpenTemplate:
        {
            result.setInfo(juce::translate("Open Template..."), juce::translate("Open a template"), "Application", 0);
            result.setActive(docAcsr.getAttr<Document::AttrType::file>() != juce::File());
        }
        break;
        case CommandIDs::DocumentSaveTemplate:
        {
            result.setInfo(juce::translate("Save Template..."), juce::translate("Save as a template"), "Application", 0);
            result.setActive(!docAcsr.getAcsrs<Document::AcsrType::tracks>().empty());
        }
        break;

        case CommandIDs::EditUndo:
        {
            auto& undoManager = Instance::get().getUndoManager();
            result.setInfo(juce::translate("Undo Action"), juce::translate(""), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0));
            result.setActive(undoManager.canUndo());
        }
        break;
        case CommandIDs::EditRedo:
        {
            auto& undoManager = Instance::get().getUndoManager();
            result.setInfo(juce::translate("Redo Action"), juce::translate(""), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('z', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(undoManager.canRedo());
        }
        break;

        case CommandIDs::EditNewGroup:
        {
            result.setInfo(juce::translate("Add New Group"), juce::translate("Adds a new group"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('g', juce::ModifierKeys::commandModifier, 0));
            result.setActive(docAcsr.getAttr<Document::AttrType::file>() != juce::File());
        }
        break;
        case CommandIDs::EditNewTrack:
        {
            result.setInfo(juce::translate("Add New Track"), juce::translate("Adds a new track"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('t', juce::ModifierKeys::commandModifier, 0));
            result.setActive(docAcsr.getAttr<Document::AttrType::file>() != juce::File());
        }
        break;

        case CommandIDs::TransportTogglePlayback:
        {
            result.setInfo(juce::translate("Toggle Playback"), juce::translate("Start or stop the audio playback"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::spaceKey, juce::ModifierKeys::noModifiers, 0));
            result.setActive(docAcsr.getAttr<Document::AttrType::file>() != juce::File());
            result.setTicked(transportAcsr.getAttr<Transport::AttrType::playback>());
        }
        break;
        case CommandIDs::TransportToggleLooping:
        {
            result.setInfo(juce::translate("Toggle Loop"), juce::translate("Enable or disable the loop audio playback"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress('l', juce::ModifierKeys::commandModifier, 0));
            result.setActive(docAcsr.getAttr<Document::AttrType::file>() != juce::File());
            result.setTicked(transportAcsr.getAttr<Transport::AttrType::looping>());
        }
        break;
        case CommandIDs::TransportRewindPlayHead:
        {
            result.setInfo(juce::translate("Rewind Playhead"), juce::translate("Move the playhead to the start of the document"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress('w', juce::ModifierKeys::commandModifier, 0));
            result.setActive(docAcsr.getAttr<Document::AttrType::file>() != juce::File() && transportAcsr.getAttr<Transport::AttrType::runningPlayhead>() > 0.0);
        }
        break;

        case CommandIDs::ZoomIn:
        {
            auto const& zoomAcsr = docAcsr.getAcsr<Document::AcsrType::timeZoom>();
            result.setInfo(juce::translate("Zoom In"), juce::translate("Opens the manual in a web browser"), "Zoom", 0);
            result.defaultKeypresses.add(juce::KeyPress('+', juce::ModifierKeys::commandModifier, 0));
            result.setActive(zoomAcsr.getAttr<Zoom::AttrType::visibleRange>().getLength() > zoomAcsr.getAttr<Zoom::AttrType::minimumLength>());
        }
        break;
        case CommandIDs::ZoomOut:
        {
            auto const& zoomAcsr = docAcsr.getAcsr<Document::AcsrType::timeZoom>();
            result.setInfo(juce::translate("Zoom Out"), juce::translate("Opens the manual in a web browser"), "Zoom", 0);
            result.defaultKeypresses.add(juce::KeyPress('-', juce::ModifierKeys::commandModifier, 0));
            result.setActive(zoomAcsr.getAttr<Zoom::AttrType::visibleRange>().getLength() < zoomAcsr.getAttr<Zoom::AttrType::globalRange>().getLength());
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
    auto& docAcsr = Instance::get().getDocumentAccessor();
    auto& transportAcsr = docAcsr.getAcsr<Document::AcsrType::transport>();
    switch(info.commandID)
    {
        case CommandIDs::DocumentNew:
        {
            if(fileBased.saveIfNeededAndUserAgrees() != juce::FileBasedDocument::SaveResult::savedOk)
            {
                return true;
            }
            Instance::get().openFile({});
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
            if(!fc.browseForFileToOpen())
            {
                return true;
            }
            Instance::get().openFile(fc.getResult());
            return true;
        }
        case CommandIDs::DocumentOpenRecent:
        {
            // Managed
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
        case CommandIDs::DocumentOpenTemplate:
        case CommandIDs::DocumentSaveTemplate:
        {
            showUnsupportedAction();
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
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            documentDir.startAction();

            auto const index = documentAcsr.getNumAcsr<Document::AcsrType::groups>();
            auto const focusedId = Document::Tools::getFocusedGroup(documentAcsr);
            auto const position = focusedId.has_value() ? Document::Tools::getGroupPosition(documentAcsr, *focusedId) : index;
            if(!documentDir.addGroup("Group " + juce::String(index + 1_z), position, NotificationType::synchronous))
            {
                auto constexpr icon = juce::AlertWindow::AlertIconType::WarningIcon;
                auto const title = juce::translate("Group cannot be created!");
                auto const message = juce::translate("The group cannot be inserted into the document.");
                juce::AlertWindow::showMessageBox(icon, title, message);
                documentDir.endAction("New Group", ActionState::abort);
            }
            else
            {
                documentDir.endAction("New Group " + juce::String(index + 1_z), ActionState::apply);
            }
            return true;
        }
        case CommandIDs::EditNewTrack:
        {
            auto& documentDir = Instance::get().getDocumentDirector();
            documentDir.startAction();
            documentDir.addTrack(AlertType::window, NotificationType::synchronous);
            documentDir.endAction("New Track", ActionState::apply);
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

        case CommandIDs::ZoomIn:
        {
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            auto& zoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
            auto const range = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            auto const grange = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range.expanded(grange.getLength() / -100.0), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::ZoomOut:
        {
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            auto& zoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
            auto const range = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            auto const grange = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range.expanded(grange.getLength() / 100.0), NotificationType::synchronous);
            return true;
        }

        case CommandIDs::HelpOpenAudioSettings:
        {
            Instance::get().getAudioSettings().show();
            return true;
        }
        case CommandIDs::HelpOpenAbout:
        {
            Instance::get().getAbout().show();
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
