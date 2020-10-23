#include "AnlApplicationWindow.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Window::Window(juce::Component& content)
: juce::DocumentWindow(Instance::get().getApplicationName() + " - " + ProjectInfo::versionString, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId), juce::DocumentWindow::allButtons)
{
    if(!restoreWindowStateFromString(Instance::get().getAccessor().getModel().windowState))
    {
        centreWithSize(1024, 768);
    }
    mBoundsConstrainer.setSizeLimits(200//1024
                                     , 768, std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
    mBoundsConstrainer.setMinimumOnscreenAmounts(0xffffff, 50, 50, 50);
    setConstrainer(&mBoundsConstrainer);
    
    setContentNonOwned(&content, false);
    
    setVisible(true);
    setResizable(true, false);
    setUsingNativeTitleBar(true);
    addKeyListener(Instance::get().getApplicationCommandManager().getKeyMappings());
    
#if !JUCE_MAC
    //setMenuBar(&AudioSculpt4Application::getMenuBarModel());
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

