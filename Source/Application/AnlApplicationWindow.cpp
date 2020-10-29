#include "AnlApplicationWindow.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationCommandTarget.h"

ANALYSE_FILE_BEGIN

juce::StringArray Application::Window::MainMenuModel::getMenuBarNames()
{
    return {"File", "Help"};
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
        auto const& recentFiles = Instance::get().getAccessor().getModel().recentlyOpenedFilesList;
        for(auto const& file : recentFiles)
        {
            recentFilesMenu.addItem(recentFileIndex++, file.getFileNameWithoutExtension());
        }
        menu.addSubMenu("Open Recent", recentFilesMenu);
        menu.addCommandItem(&commandManager, CommandIDs::Save);
        menu.addCommandItem(&commandManager, CommandIDs::Duplicate);
        menu.addCommandItem(&commandManager, CommandIDs::Consolidate);
        menu.addCommandItem(&commandManager, CommandIDs::Close);
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
    
    auto const& recentFiles = Instance::get().getAccessor().getModel().recentlyOpenedFilesList;
    auto const fileIndex = static_cast<size_t>(menuItemID - static_cast<int>(CommandIDs::OpenRecent) - 1);
    anlStrongAssert(fileIndex < recentFiles.size());
    if(fileIndex < recentFiles.size())
    {
        JUCE_COMPILER_WARNING("todo")
    }
}

Application::Window::Window(juce::Component& content)
: juce::DocumentWindow(Instance::get().getApplicationName() + " - " + ProjectInfo::versionString, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId), juce::DocumentWindow::allButtons)
{
    if(!restoreWindowStateFromString(Instance::get().getAccessor().getModel().windowState))
    {
        centreWithSize(1024, 768);
    }
    mBoundsConstrainer.setSizeLimits(1024, 768, std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
    mBoundsConstrainer.setMinimumOnscreenAmounts(0xffffff, 50, 50, 50);
    setConstrainer(&mBoundsConstrainer);
    
    setContentNonOwned(&content, false);
    
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
    auto model = Instance::get().getAccessor().getModel();
    model.windowState = getBounds().toString();
    Instance::get().getAccessor().fromModel(model, juce::NotificationType::sendNotificationSync);
}

ANALYSE_FILE_END

