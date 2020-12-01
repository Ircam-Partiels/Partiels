#include "AnlApplicationCommandTarget.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

void Application::CommandTarget::showUnsupportedAction()
{
    juce::ImageComponent imageComponent;
    auto const image = juce::ImageCache::getFromMemory(BinaryData::MagicWord_jpeg, BinaryData::MagicWord_jpegSize);
    imageComponent.setImage(image);
    imageComponent.setImagePlacement(juce::RectanglePlacement::fillDestination);
    imageComponent.setSize(image.getWidth() / 2, image.getHeight() / 2);
    
    juce::TextButton button("OK");
    button.setBounds({480, 270, 80, 32});
    imageComponent.addAndMakeVisible(button);
    
    auto const& lookAndFeel = juce::Desktop::getInstance().getDefaultLookAndFeel();
    auto const bgColor = lookAndFeel.findColour(juce::ResizableWindow::backgroundColourId);
    
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
        CommandIDs::DocumentOpen
        , CommandIDs::DocumentNew
        , CommandIDs::DocumentSave
        , CommandIDs::DocumentDuplicate
        , CommandIDs::DocumentConsolidate
        , CommandIDs::DocumentOpenTemplate
        , CommandIDs::DocumentSaveTemplate
        
        , CommandIDs::EditUndo
        , CommandIDs::EditRedo

        , CommandIDs::AnalysisOpen
        , CommandIDs::AnalysisNew
        , CommandIDs::AnalysisSave
        , CommandIDs::AnalysisDuplicate
        , CommandIDs::AnalysisRemove
        , CommandIDs::AnalysisProperties
        , CommandIDs::AnalysisExport
        
        , CommandIDs::PointsNew
        , CommandIDs::PointsRemove
        , CommandIDs::PointsMove
        , CommandIDs::PointsCopy
        , CommandIDs::PointsPaste
        , CommandIDs::PointsScale
        , CommandIDs::PointsQuantify
        , CommandIDs::PointsDiscretize
        
        , CommandIDs::TransportTogglePlayback
        , CommandIDs::TransportToggleLooping
        , CommandIDs::TransportRewindPlayHead
        
        , CommandIDs::HelpOpenManual
        , CommandIDs::HelpOpenForum
    });
}

void Application::CommandTarget::getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result)
{
    auto const& docAcsr = Instance::get().getDocumentAccessor();
    switch (commandID)
    {
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
        case CommandIDs::DocumentNew:
        {
            result.setInfo(juce::translate("New"), juce::translate("Create a new document"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('n', juce::ModifierKeys::commandModifier, 0));
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
            result.setActive(!docAcsr.getAccessors<Document::AttrType::analyzers>().empty());
        }
            break;
            
        case CommandIDs::EditUndo:
        {
            result.setInfo(juce::translate("Undo Action"), juce::translate(""), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0));
            result.setActive(false);
        }
            break;
        case CommandIDs::EditRedo:
        {
            result.setInfo(juce::translate("Redo Action"), juce::translate(""), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('z', juce::ModifierKeys::commandModifier + juce::ModifierKeys::shiftModifier, 0));
            result.setActive(false);
        }
            break;
        
        case CommandIDs::AnalysisOpen:
        {
            result.setInfo(juce::translate("Open Analysis Template"), juce::translate("Open a new analysis"), "Analysis", 0);
            result.setActive(docAcsr.getAttr<Document::AttrType::file>() != juce::File());
        }
            break;
        case CommandIDs::AnalysisNew:
        {
            result.setInfo(juce::translate("Add New Analysis"), juce::translate("Adds a new analysis"), "Analysis", 0);
            result.defaultKeypresses.add(juce::KeyPress('t', juce::ModifierKeys::commandModifier, 0));
            result.setActive(docAcsr.getAttr<Document::AttrType::file>() != juce::File());
        }
            break;
        case CommandIDs::AnalysisSave:
        {
            result.setInfo(juce::translate("Save Analysis Template"), juce::translate("Save the analysis"), "Analysis", 0);
            result.setActive(docAcsr.getAttr<Document::AttrType::file>() != juce::File());
        }
            break;
        case CommandIDs::AnalysisDuplicate:
        {
            result.setInfo(juce::translate("Duplicate Analysis"), juce::translate("Duplicates the analysis of the document"), "Analysis", 0);
            result.setActive(docAcsr.getAttr<Document::AttrType::file>() != juce::File());
        }
            break;
        case CommandIDs::AnalysisRemove:
        {
            result.setInfo(juce::translate("Remove Analysis"), juce::translate("Removes the analysis from the document"), "Analysis", 0);
            result.defaultKeypresses.add(juce::KeyPress(0x08, juce::ModifierKeys::noModifiers, 0));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::backspaceKey, juce::ModifierKeys::noModifiers, 0));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::deleteKey, juce::ModifierKeys::noModifiers, 0));
            result.setActive(!docAcsr.getAccessors<Document::AttrType::analyzers>().empty());
        }
            break;
        case CommandIDs::AnalysisProperties:
        {
            result.setInfo(juce::translate("Show Analysis Properties"), juce::translate("Shows the analysis property window"), "Analysis", 0);
            result.setActive(docAcsr.getAttr<Document::AttrType::file>() != juce::File());
        }
            break;
        case CommandIDs::AnalysisExport:
        {
            result.setInfo(juce::translate("Exports Analysis"), juce::translate("Exports the analysis"), "Analysis", 0);
            result.setActive(docAcsr.getAttr<Document::AttrType::file>() != juce::File());
        }
            break;
            
        case CommandIDs::PointsNew:
        {
            result.setInfo(juce::translate("New Point(s) or Marker(s)"), juce::translate("Adds new points or markers to the analysis"), "Points", 0);
            result.setActive(true);
        }
            break;
        case CommandIDs::PointsRemove:
        {
            result.setInfo(juce::translate("Remove Point(s) or Marker(s)"), juce::translate("Removes points or markers from the analysis"), "Points", 0);
            result.setActive(true);
        }
            break;
        case CommandIDs::PointsMove:
        {
            result.setInfo(juce::translate("Move Point(s) or Marker(s)"), juce::translate("Moves points or markers of the analysis"), "Points", 0);
            result.setActive(true);
        }
            break;
        case CommandIDs::PointsCopy:
        {
            result.setInfo(juce::translate("Copy Point(s) or Marker(s)"), juce::translate("Copies points or markers of the analysis"), "Points", 0);
            result.setActive(true);
        }
            break;
        case CommandIDs::PointsPaste:
        {
            result.setInfo(juce::translate("Paste Point(s) or Marker(s)"), juce::translate("Pastes points or markers to the analysis"), "Points", 0);
            result.setActive(true);
        }
            break;
        case CommandIDs::PointsScale:
        {
            result.setInfo(juce::translate("Scale Point(s) or Marker(s)"), juce::translate("Scales points or markers of the analysis"), "Points", 0);
            result.setActive(true);
        }
            break;
        case CommandIDs::PointsQuantify:
        {
            result.setInfo(juce::translate("Quantify Point(s) or Marker(s)"), juce::translate("Quantifies points or markers of the analysis"), "Points", 0);
            result.setActive(true);
        }
            break;
        case CommandIDs::PointsDiscretize:
        {
            result.setInfo(juce::translate("Discretize Point(s) or Marker(s)"), juce::translate("Discretizes points or markers of the analysis"), "Points", 0);
            result.setActive(true);
        }
            break;
            
            
        case CommandIDs::TransportTogglePlayback:
        {
            result.setInfo(juce::translate("Toggle Playback"), juce::translate("Start or stop the audio playback"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::spaceKey, juce::ModifierKeys::noModifiers, 0));
            result.setActive(docAcsr.getAttr<Document::AttrType::file>() != juce::File());
            result.setTicked(docAcsr.getAttr<Document::AttrType::isPlaybackStarted>());
        }
            break;
        case CommandIDs::TransportToggleLooping:
        {
            result.setInfo(juce::translate("Toggle Loop"), juce::translate("Enable or disable the loop audio playback"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress('l', juce::ModifierKeys::commandModifier, 0));
            result.setActive(docAcsr.getAttr<Document::AttrType::file>() != juce::File());
            result.setTicked(docAcsr.getAttr<Document::AttrType::isLooping>());
        }
            break;
        case CommandIDs::TransportRewindPlayHead:
        {
            result.setInfo(juce::translate("Rewind Playhead"), juce::translate("Move the playhead to the start of the document"), "Transport", 0);
            result.defaultKeypresses.add(juce::KeyPress('w', juce::ModifierKeys::commandModifier, 0));
            result.setActive(docAcsr.getAttr<Document::AttrType::file>() != juce::File() && docAcsr.getAttr<Document::AttrType::playheadPosition>() > 0.0);
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
    switch (info.commandID)
    {
        case CommandIDs::DocumentOpen:
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
        case CommandIDs::DocumentOpenRecent:
        {
            // Managed 
            return true;
        }
        case CommandIDs::DocumentNew:
        {
            if(fileBased.saveIfNeededAndUserAgrees() != juce::FileBasedDocument::SaveResult::savedOk)
            {
                return true;
            }
            fileBased.setFile({});
            Instance::get().getDocumentAccessor().fromContainer({
                  {juce::File{}}
                , {false}
                , {1.0}
                , {false}
                , {0.0}
                , {}
                , {}
                , {66}
                , {}
            }, NotificationType::synchronous);
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
        case CommandIDs::EditRedo:
        {
            showUnsupportedAction();
            return true;
        }
            
            
        case CommandIDs::AnalysisOpen:
        {
            showUnsupportedAction();
            return true;
        }
        case CommandIDs::AnalysisNew:
        {
            auto& documentDir = Instance::get().getDocumentDirector();
            documentDir.addAnalysis(AlertType::window);
            return true;
        }
        case CommandIDs::AnalysisSave:
        case CommandIDs::AnalysisDuplicate:
        case CommandIDs::AnalysisRemove:
        case CommandIDs::AnalysisProperties:
        case CommandIDs::AnalysisExport:
        {
            showUnsupportedAction();
            return true;
        }
            
        case CommandIDs::PointsNew:
        case CommandIDs::PointsRemove:
        case CommandIDs::PointsMove:
        case CommandIDs::PointsCopy:
        case CommandIDs::PointsPaste:
        case CommandIDs::PointsScale:
        case CommandIDs::PointsQuantify:
        case CommandIDs::PointsDiscretize:
        {
            showUnsupportedAction();
            return true;
        }
            
        case CommandIDs::TransportTogglePlayback:
        {
            auto constexpr attr = Document::AttrType::isPlaybackStarted;
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            documentAcsr.setAttr<attr>(!documentAcsr.getAttr<attr>(), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::TransportToggleLooping:
        {
            auto constexpr attr = Document::AttrType::isLooping;
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            documentAcsr.setAttr<attr>(!documentAcsr.getAttr<attr>(), NotificationType::synchronous);
            return true;
        }
        case CommandIDs::TransportRewindPlayHead:
        {
            auto constexpr attr = Document::AttrType::playheadPosition;
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            auto const isPlaying = documentAcsr.getAttr<Document::AttrType::isPlaybackStarted>();
            if(isPlaying)
            {
                documentAcsr.setAttr<Document::AttrType::isPlaybackStarted>(false, NotificationType::synchronous);
            }
            documentAcsr.setAttr<attr>(0.0, NotificationType::synchronous);
            if(isPlaying)
            {
                documentAcsr.setAttr<Document::AttrType::isPlaybackStarted>(true, NotificationType::synchronous);
            }
            return true;
        }
            
        case CommandIDs::HelpOpenManual:
        case CommandIDs::HelpOpenForum:
        {
            showUnsupportedAction();
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

juce::StringArray Application::MainMenuModel::getMenuBarNames()
{
    return {"File", "Edit", "Transport", "Analysis", "Points", "Help"};
}

juce::PopupMenu Application::MainMenuModel::getMenuForIndex(int topLevelMenuIndex, juce::String const& menuName)
{
    juce::ignoreUnused(topLevelMenuIndex);
    
    using CommandIDs = CommandTarget::CommandIDs;
    auto& commandManager = Instance::get().getApplicationCommandManager();
    juce::PopupMenu menu;
    if(menuName == "File")
    {
        menu.addCommandItem(&commandManager, CommandIDs::DocumentNew);
        menu.addCommandItem(&commandManager, CommandIDs::DocumentOpen);
        juce::PopupMenu recentFilesMenu;
        auto recentFileIndex = static_cast<int>(CommandIDs::DocumentOpenRecent);
        auto const& recentFiles = Instance::get().getApplicationAccessor().getAttr<AttrType::recentlyOpenedFilesList>();
        for(auto const& file : recentFiles)
        {
            auto const isActive = Instance::get().getDocumentFileBased().getFile() != file;
            recentFilesMenu.addItem(recentFileIndex++, file.getFileNameWithoutExtension(), isActive);
        }
        menu.addSubMenu("Open Recent", recentFilesMenu);
        menu.addCommandItem(&commandManager, CommandIDs::DocumentSave);
        menu.addCommandItem(&commandManager, CommandIDs::DocumentDuplicate);
        menu.addCommandItem(&commandManager, CommandIDs::DocumentConsolidate);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::DocumentOpenTemplate);
        menu.addCommandItem(&commandManager, CommandIDs::DocumentSaveTemplate);
    }
    else if(menuName == "Edit")
    {
        menu.addCommandItem(&commandManager, CommandIDs::EditUndo);
        menu.addCommandItem(&commandManager, CommandIDs::EditRedo);
    }
    else if(menuName == "Analysis")
    {
        menu.addCommandItem(&commandManager, CommandIDs::AnalysisOpen);
        menu.addCommandItem(&commandManager, CommandIDs::AnalysisNew);
        menu.addCommandItem(&commandManager, CommandIDs::AnalysisSave);
        menu.addCommandItem(&commandManager, CommandIDs::AnalysisDuplicate);
        menu.addCommandItem(&commandManager, CommandIDs::AnalysisRemove);
        menu.addCommandItem(&commandManager, CommandIDs::AnalysisExport);
        menu.addCommandItem(&commandManager, CommandIDs::AnalysisProperties);
    }
    else if(menuName == "Points")
    {
        menu.addCommandItem(&commandManager, CommandIDs::PointsNew);
        menu.addCommandItem(&commandManager, CommandIDs::PointsRemove);
        menu.addCommandItem(&commandManager, CommandIDs::PointsMove);
        menu.addCommandItem(&commandManager, CommandIDs::PointsCopy);
        menu.addCommandItem(&commandManager, CommandIDs::PointsPaste);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::PointsScale);
        menu.addCommandItem(&commandManager, CommandIDs::PointsQuantify);
        menu.addCommandItem(&commandManager, CommandIDs::PointsDiscretize);
    }
    else if(menuName == "Transport")
    {
        menu.addCommandItem(&commandManager, CommandIDs::TransportTogglePlayback);
        menu.addCommandItem(&commandManager, CommandIDs::TransportToggleLooping);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::TransportRewindPlayHead);
    }
    else if(menuName == "Help")
    {
        menu.addCommandItem(&commandManager, CommandIDs::HelpOpenManual);
        menu.addCommandItem(&commandManager, CommandIDs::HelpOpenForum);
    }
    else
    {
        anlStrongAssert(false && "menu name is invalid");
    }
    return menu;
}

void Application::MainMenuModel::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
    juce::ignoreUnused(topLevelMenuIndex);
    using CommandIDs = CommandTarget::CommandIDs;
    
    auto const& recentFiles = Instance::get().getApplicationAccessor().getAttr<AttrType::recentlyOpenedFilesList>();
    auto const fileIndex = static_cast<size_t>(menuItemID - static_cast<int>(CommandIDs::DocumentOpenRecent));
    if(fileIndex < recentFiles.size())
    {
        Instance::get().openFile(recentFiles[fileIndex]);
    }
}

ANALYSE_FILE_END
