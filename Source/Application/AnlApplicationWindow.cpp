#include "AnlApplicationWindow.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Window::Window()
// clang-format off
: juce::DocumentWindow(  Instance::get().getApplicationName() + " - v" + ProjectInfo::versionString
                       , juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId)
                       , juce::DocumentWindow::allButtons
#ifdef JUCE_MAC
                       , false
#else
                       , true
#endif
                       )
// clang-format on
{
    mBoundsConstrainer.setSizeLimits(512, 384, std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
    mBoundsConstrainer.setMinimumOnscreenAmounts(0xffffff, 50, 50, 50);
    setConstrainer(&mBoundsConstrainer);

    setContentNonOwned(&mInterface, false);

    setVisible(true);
    setResizable(true, false);
    setUsingNativeTitleBar(true);
    addKeyListener(Instance::get().getApplicationCommandManager().getKeyMappings());
    Instance::get().getDocumentFileBased().addChangeListener(this);
    auto const state = Instance::get().getApplicationAccessor().getAttr<AttrType::windowState>();
#ifdef JUCE_MAC
    // This is a fix to properly restore fullscreen window state on macOS
    juce::WeakReference<Component> weakReference(this);
    juce::MessageManager::callAsync([=, this]()
                                    {
                                        if(weakReference.get() == nullptr)
                                        {
                                            return;
                                        }
                                        addToDesktop(getDesktopWindowStyleFlags());
                                        if(!restoreWindowStateFromString(state))
                                        {
                                            centreWithSize(1024, 768);
                                        }
                                    });
#else
    if(!restoreWindowStateFromString(state))
    {
        centreWithSize(1024, 768);
    }
#endif
}

Application::Window::~Window()
{
    handleAsyncUpdate();
    Instance::get().getDocumentFileBased().removeChangeListener(this);
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

void Application::Window::maximiseButtonPressed()
{
    juce::DocumentWindow::maximiseButtonPressed();
    triggerAsyncUpdate();
}

void Application::Window::lookAndFeelChanged()
{
    juce::DocumentWindow::lookAndFeelChanged();
    setBackgroundColour(getLookAndFeel().findColour(juce::ResizableWindow::ColourIds::backgroundColourId));
}

void Application::Window::handleAsyncUpdate()
{
    Instance::get().getApplicationAccessor().setAttr<AttrType::windowState>(getWindowStateAsString(), NotificationType::synchronous);
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

juce::Rectangle<int> Application::Window::getPlotBounds(juce::String const& identifier) const
{
    return mInterface.getPlotBounds(identifier);
}

ANALYSE_FILE_END
