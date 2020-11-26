#include "AnlApplicationWindow.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationCommandTarget.h"

ANALYSE_FILE_BEGIN

juce::StringArray Application::Window::MainMenuModel::getMenuBarNames()
{
    return {"File", "Transport", "Help"};
}

juce::PopupMenu Application::Window::MainMenuModel::getMenuForIndex(int topLevelMenuIndex, juce::String const& menuName)
{
    juce::ignoreUnused(topLevelMenuIndex);
    
    using CommandIDs = CommandTarget::CommandIDs;
    auto& commandManager = Instance::get().getApplicationCommandManager();
    juce::PopupMenu menu;
    if(menuName == "File")
    {
        menu.addCommandItem(&commandManager, CommandIDs::New);
        menu.addCommandItem(&commandManager, CommandIDs::Open);
        juce::PopupMenu recentFilesMenu;
        auto recentFileIndex = static_cast<int>(CommandIDs::OpenRecent);
        auto const& recentFiles = Instance::get().getApplicationAccessor().getAttr<AttrType::recentlyOpenedFilesList>();
        for(auto const& file : recentFiles)
        {
            auto const isActive = Instance::get().getDocumentFileBased().getFile() != file;
            recentFilesMenu.addItem(recentFileIndex++, file.getFileNameWithoutExtension(), isActive);
        }
        menu.addSubMenu("Open Recent", recentFilesMenu);
        menu.addCommandItem(&commandManager, CommandIDs::Save);
        menu.addCommandItem(&commandManager, CommandIDs::Duplicate);
        menu.addCommandItem(&commandManager, CommandIDs::Consolidate);
    }
    else if(menuName == "Transport")
    {
        menu.addCommandItem(&commandManager, CommandIDs::TogglePlayback);
        menu.addCommandItem(&commandManager, CommandIDs::ToggleLooping);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::RewindPlayHead);
    }
    else if(menuName == "Help")
    {
        
    }
    else
    {
        anlStrongAssert(false && "menu name is invalid");
    }
    return menu;
}

void Application::Window::MainMenuModel::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
    juce::ignoreUnused(topLevelMenuIndex);
    using CommandIDs = CommandTarget::CommandIDs;
    
    auto const& recentFiles = Instance::get().getApplicationAccessor().getAttr<AttrType::recentlyOpenedFilesList>();
    auto const fileIndex = static_cast<size_t>(menuItemID - static_cast<int>(CommandIDs::OpenRecent));
    if(fileIndex < recentFiles.size())
    {
        Instance::get().openFile(recentFiles[fileIndex]);
    }
}

Application::Window::Window()
: juce::DocumentWindow(Instance::get().getApplicationName() + " - " + ProjectInfo::versionString, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId), juce::DocumentWindow::allButtons)
{
    if(!restoreWindowStateFromString(Instance::get().getApplicationAccessor().getAttr<AttrType::windowState>()))
    {
        centreWithSize(1024, 768);
    }
    mBoundsConstrainer.setSizeLimits(1024, 768, std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
    mBoundsConstrainer.setMinimumOnscreenAmounts(0xffffff, 50, 50, 50);
    setConstrainer(&mBoundsConstrainer);
    
    setContentNonOwned(&mInterface, false);
    
    setVisible(true);
    setResizable(true, false);
    setUsingNativeTitleBar(true);
    addKeyListener(Instance::get().getApplicationCommandManager().getKeyMappings());
    mMainMenuModel.setApplicationCommandManagerToWatch(&Instance::get().getApplicationCommandManager());
#if !JUCE_MAC
    setMenuBar(&MainMenuModel);
#elif !defined(JUCE_IOS)
    juce::PopupMenu extraAppleMenuItems;
    juce::MenuBarModel::setMacMainMenu(&mMainMenuModel, nullptr);
#endif
}

Application::Window::~Window()
{
#if JUCE_MAC && (!defined(JUCE_IOS))
    juce::MenuBarModel::setMacMainMenu(nullptr);
#endif
    removeKeyListener(Instance::get().getApplicationCommandManager().getKeyMappings());
}

void Application::Window::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

void Application::Window::resized()
{
    juce::DocumentWindow::resized();
    triggerAsyncUpdate();
}

void Application::Window::moved()
{
    juce::DocumentWindow::moved();
    triggerAsyncUpdate();
}

void Application::Window::handleAsyncUpdate()
{
    Instance::get().getApplicationAccessor().setAttr<AttrType::windowState>(getBounds().toString(), NotificationType::synchronous);
}

ANALYSE_FILE_END

