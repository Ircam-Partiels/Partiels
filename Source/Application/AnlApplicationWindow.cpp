#include "AnlApplicationWindow.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Window::Window()
// clang-format off
: juce::DocumentWindow(  Instance::get().getApplicationName()
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

    refreshInterface();
    setContentNonOwned(&mCommandTargetInterface, false);

    setVisible(true);
    setResizable(true, false);
    setUsingNativeTitleBar(true);
    addKeyListener(Instance::get().getApplicationCommandManager().getKeyMappings());
    Instance::get().getDocumentFileBased().addChangeListener(this);
    changeListenerCallback(std::addressof(Instance::get().getDocumentFileBased()));
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

void Application::Window::changeListenerCallback([[maybe_unused]] juce::ChangeBroadcaster* source)
{
    MiscWeakAssert(source == std::addressof(Instance::get().getDocumentFileBased()));
    auto const file = Instance::get().getDocumentFileBased().getFile();
    auto const fileName = file.existsAsFile() ? file.getFileNameWithoutExtension() : "Unsaved Project";
    auto const hasChanged = file.existsAsFile() && Instance::get().getDocumentFileBased().hasChangedSinceSaved();
    setName(juce::String("APPNAME vAPPVERSION - FILENAME")
                .replace("APPNAME", Instance::get().getApplicationName())
                .replace("APPVERSION", Instance::get().getApplicationVersion())
                .replace("FILENAME", fileName + (hasChanged ? "*" : "")));
}

Application::Interface& Application::Window::getInterface()
{
    return *mInterface.get();
}

Application::Interface const& Application::Window::getInterface() const
{
    return *mInterface.get();
}

Application::CommandTarget& Application::Window::getCommandTarget()
{
    return mCommandTargetInterface;
}

Application::CommandTarget const& Application::Window::getCommandTarget() const
{
    return mCommandTargetInterface;
}

void Application::Window::refreshInterface()
{
    mInterface = std::make_unique<Interface>();
    mCommandTargetInterface.addAndMakeVisible(mInterface.get());
    mCommandTargetInterface.resized();
}

void Application::Window::CommandTargetInterface::resized()
{
    MiscWeakAssert(getNumChildComponents() == 1);
    if(getNumChildComponents() < 1)
    {
        return;
    }
    if(auto* child = getChildComponent(0))
    {
        child->setBounds(getLocalBounds());
    }
}

ANALYSE_FILE_END
