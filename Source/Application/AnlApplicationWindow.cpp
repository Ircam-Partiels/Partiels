#include "AnlApplicationWindow.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Window::Window()
: juce::DocumentWindow(Instance::get().getApplicationName() + " - v" + ProjectInfo::versionString, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId), juce::DocumentWindow::allButtons)
{
#ifndef JUCE_MAC
    mOpenGLContext.setComponentPaintingEnabled(true);
    mOpenGLContext.attachTo(*this);
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
    Instance::get().getDocumentFileBased().addChangeListener(this);
}

Application::Window::~Window()
{
    Instance::get().getDocumentFileBased().removeChangeListener(this);
    removeKeyListener(Instance::get().getApplicationCommandManager().getKeyMappings());
#ifndef JUCE_MAC
    mOpenGLContext.detach();
#endif
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

void Application::Window::lookAndFeelChanged()
{
    juce::DocumentWindow::lookAndFeelChanged();
    setBackgroundColour(getLookAndFeel().findColour(juce::ResizableWindow::ColourIds::backgroundColourId));
}

void Application::Window::handleAsyncUpdate()
{
    Instance::get().getApplicationAccessor().setAttr<AttrType::windowState>(getBounds().toString(), NotificationType::synchronous);
}

void Application::Window::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    juce::ignoreUnused(source);
    anlStrongAssert(source == &Instance::get().getDocumentFileBased());
    auto const file = Instance::get().getDocumentFileBased().getFile();
    auto const name = file.existsAsFile() ? file.getFileNameWithoutExtension() : "Unsaved Project";
    auto const extension = file.existsAsFile() && Instance::get().getDocumentFileBased().hasChangedSinceSaved() ? "*" : "";
    setName(Instance::get().getApplicationName() + " - v" + ProjectInfo::versionString + " - " + name + extension);
}

void Application::Window::moveKeyboardFocusTo(juce::String const& identifier)
{
    mInterface.moveKeyboardFocusTo(identifier);
}

juce::Rectangle<int> Application::Window::getPlotBounds(juce::String const& identifier) const
{
    return mInterface.getPlotBounds(identifier);
}

ANALYSE_FILE_END
