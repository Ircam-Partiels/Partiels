#include "AnlApplicationAudioSettings.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::AudioSettingsContent::PropertyChannelRouting::PropertyChannelRouting(std::function<void(size_t row, size_t column)> fn)
: PropertyComponent<BooleanMatrixSelector>("Channels Routing", "Define the output channels rounting", {})
{
    entry.onClick = std::move(fn);
}

void Application::AudioSettingsContent::PropertyChannelRouting::resized()
{
    auto bounds = getLocalBounds();
    title.setBounds(bounds.removeFromTop(24));
    entry.setBounds(bounds);
}

Application::AudioSettingsContent::AudioSettingsContent()
: mPropertyDriver(juce::translate("Driver"), juce::translate("The current audio device driver"), "", {}, [&](size_t index)
                  {
                      auto const driverName = mPropertyDriver.entry.getItemText(static_cast<int>(index));
                      auto& audioDeviceManager = Instance::get().getAudioDeviceManager();
                      if(audioDeviceManager.getCurrentAudioDeviceType() == driverName)
                      {
                          return;
                      }
                      audioDeviceManager.setCurrentAudioDeviceType(driverName, true);
                  })
, mPropertyOutputDevice(juce::translate("Output Device"), juce::translate("The current output device"), "", {}, [&](size_t index)
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
                                Properties::askToRestoreDefaultAudioSettings(error);
                            }
                        })
, mPropertySampleRate(juce::translate("Sample Rate"), juce::translate("The current device sample rate"), "Hz", {}, [&](size_t index)
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
                              Properties::askToRestoreDefaultAudioSettings(error);
                          }
                      })
, mPropertyBufferSize(juce::translate("Buffer Size"), juce::translate("The current buffer size"), "samples", {}, [&](size_t index)
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
                              Properties::askToRestoreDefaultAudioSettings(error);
                          }
                      })
, mPropertyBufferSizeNumber(juce::translate("Buffer Size"), juce::translate("The current buffer size"), "samples", {8.0f, 8192.0f}, 1.0f, [&](float value)
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
                                    Properties::askToRestoreDefaultAudioSettings(juce::translate("No audio device selected."));
                                    return;
                                }

                                if(!currentDevice->getAvailableBufferSizes().contains(bufferSize))
                                {
                                    Properties::askToRestoreDefaultAudioSettings(juce::translate("Buffer size BUFFERSIZE is not supported by the audio device.").replace("BUFFERSIZE", juce::String(bufferSize)));
                                    return;
                                }

                                currentAudioSetup.bufferSize = bufferSize;
                                auto const error = audioDeviceManager.setAudioDeviceSetup(currentAudioSetup, true);
                                if(error.isNotEmpty())
                                {
                                    Properties::askToRestoreDefaultAudioSettings(error);
                                }
                            })
, mPropertyChannelRouting([](size_t row, size_t column)
                          {
                              auto& accessor = Instance::get().getApplicationAccessor();
                              auto routing = accessor.getAttr<AttrType::routingMatrix>();
                              if(row < routing.size() && column < routing.at(row).size())
                              {
                                  routing[row][column] = !routing.at(row).at(column);
                              }
                              accessor.setAttr<AttrType::routingMatrix>(routing, NotificationType::synchronous);
                          })
, mPropertyDriverPanel(juce::translate("Audio Device Panel..."), juce::translate("Show audio device panel"), []()
                       {
#if JUCE_MAC
                           juce::File("/System/Applications/Utilities/Audio MIDI Setup.app").startAsProcess();
#else
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
#endif
                       })
{
    addAndMakeVisible(mPropertyDriver);
    addAndMakeVisible(mPropertyOutputDevice);
    addAndMakeVisible(mPropertySampleRate);
#if JUCE_MAC
    addAndMakeVisible(mPropertyBufferSize);
#else
    addAndMakeVisible(mPropertyBufferSizeNumber);
#endif
    addAndMakeVisible(mPropertyChannelRouting);
    addAndMakeVisible(mPropertyDriverPanel);

    setSize(300, 200);

    mListener.onAttrChanged = [this](Accessor const& accessor, AttrType attr)
    {
        switch(attr)
        {
            case AttrType::windowState:
            case AttrType::recentlyOpenedFilesList:
            case AttrType::currentDocumentFile:
            case AttrType::defaultTemplateFile:
            case AttrType::currentTranslationFile:
            case AttrType::colourMode:
            case AttrType::showInfoBubble:
            case AttrType::exportOptions:
            case AttrType::adaptationToSampleRate:
            case AttrType::desktopGlobalScaleFactor:
            case AttrType::autoLoadConvertedFile:
            case AttrType::silentFileManagement:
            case AttrType::autoUpdate:
            case AttrType::lastVersion:
            case AttrType::timeZoomAnchorOnPlayhead:
            case AttrType::globalGraphicPreset:
                break;
            case AttrType::routingMatrix:
            {
                auto& entry = mPropertyChannelRouting.entry;
                auto const size = entry.getSize();
                auto const& routing = accessor.getAttr<AttrType::routingMatrix>();
                for(auto input = 0_z; input < routing.size(); ++input)
                {
                    if(input < std::get<0_z>(size))
                    {
                        auto const& row = routing.at(input);
                        for(auto output = 0_z; output < row.size(); ++output)
                        {
                            if(output < std::get<1_z>(size))
                            {
                                entry.setCellState(input, output, row.at(output), juce::NotificationType::dontSendNotification);
                            }
                        }
                    }
                }
            }
            break;
        }
    };

    mDocumentListener.onAttrChanged = [this]([[maybe_unused]] Document::Accessor const& accessor, Document::AttrType attr)
    {
        switch(attr)
        {
            case Document::AttrType::reader:
            {
                changeListenerCallback(&Instance::get().getAudioDeviceManager());
            }
            break;
            case Document::AttrType::layout:
            case Document::AttrType::viewport:
            case Document::AttrType::path:
            case Document::AttrType::grid:
            case Document::AttrType::autoresize:
            case Document::AttrType::samplerate:
            case Document::AttrType::channels:
            case Document::AttrType::editMode:
            case Document::AttrType::drawingState:
            case Document::AttrType::description:
                break;
        }
    };

    Instance::get().getAudioDeviceManager().addChangeListener(this);
    Instance::get().getApplicationAccessor().addListener(mListener, NotificationType::synchronous);
    Instance::get().getDocumentAccessor().addListener(mDocumentListener, NotificationType::synchronous);
}

Application::AudioSettingsContent::~AudioSettingsContent()
{
    Instance::get().getDocumentAccessor().removeListener(mDocumentListener);
    Instance::get().getApplicationAccessor().removeListener(mListener);
    Instance::get().getAudioDeviceManager().removeChangeListener(this);
}

void Application::AudioSettingsContent::resized()
{
    auto const routingHeight = std::min(mPropertyChannelRouting.entry.getOptimalHeight() + 24, 120);
    mPropertyChannelRouting.setSize(getWidth(), routingHeight);
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto const setBounds = [&](juce::Component& component)
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
    setBounds(mPropertyChannelRouting);
    setBounds(mPropertyDriverPanel);
    setSize(bounds.getWidth(), bounds.getY() + 2);
}

void Application::AudioSettingsContent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    auto& deviceManager = Instance::get().getAudioDeviceManager();
    anlWeakAssert(source == std::addressof(deviceManager));
    if(source != std::addressof(deviceManager))
    {
        return;
    }

    mPropertyDriver.entry.clear(juce::NotificationType::dontSendNotification);
    mPropertyOutputDevice.entry.clear(juce::NotificationType::dontSendNotification);
    mPropertySampleRate.entry.clear(juce::NotificationType::dontSendNotification);
    mPropertyBufferSize.entry.clear(juce::NotificationType::dontSendNotification);

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
            mPropertyDriver.entry.setSelectedItemIndex(i, juce::NotificationType::dontSendNotification);
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
            if(currentAudioDevice != nullptr && outputDeviceNames[i] == currentAudioDevice->getName())
            {
                mPropertyOutputDevice.entry.setSelectedItemIndex(i, juce::NotificationType::dontSendNotification);
            }
        }
        mPropertyOutputDevice.entry.addItem(juce::translate("None"), -1);
    }

    // Update sample rates and buffer sizes lists
    mPropertySampleRate.setEnabled(currentAudioDevice != nullptr);
    mPropertyBufferSize.setEnabled(currentAudioDevice != nullptr);
    mPropertyBufferSizeNumber.setEnabled(currentAudioDevice != nullptr);
#ifndef JUCE_MAC
    mPropertyDriverPanel.setEnabled(currentAudioDevice != nullptr);
#endif

    mPropertyChannelRouting.setEnabled(currentAudioDevice != nullptr);
    mPropertyDriverPanel.setEnabled(currentAudioDevice != nullptr);
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
        mPropertySampleRate.entry.setSelectedId(static_cast<int>(currentSampleRate), juce::NotificationType::dontSendNotification);
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
        mPropertyBufferSize.entry.setSelectedId(currentBufferSize, juce::NotificationType::dontSendNotification);
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

#ifndef JUCE_MAC
        mPropertyDriverPanel.setVisible(currentAudioDevice->hasControlPanel());
#endif
        auto const setup = deviceManager.getAudioDeviceSetup();
        auto const documentChannels = Instance::get().getDocumentAccessor().getAttr<Document::AttrType::channels>();
        auto const numInputs = documentChannels > 0_z ? documentChannels : 2_z;
        auto const numOutputs = static_cast<size_t>(std::max(setup.outputChannels.countNumberOfSetBits(), 0));

        MiscDebug("Application::AudioSettingsContent", "Inputs: " + juce::String(numInputs) + " Outputs: " + juce::String(numOutputs));
        mPropertyChannelRouting.entry.setSize(numInputs, numOutputs);
        mPropertyDriverPanel.setEnabled(numInputs > 0_z && numOutputs > 0_z);

        auto& accessor = Instance::get().getApplicationAccessor();
        auto routing = accessor.getAttr<AttrType::routingMatrix>();

        if(numInputs > 0_z && numOutputs > 0_z && (routing.size() != numInputs || routing.at(0_z).size() != numOutputs))
        {
            if(routing.size() == 1_z && routing.at(0_z).size() > 1_z)
            {
                std::fill(routing[0_z].begin() + 1l, routing[0_z].end(), false);
            }
            routing.resize(numInputs);
            if(numInputs == 1_z)
            {
                routing[0_z].resize(numOutputs, false);
                routing[0_z][0_z] = true;
                if(numOutputs > 1_z)
                {
                    routing[0_z][1_z] = true;
                }
            }
            else
            {
                for(auto input = 0_z; input < routing.size(); ++input)
                {
                    auto& ouputs = routing[input];
                    auto const active = (ouputs.size() < numOutputs && input < numOutputs && std::count(ouputs.cbegin(), ouputs.cend(), true) == 0l) || (input < ouputs.size() && ouputs.at(input));
                    ouputs.resize(numOutputs, false);
                    ouputs[input] = active;
                }
            }
            accessor.setAttr<AttrType::routingMatrix>(routing, NotificationType::synchronous);
        }

        auto& entry = mPropertyChannelRouting.entry;
        auto const size = entry.getSize();
        for(auto input = 0_z; input < routing.size(); ++input)
        {
            if(input < std::get<0_z>(size))
            {
                auto const& row = routing.at(input);
                for(auto output = 0_z; output < row.size(); ++output)
                {
                    if(output < std::get<1_z>(size))
                    {
                        entry.setCellState(input, output, row.at(output), juce::NotificationType::dontSendNotification);
                    }
                }
            }
        }
    }
    else
    {
        mPropertyChannelRouting.entry.setSize(2_z, 2_z);
    }
    resized();
}

Application::AudioSettingsPanel::AudioSettingsPanel()
: HideablePanelTyped<AudioSettingsContent>(juce::translate("Audio Settings"))
{
}

ANALYSE_FILE_END
