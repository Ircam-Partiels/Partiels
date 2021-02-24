#include "AnlApplicationAudioReader.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::AudioReader::AudioReader()
: mDocumentAudioReader(Instance::get().getDocumentAccessor(), Instance::get().getAudioFormatManager())
, mResamplingAudioSource(&mDocumentAudioReader, false)
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
    anlWeakAssert(sampleRate > 0.0);
    sampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    
    auto sourceSampleRate = mDocumentAudioReader.getSampleRate();
    anlWeakAssert(sourceSampleRate > 0.0);
    sourceSampleRate = sourceSampleRate > 0.0 ? sourceSampleRate : 44100.0;
    
    mResamplingAudioSource.setResamplingRatio(sourceSampleRate / sampleRate);
    mResamplingAudioSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void Application::AudioReader::releaseResources()
{
    mResamplingAudioSource.releaseResources();
}

void Application::AudioReader::getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill)
{
    mResamplingAudioSource.getNextAudioBlock(bufferToFill);
}

ANALYSE_FILE_END
