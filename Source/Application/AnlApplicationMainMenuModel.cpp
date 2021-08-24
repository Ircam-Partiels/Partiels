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
    extraAppleMenuItems.addCommandItem(&commandManager, CommandIDs::HelpOpenPluginPath);
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
        menu.addCommandItem(&commandManager, CommandIDs::DocumentNew);
        menu.addCommandItem(&commandManager, CommandIDs::DocumentOpen);

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
                                        Instance::get().openFiles({recentFile});
                                    });
        }

        menu.addSubMenu("Open Recent", recentFilesMenu);
        menu.addCommandItem(&commandManager, CommandIDs::DocumentSave);
        menu.addCommandItem(&commandManager, CommandIDs::DocumentDuplicate);
        menu.addCommandItem(&commandManager, CommandIDs::DocumentConsolidate);
        menu.addCommandItem(&commandManager, CommandIDs::DocumentExport);
        menu.addCommandItem(&commandManager, CommandIDs::DocumentImport);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::DocumentBatch);
    }
    else if(menuName == "Edit")
    {
        menu.addCommandItem(&commandManager, CommandIDs::EditUndo);
        menu.addCommandItem(&commandManager, CommandIDs::EditRedo);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::EditRemoveItem);
        menu.addCommandItem(&commandManager, CommandIDs::EditNewTrack);
        menu.addCommandItem(&commandManager, CommandIDs::EditNewGroup);
    }
    else if(menuName == "Transport")
    {
        menu.addCommandItem(&commandManager, CommandIDs::TransportTogglePlayback);
        menu.addCommandItem(&commandManager, CommandIDs::TransportToggleLooping);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::TransportRewindPlayHead);
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
        menu.addSubMenu("Theme", colourModeMenu);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::ViewZoomIn);
        menu.addCommandItem(&commandManager, CommandIDs::ViewZoomOut);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::ViewInfoBubble);
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
        menu.addCommandItem(&commandManager, CommandIDs::HelpOpenPluginPath);
#endif
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::HelpSDIFConverter);
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
