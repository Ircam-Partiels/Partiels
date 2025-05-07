#include "AnlApplicationMainMenuModel.h"
#include "../Document/AnlDocumentCommandTarget.h"
#include "AnlApplicationCommandTarget.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::MainMenuModel::MainMenuModel([[maybe_unused]] juce::DocumentWindow& window)
#ifndef JUCE_MAC
: mWindow(window)
#endif
{
    setApplicationCommandManagerToWatch(std::addressof(Instance::get().getApplicationCommandManager()));
#ifdef JUCE_MAC
    updateAppleMenuItems();
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
    return {juce::translate("File"), juce::translate("Edit"), juce::translate("Frame"), juce::translate("Transport"), juce::translate("View"), juce::translate("Help")};
}

juce::PopupMenu Application::MainMenuModel::getMenuForIndex(int topLevelMenuIndex, juce::String const& menuName)
{
    juce::ignoreUnused(topLevelMenuIndex);

    using CommandIDs = CommandTarget::CommandIDs;
    auto& commandManager = Instance::get().getApplicationCommandManager();
    juce::PopupMenu menu;
    if(menuName == juce::translate("File"))
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

        menu.addSubMenu(juce::translate("Open Recent"), recentFilesMenu);
        menu.addCommandItem(&commandManager, CommandIDs::documentSave);
        menu.addCommandItem(&commandManager, CommandIDs::documentDuplicate);
        menu.addCommandItem(&commandManager, CommandIDs::documentConsolidate);
        menu.addCommandItem(&commandManager, CommandIDs::documentExport);
        menu.addCommandItem(&commandManager, CommandIDs::documentImport);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::documentBatch);
        menu.addSeparator();
    }
    else if(menuName == juce::translate("Edit"))
    {
        menu.addCommandItem(&commandManager, CommandIDs::editUndo);
        menu.addCommandItem(&commandManager, CommandIDs::editRedo);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::editRemoveItem);
        menu.addCommandItem(&commandManager, CommandIDs::editNewTrack);
        menu.addCommandItem(&commandManager, CommandIDs::editNewGroup);
    }
    else if(menuName == juce::translate("Frame"))
    {
        menu.addCommandItem(&commandManager, CommandIDs::frameSelectAll);
        menu.addCommandItem(&commandManager, CommandIDs::frameDelete);
        menu.addCommandItem(&commandManager, CommandIDs::frameCopy);
        menu.addCommandItem(&commandManager, CommandIDs::frameCut);
        menu.addCommandItem(&commandManager, CommandIDs::framePaste);
        menu.addCommandItem(&commandManager, CommandIDs::frameDuplicate);
        menu.addCommandItem(&commandManager, CommandIDs::frameInsert);
        menu.addCommandItem(&commandManager, CommandIDs::frameBreak);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::frameSystemCopy);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::frameToggleDrawing);
    }
    else if(menuName == juce::translate("Transport"))
    {
        menu.addCommandItem(&commandManager, CommandIDs::transportTogglePlayback);
        menu.addCommandItem(&commandManager, CommandIDs::transportToggleLooping);
        menu.addCommandItem(&commandManager, CommandIDs::transportToggleStopAtLoopEnd);
        menu.addCommandItem(&commandManager, CommandIDs::transportToggleMagnetism);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::transportRewindPlayHead);
        menu.addCommandItem(&commandManager, CommandIDs::transportMovePlayHeadBackward);
        menu.addCommandItem(&commandManager, CommandIDs::transportMovePlayHeadForward);
    }
    else if(menuName == juce::translate("View"))
    {
        juce::PopupMenu colourModeMenu;
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
        juce::PopupMenu scaleMenu;
        auto const currentScale = Instance::get().getApplicationAccessor().getAttr<AttrType::desktopGlobalScaleFactor>();
        for(auto i = 0; i <= 4; ++i)
        {
            auto const scale = 1.0f + static_cast<float>(i) / 4.0f;
            auto const difference = std::abs(currentScale - scale);
            scaleMenu.addItem(juce::String(static_cast<int>(std::round(scale * 100.0f))) + "%", difference >= 0.01f, difference < 0.01f, [=]()
                              {
                                  Instance::get().getApplicationAccessor().setAttr<AttrType::desktopGlobalScaleFactor>(scale, NotificationType::synchronous);
                              });
        }
        menu.addSubMenu(juce::translate("Scale"), scaleMenu);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::viewTimeZoomIn);
        menu.addCommandItem(&commandManager, CommandIDs::viewTimeZoomOut);
        menu.addCommandItem(&commandManager, CommandIDs::viewVerticalZoomIn);
        menu.addCommandItem(&commandManager, CommandIDs::viewVerticalZoomOut);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::viewTimeZoomAnchorOnPlayhead);
        menu.addCommandItem(&commandManager, CommandIDs::viewInfoBubble);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::viewShowItemProperties);
    }
    else if(menuName == juce::translate("Help"))
    {
#ifndef JUCE_MAC
        menu.addCommandItem(&commandManager, CommandIDs::helpOpenAbout);
#endif
        menu.addCommandItem(&commandManager, CommandIDs::helpOpenProjectPage);
#ifndef JUCE_MAC
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::helpOpenAudioSettings);
        menu.addCommandItem(&commandManager, CommandIDs::helpOpenOscSettings);
        menu.addCommandItem(&commandManager, CommandIDs::helpOpenPluginSettings);
        menu.addCommandItem(&commandManager, CommandIDs::helpOpenKeyMappings);
        addGlobalSettingsMenu(menu);
#endif
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::helpAutoUpdate);
        menu.addCommandItem(&commandManager, CommandIDs::helpCheckForUpdate);
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

void Application::MainMenuModel::addGlobalSettingsMenu(juce::PopupMenu& menu)
{
    juce::PopupMenu globalSettingsMenu;
    auto& accessor = Instance::get().getApplicationAccessor();
    auto const silentFileManagement = accessor.getAttr<AttrType::silentFileManagement>();
    globalSettingsMenu.addItem(juce::translate("Silent File Management"), true, silentFileManagement, []()
                               {
                                   Instance::get().getApplicationAccessor().setAttr<AttrType::silentFileManagement>(!Instance::get().getApplicationAccessor().getAttr<AttrType::silentFileManagement>(), NotificationType::synchronous);
                               });
    juce::PopupMenu templateMenu;
    auto const templateFile = accessor.getAttr<AttrType::defaultTemplateFile>();
    templateMenu.addItem(juce::translate("None"), true, !templateFile.existsAsFile(), []()
                         {
                             Instance::get().getApplicationAccessor().setAttr<AttrType::defaultTemplateFile>(juce::File(), NotificationType::synchronous);
                         });
    templateMenu.addItem(juce::translate("Factory"), true, templateFile == Accessor::getFactoryTemplateFile(), []()
                         {
                             Instance::get().getApplicationAccessor().setAttr<AttrType::defaultTemplateFile>(Accessor::getFactoryTemplateFile(), NotificationType::synchronous);
                         });
    if(templateFile.existsAsFile() && templateFile != Accessor::getFactoryTemplateFile())
    {
        templateMenu.addItem(templateFile.getFileNameWithoutExtension(), false, true, nullptr);
    }
    templateMenu.addItem(juce::translate("Select..."), true, false, []()
                         {
                             if(auto* window = Instance::get().getWindow())
                             {
                                 window->getInterface().selectDefaultTemplateFile();
                             }
                         });
    globalSettingsMenu.addSubMenu(juce::translate("Default Template"), templateMenu);
    menu.addSubMenu(juce::translate("Global Settings"), globalSettingsMenu);
}

#ifdef JUCE_MAC
void Application::MainMenuModel::updateAppleMenuItems()
{
    juce::PopupMenu extraAppleMenuItems;
    using CommandIDs = CommandTarget::CommandIDs;
    auto& commandManager = Instance::get().getApplicationCommandManager();
    extraAppleMenuItems.addCommandItem(&commandManager, CommandIDs::helpOpenAbout);
    extraAppleMenuItems.addSeparator();
    extraAppleMenuItems.addCommandItem(&commandManager, CommandIDs::helpOpenAudioSettings);
    extraAppleMenuItems.addCommandItem(&commandManager, CommandIDs::helpOpenOscSettings);
    extraAppleMenuItems.addCommandItem(&commandManager, CommandIDs::helpOpenPluginSettings);
    extraAppleMenuItems.addCommandItem(&commandManager, CommandIDs::helpOpenKeyMappings);
    addGlobalSettingsMenu(extraAppleMenuItems);
    juce::MenuBarModel::setMacMainMenu(this, &extraAppleMenuItems);
}
#endif

ANALYSE_FILE_END
