#include "AnlApplicationAudioReader.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::AudioReader::AudioReader()
: mDocumentAudioReader(Instance::get().getDocumentAccessor(), Instance::get().getAudioFormatManager())
{
    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& accessor, AttrType attr)
    {
        switch(attr)
        {
            case AttrType::windowState:
            case AttrType::recentlyOpenedFilesList:
            case AttrType::currentDocumentFile:
            case AttrType::colourMode:
            case AttrType::showInfoBubble:
            case AttrType::exportOptions:
            case AttrType::adaptationToSampleRate:
            case AttrType::desktopGlobalScaleFactor:
            case AttrType::autoLoadConvertedFile:
                break;
            case AttrType::routingMatrix:
            {
                updateBuffer();
            }
            break;
        }
    };
    Instance::get().getApplicationAccessor().addListener(mListener, NotificationType::synchronous);

    mAudioSourcePlayer.setSource(this);
    auto& audioDeviceManager = Instance::get().getAudioDeviceManager();
    audioDeviceManager.addAudioCallback(&mAudioSourcePlayer);
}

Application::AudioReader::~AudioReader()
{
    auto& audioDeviceManager = Instance::get().getAudioDeviceManager();
    audioDeviceManager.removeAudioCallback(&mAudioSourcePlayer);
    audioDeviceManager.closeAudioDevice();
    mAudioSourcePlayer.setSource(nullptr);

    Instance::get().getApplicationAccessor().removeListener(mListener);
}

void Application::AudioReader::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    mDocumentAudioReader.prepareToPlay(samplesPerBlockExpected, sampleRate);
    mBlockSize = samplesPerBlockExpected;
    updateBuffer();
}

void Application::AudioReader::releaseResources()
{
    mBlockSize = 0;
    mDocumentAudioReader.releaseResources();
}

void Application::AudioReader::getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill)
{
    if(bufferToFill.buffer == nullptr)
    {
        return;
    }
    bufferToFill.clearActiveBufferRegion();
    auto const numOutputChannels = bufferToFill.buffer->getNumChannels();
    if(numOutputChannels <= 0)
    {
        return;
    }

    std::unique_lock<audio_spin_mutex> lock(mMutex, std::try_to_lock);
    if(!lock.owns_lock())
    {
        return;
    }

    auto numRemainingSamples = bufferToFill.numSamples;
    auto outputPosition = bufferToFill.startSample;
    auto const bufferNumChannels = mBuffer.getNumChannels();
    auto const bufferNumSamples = mBuffer.getNumSamples();

    MiscWeakAssert(bufferNumChannels == static_cast<int>(mMatrix.size()));
    if(bufferNumSamples <= 0 || bufferNumChannels <= 0)
    {
        return;
    }

    MiscWeakAssert(bufferNumChannels == static_cast<int>(mMatrix.size()));
    if(bufferNumChannels != static_cast<int>(mMatrix.size()))
    {
        return;
    }

    MiscWeakAssert(numOutputChannels == static_cast<int>(mMatrix.at(0_z).size()));
    if(numOutputChannels != static_cast<int>(mMatrix.at(0_z).size()))
    {
        return;
    }

    while(numRemainingSamples > 0)
    {
        juce::AudioSourceChannelInfo temp(&mBuffer, 0, std::min(bufferNumSamples, numRemainingSamples));
        mDocumentAudioReader.getNextAudioBlock(temp);
        for(auto input = 0; input < bufferNumChannels; ++input)
        {
            auto const& routing = mMatrix.at(static_cast<size_t>(input));
            auto const numRoutingOutputs = static_cast<int>(routing.size());
            MiscWeakAssert(numOutputChannels == numRoutingOutputs);
            for(auto output = 0; output < numOutputChannels; ++output)
            {
                if(output < numRoutingOutputs && routing.at(static_cast<size_t>(output)))
                {
                    bufferToFill.buffer->addFrom(output, outputPosition, mBuffer, input, 0, temp.numSamples);
                }
            }
        }
        numRemainingSamples -= temp.numSamples;
        outputPosition += temp.numSamples;
    }
}

void Application::AudioReader::updateBuffer()
{
    auto const& accessor = Instance::get().getApplicationAccessor();
    auto const& routing = accessor.getAttr<AttrType::routingMatrix>();
    if(mBlockSize <= 0)
    {
        return;
    }

    std::unique_lock<audio_spin_mutex> lock(mMutex);
    if(!lock.owns_lock())
    {
        return;
    }
    mMatrix = routing;
    mBuffer.setSize(static_cast<int>(routing.size()), mBlockSize);
}

ANALYSE_FILE_END
