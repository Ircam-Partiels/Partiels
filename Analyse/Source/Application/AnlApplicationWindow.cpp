
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Window::Window(juce::Component& content)
: juce::DocumentWindow(Instance::get().getApplicationName() + " - " + ProjectInfo::versionString, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId), juce::DocumentWindow::allButtons)
{
    mOpenGLContext.setMultisamplingEnabled(true);
    mOpenGLContext.setComponentPaintingEnabled(true);
    mOpenGLContext.setImageCacheSize(2^24 * sizeof(float));
    mOpenGLContext.attachTo(*this);
    
    if(!restoreWindowStateFromString(Instance::get().getController().getWindowState()))
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
    addKeyListener(Instance::get().getCommandManager().getKeyMappings());
    
#if !JUCE_MAC
    setMenuBar(&AudioSculpt4Application::getMenuBarModel());
#endif
}

Application::Window::~Window()
{
    mOpenGLContext.detach();
}

void Application::Window::closeButtonPressed()
{
    JUCEApplication::getInstance()->systemRequestedQuit();
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
    Instance::get().getController().setWindowState(getBounds().toString(), juce::NotificationType::sendNotificationSync);
}

ANALYSE_FILE_END

