#include "AnlApplicationAudioReader.h"
#include "AnlApplicationInstance.h"


ANALYSE_FILE_BEGIN

Application::AudioReader::AudioReader()
: mDocumentAudioReader(Instance::get().getDocumentAccessor(), Instance::get().getAudioFormatManager())
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
    mDocumentAudioReader.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void Application::AudioReader::releaseResources()
{
    mDocumentAudioReader.releaseResources();
}

void Application::AudioReader::getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill)
{
    mDocumentAudioReader.getNextAudioBlock(bufferToFill);
}

ANALYSE_FILE_END
