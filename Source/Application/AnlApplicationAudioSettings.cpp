#include "AnlApplicationAudioSettings.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::AudioSettings::AudioSettings()
: mPropertyDriver("Driver", "The current audio device driver", "", {}, [&](size_t index)
{
    auto const driverName = mPropertyDriver.entry.getItemText(static_cast<int>(index));
    auto& audioDeviceManager = Instance::get().getAudioDeviceManager();
    if(audioDeviceManager.getCurrentAudioDeviceType() == driverName)
    {
        return;
    }
    
    audioDeviceManager.setCurrentAudioDeviceType(driverName, true);
})
, mPropertyOutputDevice("Output Device", "The current output device", "", {}, [&](size_t index)
{
    auto const deviceName = mPropertyOutputDevice.entry.getItemText(static_cast<int>(index));
    auto& audioDeviceManager = Instance::get().getAudioDeviceManager();
    auto currentAudioSetup = audioDeviceManager.getAudioDeviceSetup();
    if(currentAudioSetup.outputDeviceName == deviceName)
    {
        return;
    }
        
    currentAudioSetup.outputDeviceName = deviceName;
    auto const error = audioDeviceManager.setAudioDeviceSetup(currentAudioSetup, true);
    if(error.isNotEmpty())
    {
        MessageWindow::show(MessageWindow::MessageType::warning, "Error loading audio device settings!", error);
    }
})
, mPropertySampleRate("Sample Rate", "The current device sample rate", "Hz", {}, [&](size_t index)
{
    auto const sampleRate = static_cast<double>(mPropertySampleRate.entry.getItemId(static_cast<int>(index)));
    auto& audioDeviceManager = Instance::get().getAudioDeviceManager();
    auto currentAudioSetup = audioDeviceManager.getAudioDeviceSetup();
    if(std::abs(currentAudioSetup.sampleRate - sampleRate) <= std::numeric_limits<double>::epsilon())
    {
        return;
    }
        
    currentAudioSetup.sampleRate = sampleRate;
    auto const error = audioDeviceManager.setAudioDeviceSetup(currentAudioSetup, true);
    if(error.isNotEmpty())
    {
        MessageWindow::show(MessageWindow::MessageType::warning, "Error loading audio device settings!", error);
    }
})
, mPropertyBufferSize("Buffer Size", "The current buffer size", "samples", {}, [&](size_t index)
{
    auto const bufferSize = mPropertyBufferSize.entry.getItemId(static_cast<int>(index));
    auto& audioDeviceManager = Instance::get().getAudioDeviceManager();
    auto currentAudioSetup = audioDeviceManager.getAudioDeviceSetup();
    if(currentAudioSetup.bufferSize == bufferSize)
    {
        return;
    }
    
    currentAudioSetup.bufferSize = bufferSize;
    auto const error = audioDeviceManager.setAudioDeviceSetup(currentAudioSetup, true);
    if(error.isNotEmpty())
    {
        MessageWindow::show(MessageWindow::MessageType::warning, "Error loading audio device settings!", error);
    }
})
, mPropertyBufferSizeNumber("Buffer Size", "The current buffer size", "samples", {8.0f, 8192.0f}, 1.0f, [&](float value)
{
    auto const bufferSize = static_cast<int>(std::round(value));
    auto& audioDeviceManager = Instance::get().getAudioDeviceManager();
    auto currentAudioSetup = audioDeviceManager.getAudioDeviceSetup();
    if(currentAudioSetup.bufferSize == bufferSize)
    {
        return;
    }
    auto* currentDevice = audioDeviceManager.getCurrentAudioDevice();
    anlStrongAssert(currentDevice != nullptr);
    if(currentDevice == nullptr)
    {
        MessageWindow::show(MessageWindow::MessageType::warning, "Error loading audio device settings!", "No audio device selected.");
        return;
    }
    
    auto const bufferSizes = currentDevice->getAvailableBufferSizes();
    auto const it = std::lower_bound(bufferSizes.begin(), bufferSizes.end(), bufferSize);
    if(it == bufferSizes.end())
    {
        MessageWindow::show(MessageWindow::MessageType::warning, "Error loading audio device settings!", "Buffer size BUFFERSIZE is not supported by the audio device.", {{"BUFFERSIZE", juce::String(bufferSize)}});
        return;
    }
    
    currentAudioSetup.bufferSize = bufferSize;
    auto const error = audioDeviceManager.setAudioDeviceSetup(currentAudioSetup, true);
    if(error.isNotEmpty())
    {
        MessageWindow::show(MessageWindow::MessageType::warning, "Error loading audio device settings!", error);
    }
})
, mPropertyDriverPanel("Audio Device Panel...", "Show audio device panel", []()
{
    auto& audioDeviceManager = Instance::get().getAudioDeviceManager();
    auto* audioDevice = audioDeviceManager.getCurrentAudioDevice();
    if(audioDevice != nullptr && audioDevice->hasControlPanel())
    {
        if(audioDevice->showControlPanel())
        {
            // Force to update the audio device
            audioDeviceManager.closeAudioDevice();
            audioDeviceManager.restartLastAudioDevice();
        }
    }
})
{
    addAndMakeVisible(mPropertyDriver);
    addAndMakeVisible(mPropertyOutputDevice);
    addAndMakeVisible(mPropertySampleRate);
#ifdef JUCE_MAC
    addAndMakeVisible(mPropertyBufferSize);
#else
    addAndMakeVisible(mPropertyBufferSizeNumber);
#endif
    addAndMakeVisible(mPropertyDriverPanel);

    setSize(300, 200);
    Instance::get().getAudioDeviceManager().addChangeListener(this);
    changeListenerCallback(&Instance::get().getAudioDeviceManager());
}

Application::AudioSettings::~AudioSettings()
{
    Instance::get().getAudioDeviceManager().removeChangeListener(this);
}

void Application::AudioSettings::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyDriver);
    setBounds(mPropertyOutputDevice);
    setBounds(mPropertySampleRate);
    setBounds(mPropertyBufferSize);
    setBounds(mPropertyBufferSizeNumber);
    setBounds(mPropertyDriverPanel);
    setSize(bounds.getWidth(), bounds.getY() + 2);
}

void Application::AudioSettings::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    auto& deviceManager = Instance::get().getAudioDeviceManager();
    anlStrongAssert(source == &deviceManager);
    if(source != &deviceManager)
    {
        return;
    }
    
    mPropertyDriver.entry.clear(juce::NotificationType::dontSendNotification);
    mPropertyOutputDevice.entry.clear(juce::NotificationType::dontSendNotification);
    mPropertySampleRate.entry.clear(juce::NotificationType::dontSendNotification);
#ifdef JUCE_MAC
    mPropertyBufferSize.entry.clear(juce::NotificationType::dontSendNotification);
#endif
    
    // Update drivers list
    auto const currentDriverName = deviceManager.getCurrentAudioDeviceType();
    auto const& driverTypes = deviceManager.getAvailableDeviceTypes();
    mPropertyDriver.setEnabled(!driverTypes.isEmpty());
    for(int i = 0; i < driverTypes.size(); ++i)
    {
        auto const driverName = driverTypes.getUnchecked(i)->getTypeName();
        mPropertyDriver.entry.addItem(driverName, i + 1);
        if(driverName == currentDriverName)
        {
            mPropertyDriver.entry.setSelectedItemIndex(i);
        }
    }
    
    // Update output devices list
    auto* currentDriverObject = deviceManager.getCurrentDeviceTypeObject();
    auto* currentAudioDevice = deviceManager.getCurrentAudioDevice();
    mPropertyOutputDevice.setEnabled(currentDriverObject != nullptr);
    if(currentDriverObject != nullptr)
    {
        auto const outputDeviceNames = currentDriverObject->getDeviceNames(false);
        for(int i = 0; i < outputDeviceNames.size(); i++)
        {
            mPropertyOutputDevice.entry.addItem(outputDeviceNames[i], i + 1);
            if(outputDeviceNames[i] == currentAudioDevice->getName())
            {
                mPropertyOutputDevice.entry.setSelectedItemIndex(i);
            }
        }
        mPropertyOutputDevice.entry.addItem(juce::translate("None"), -1);
    }
    
    // Update sample rates and buffer sizes lists
    mPropertySampleRate.setEnabled(currentAudioDevice != nullptr);
    mPropertyBufferSize.setEnabled(currentAudioDevice != nullptr);
    mPropertyBufferSizeNumber.setEnabled(currentAudioDevice != nullptr);
    mPropertyDriverPanel.setVisible(currentAudioDevice != nullptr);
    
    if(currentAudioDevice != nullptr)
    {
        auto const currentSampleRate = currentAudioDevice->getCurrentSampleRate();
        auto const currentBufferSize = currentAudioDevice->getCurrentBufferSizeSamples();
        
        auto const availableSampleRates = currentAudioDevice->getAvailableSampleRates();
        mPropertyBufferSize.setEnabled(availableSampleRates.isEmpty());
        for(auto const sampleRate : availableSampleRates)
        {
            mPropertySampleRate.entry.addItem(juce::String(static_cast<int>(sampleRate)) + "Hz", static_cast<int>(sampleRate));
        }
        mPropertySampleRate.setEnabled(mPropertySampleRate.entry.getNumItems() > 1);
        mPropertySampleRate.entry.setSelectedId(static_cast<int>(currentSampleRate));
        mPropertySampleRate.entry.setTextWhenNothingSelected(juce::String(static_cast<int>(currentSampleRate)) + "Hz");
        
        auto availableBufferSizes = currentAudioDevice->getAvailableBufferSizes();
        availableBufferSizes.removeIf([&](int value)
        {
            return !juce::isPowerOfTwo(value) && value != currentBufferSize;
        });
        mPropertyBufferSize.setEnabled(availableBufferSizes.isEmpty());
        for(auto const bufferSize : availableBufferSizes)
        {
            mPropertyBufferSize.entry.addItem(juce::String(bufferSize) + " samples (" + juce::String(bufferSize * 1000.0 / currentSampleRate, 1) + " ms)", bufferSize);
        }
        mPropertyBufferSize.setEnabled(mPropertyBufferSize.entry.getNumItems() > 1);
        mPropertyBufferSize.entry.setSelectedId(currentBufferSize);
        mPropertyBufferSize.entry.setTextWhenNothingSelected(juce::String(currentBufferSize) + " samples (" + juce::String(currentBufferSize * 1000.0 / currentSampleRate, 1) + " ms)");
        
        mPropertyBufferSizeNumber.setEnabled(!availableBufferSizes.isEmpty());
        if(!availableBufferSizes.isEmpty())
        {
            auto const minBufferSize = static_cast<double>(availableBufferSizes.getFirst());
            auto const maxBufferSize = static_cast<double>(availableBufferSizes.getLast());
            mPropertyBufferSizeNumber.entry.setRange({minBufferSize, maxBufferSize}, 1.0, juce::NotificationType::dontSendNotification);
        }
        
        mPropertyBufferSizeNumber.entry.setValue(static_cast<double>(currentBufferSize), juce::NotificationType::dontSendNotification);
        mPropertyBufferSizeNumber.entry.setTextValueSuffix(" samples (" + juce::String(currentBufferSize * 1000.0 / currentSampleRate, 1) + " ms)");
        
        mPropertyDriverPanel.setVisible(currentAudioDevice->hasControlPanel());
    }
    resized();
}

void Application::AudioSettings::show()
{
    if(mFloatingWindow.getContentComponent() == nullptr)
    {
        auto const& desktop = juce::Desktop::getInstance();
        auto const mousePosition = desktop.getMainMouseSource().getScreenPosition().toInt();
        auto const* display = desktop.getDisplays().getDisplayForPoint(mousePosition);
        anlStrongAssert(display != nullptr);
        if(display != nullptr)
        {
            auto const bounds = display->userArea.withSizeKeepingCentre(mFloatingWindow.getWidth(), mFloatingWindow.getHeight());
            mFloatingWindow.setBounds(bounds);
        }
        mFloatingWindow.setContentNonOwned(this, true);
    }

    mFloatingWindow.setVisible(true);
    mFloatingWindow.toFront(false);
}

ANALYSE_FILE_END
