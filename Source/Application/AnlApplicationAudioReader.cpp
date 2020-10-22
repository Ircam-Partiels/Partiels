
#include "AnlApplicationAudioReader.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::AudioReader::AudioReader()
{
    mAudioSourcePlayer.setSource(this);
    Instance::get().getAudioDeviceManager().addAudioCallback(&mAudioSourcePlayer);
}

Application::AudioReader::~AudioReader()
{
    Instance::get().getAudioDeviceManager().removeAudioCallback(&mAudioSourcePlayer);
    mAudioSourcePlayer.setSource(nullptr);
    Instance::get().getAudioDeviceManager().closeAudioDevice();
}

void Application::AudioReader::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    auto* interface = Instance::get().getInsterface();
    if(interface != nullptr)
    {
        interface->getDocumentAudioReader().prepareToPlay(samplesPerBlockExpected, sampleRate);
    }
}

void Application::AudioReader::releaseResources()
{
    auto* interface = Instance::get().getInsterface();
    if(interface != nullptr)
    {
        interface->getDocumentAudioReader().releaseResources();
    }
}

void Application::AudioReader::getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill)
{
    auto* interface = Instance::get().getInsterface();
    if(interface != nullptr)
    {
        interface->getDocumentAudioReader().getNextAudioBlock(bufferToFill);
    }
}

ANALYSE_FILE_END
