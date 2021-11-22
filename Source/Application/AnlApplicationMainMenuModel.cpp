#include "AnlApplicationMainMenuModel.h"
#include "AnlApplicationCommandTarget.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::MainMenuModel::MainMenuModel(juce::DocumentWindow& window)
: mWindow(window)
{
    auto& commandManager = Instance::get().getApplicationCommandManager();
    setApplicationCommandManagerToWatch(&commandManager);
#if JUCE_MAC
    juce::ignoreUnused(mWindow);
    juce::PopupMenu extraAppleMenuItems;
    using CommandIDs = CommandTarget::CommandIDs;
    extraAppleMenuItems.addCommandItem(&commandManager, CommandIDs::helpOpenAbout);
    extraAppleMenuItems.addSeparator();
    extraAppleMenuItems.addCommandItem(&commandManager, CommandIDs::helpOpenAudioSettings);
    extraAppleMenuItems.addCommandItem(&commandManager, CommandIDs::helpOpenPluginSettings);
    juce::MenuBarModel::setMacMainMenu(this, &extraAppleMenuItems);
#else
    mWindow.setMenuBar(this);
#endif
}

Application::MainMenuModel::~MainMenuModel()
{
#if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu(nullptr, nullptr);
#else
    mWindow.setMenuBar(nullptr);
#endif
}

juce::StringArray Application::MainMenuModel::getMenuBarNames()
{
    return {"File", "Edit", "Transport", "View", "Help"};
}

juce::PopupMenu Application::MainMenuModel::getMenuForIndex(int topLevelMenuIndex, juce::String const& menuName)
{
    juce::ignoreUnused(topLevelMenuIndex);

    using CommandIDs = CommandTarget::CommandIDs;
    auto& commandManager = Instance::get().getApplicationCommandManager();
    juce::PopupMenu menu;
    if(menuName == "File")
    {
        menu.addCommandItem(&commandManager, CommandIDs::documentNew);
        menu.addCommandItem(&commandManager, CommandIDs::documentOpen);

        juce::PopupMenu recentFilesMenu;
        auto const& recentFiles = Instance::get().getApplicationAccessor().getAttr<AttrType::recentlyOpenedFilesList>();
        for(auto const& recentFile : recentFiles)
        {
            auto const fileName = recentFile.getFileNameWithoutExtension();
            auto const hasDuplicate = std::count_if(recentFiles.cbegin(), recentFiles.cend(), [&](auto const file)
                                                    {
                                                        return fileName == file.getFileNameWithoutExtension();
                                                    }) > 1l;
            auto const isActive = Instance::get().getDocumentFileBased().getFile() != recentFile;
            auto const name = fileName + (hasDuplicate ? " (" + recentFile.getParentDirectory().getFileName() + ")" : "");
            recentFilesMenu.addItem(name, isActive, !isActive, [=]()
                                    {
                                        Instance::get().openDocumentFile(recentFile);
                                    });
        }

        menu.addSubMenu("Open Recent", recentFilesMenu);
        menu.addCommandItem(&commandManager, CommandIDs::documentSave);
        menu.addCommandItem(&commandManager, CommandIDs::documentDuplicate);
        menu.addCommandItem(&commandManager, CommandIDs::documentConsolidate);
        menu.addCommandItem(&commandManager, CommandIDs::documentExport);
        menu.addCommandItem(&commandManager, CommandIDs::documentImport);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::documentBatch);
    }
    else if(menuName == "Edit")
    {
        menu.addCommandItem(&commandManager, CommandIDs::editUndo);
        menu.addCommandItem(&commandManager, CommandIDs::editRedo);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::editRemoveItem);
        menu.addCommandItem(&commandManager, CommandIDs::editNewTrack);
        menu.addCommandItem(&commandManager, CommandIDs::editNewGroup);
    }
    else if(menuName == "Transport")
    {
        menu.addCommandItem(&commandManager, CommandIDs::transportTogglePlayback);
        menu.addCommandItem(&commandManager, CommandIDs::transportToggleLooping);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::transportRewindPlayHead);
    }
    else if(menuName == "View")
    {
        juce::PopupMenu colourModeMenu;
        using ColourMode = LookAndFeel::ColourChart::Mode;
        auto const selectedMode = Instance::get().getApplicationAccessor().getAttr<AttrType::colourMode>();
        for(auto const& entry : magic_enum::enum_entries<ColourMode>())
        {
            auto const name = Format::withFirstCharUpperCase(std::string(entry.second));
            colourModeMenu.addItem(juce::String(name), true, selectedMode == entry.first, [=]()
                                   {
                                       auto& accessor = Instance::get().getApplicationAccessor();
                                       accessor.setAttr<AttrType::colourMode>(entry.first, NotificationType::synchronous);
                                   });
        }
        menu.addSubMenu(juce::translate("Theme"), colourModeMenu);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::viewZoomIn);
        menu.addCommandItem(&commandManager, CommandIDs::viewZoomOut);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::viewInfoBubble);
    }
    else if(menuName == "Help")
    {
#ifndef JUCE_MAC
        menu.addCommandItem(&commandManager, CommandIDs::helpOpenAbout);
#endif
        menu.addCommandItem(&commandManager, CommandIDs::helpOpenProjectPage);
#ifndef JUCE_MAC
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::helpOpenAudioSettings);
        menu.addCommandItem(&commandManager, CommandIDs::helpOpenPluginSettings);
#endif
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::helpSdifConverter);
    }
    else
    {
        anlStrongAssert(false && "menu name is invalid");
    }
    return menu;
}

void Application::MainMenuModel::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
    juce::ignoreUnused(menuItemID, topLevelMenuIndex);
}

ANALYSE_FILE_END
