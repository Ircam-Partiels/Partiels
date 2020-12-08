#include "AnlApplicationWindow.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Window::Window()
: juce::DocumentWindow(Instance::get().getApplicationName() + " - " + ProjectInfo::versionString, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId), juce::DocumentWindow::allButtons)
{
#ifdef BRIOCHE_ALPHA_VERSION
    setName(Instance::get().getApplicationName() + " - " + ProjectInfo::versionString + "-alpha" + juce::String(BRIOCHE_ALPHA_VERSION));
#endif
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
    setMenuBar(&mMainMenuModel);
#elif !defined(JUCE_IOS)
    juce::PopupMenu extraAppleMenuItems;
    juce::MenuBarModel::setMacMainMenu(&mMainMenuModel, nullptr);
#endif
}

Application::Window::~Window()
{
#if !JUCE_MAC
    setMenuBar(nullptr);
#elif  !defined(JUCE_IOS)
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

