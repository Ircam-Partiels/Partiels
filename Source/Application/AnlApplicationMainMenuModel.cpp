#include "AnlApplicationMainMenuModel.h"
#include "AnlApplicationCommandTarget.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::MainMenuModel::MainMenuModel(juce::DocumentWindow& window)
: mWindow(window)
{
    auto& commandManager = Instance::get().getApplicationCommandManager();
    setApplicationCommandManagerToWatch(&commandManager);
#ifdef JUCE_MAC
    juce::ignoreUnused(mWindow);
    juce::PopupMenu extraAppleMenuItems;
    using CommandIDs = CommandTarget::CommandIDs;
    extraAppleMenuItems.addCommandItem(&commandManager, CommandIDs::HelpOpenAbout);
    extraAppleMenuItems.addSeparator();
    extraAppleMenuItems.addCommandItem(&commandManager, CommandIDs::HelpOpenAudioSettings);
    juce::MenuBarModel::setMacMainMenu(this, &extraAppleMenuItems);
#else
    mWindow.setMenuBar(this);
#endif
}

Application::MainMenuModel::~MainMenuModel()
{
#ifdef JUCE_MAC
    juce::MenuBarModel::setMacMainMenu(nullptr, nullptr);
#else
    mWindow.setMenuBar(nullptr);
#endif
}

juce::StringArray Application::MainMenuModel::getMenuBarNames()
{
    return {"File", "Edit", "Transport", "Points", "Zoom", "Help"};
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
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::AnalysisNew);
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
    else if(menuName == "Zoom")
    {
        menu.addCommandItem(&commandManager, CommandIDs::ZoomIn);
        menu.addCommandItem(&commandManager, CommandIDs::ZoomOut);
    }
    else if(menuName == "Help")
    {
#ifndef JUCE_MAC
        menu.addCommandItem(&commandManager, CommandIDs::HelpOpenAbout);
#endif
        menu.addCommandItem(&commandManager, CommandIDs::HelpOpenManual);
        menu.addCommandItem(&commandManager, CommandIDs::HelpOpenForum);
#ifndef JUCE_MAC
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::HelpOpenAudioSettings);
#endif
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
