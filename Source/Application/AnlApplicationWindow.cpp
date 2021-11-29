#include "AnlApplicationWindow.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Window::DesktopScaler::DesktopScaler()
: FloatingWindowContainer("Desktop Global Scale", *this)
, mScale(juce::translate("Scale"), juce::translate("Defines the desktop global scale"), "x", juce::Range<float>{1.0f, 4.0f}, 0.1f, [](float scale)
         {
             auto& acsr = Instance::get().getApplicationAccessor();
             acsr.setAttr<AttrType::desktopGlobalScaleFactor>(scale, NotificationType::synchronous);
         })
{
    addAndMakeVisible(mScale);
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::windowState:
            case AttrType::recentlyOpenedFilesList:
            case AttrType::currentDocumentFile:
            case AttrType::colourMode:
            case AttrType::showInfoBubble:
            case AttrType::autoLoadConvertedFile:
            case AttrType::adaptationToSampleRate:
            case AttrType::exportOptions:
                break;
            case AttrType::desktopGlobalScaleFactor:
            {
                mScale.entry.setValue(acsr.getAttr<AttrType::desktopGlobalScaleFactor>(), juce::NotificationType::dontSendNotification);
            }
            break;
        }
    };
    Instance::get().getApplicationAccessor().addListener(mListener, NotificationType::synchronous);
    setBounds(0, 0, 200, 24);
}

Application::Window::DesktopScaler::~DesktopScaler()
{
    Instance::get().getApplicationAccessor().removeListener(mListener);
}

void Application::Window::DesktopScaler::resized()
{
    auto bounds = getLocalBounds();
    mScale.setBounds(bounds.removeFromTop(mScale.getHeight()));
    setSize(bounds.getWidth(), std::max(bounds.getY(), 24) + 2);
}

Application::Window::Window()
: juce::DocumentWindow(Instance::get().getApplicationName() + " - v" + ProjectInfo::versionString, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId), juce::DocumentWindow::allButtons)
{
    if(!restoreWindowStateFromString(Instance::get().getApplicationAccessor().getAttr<AttrType::windowState>()))
    {
        centreWithSize(1024, 768);
    }
    mBoundsConstrainer.setSizeLimits(512, 384, std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
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

void Application::Window::showDesktopScaler()
{
    mDesktopScaler.show();
}

ANALYSE_FILE_END
