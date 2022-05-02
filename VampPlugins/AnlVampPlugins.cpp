#include "AnlVampPlugins.h"
#include <algorithm>
#include <vamp-sdk/PluginAdapter.h>

AnlVampPlugin::Waveform::Waveform(float sampleRate)
: Vamp::Plugin(sampleRate)
{
}

bool AnlVampPlugin::Waveform::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    mNumChannels = channels;
    mStepSize = stepSize;
    mBlockSize = blockSize;
    return true;
}

Vamp::Plugin::InputDomain AnlVampPlugin::Waveform::getInputDomain() const
{
    return TimeDomain;
}

std::string AnlVampPlugin::Waveform::getIdentifier() const
{
    return "partielswaveform";
}

std::string AnlVampPlugin::Waveform::getName() const
{
    return "Waveform";
}

std::string AnlVampPlugin::Waveform::getDescription() const
{
    return "Display the waveform of the audio.";
}
std::string AnlVampPlugin::Waveform::getMaker() const
{
    return "Ircam";
}

int AnlVampPlugin::Waveform::getPluginVersion() const
{
    return PARTIELS_VAMP_PLUGINS_VERSION;
}

std::string AnlVampPlugin::Waveform::getCopyright() const
{
    return "Copyright 2022 Ircam. All rights reserved. Plugin by Pierre Guillot.";
}

size_t AnlVampPlugin::Waveform::getPreferredBlockSize() const
{
    return 0_z;
}

size_t AnlVampPlugin::Waveform::getPreferredStepSize() const
{
    return 0_z;
}

Vamp::Plugin::OutputList AnlVampPlugin::Waveform::getOutputDescriptors() const
{
    OutputDescriptor d;
    d.identifier = "peaks";
    d.name = "Peaks";
    d.description = "Peaks from the input signal";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = true;
    d.minValue = -1.0f;
    d.maxValue = 1.0f;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::SampleType::FixedSampleRate;
    d.sampleRate = static_cast<float>(mStepSize) / getInputSampleRate();
    d.hasDuration = false;
    return {d};
}

void AnlVampPlugin::Waveform::reset()
{
}

Vamp::Plugin::FeatureSet AnlVampPlugin::Waveform::process(const float* const* inputBuffers, Vamp::RealTime timeStamp)
{
    if(mBlockSize == 0 || mStepSize == 0)
    {
        std::cerr << "AnlVampPlugin::Waveform::process invalid parameters.\n";
        return {};
    }

    FeatureSet featureSet;
    auto& featureList = featureSet[0];
    featureList.resize(mNumChannels);
    for(auto channel = 0_z; channel < mNumChannels; ++channel)
    {
        auto& feature = featureList[channel];
        feature.timestamp = timeStamp;
        feature.hasTimestamp = true;
        auto it = std::max_element(inputBuffers[channel], inputBuffers[channel] + mBlockSize, [](auto const& lhs, auto const& rhs)
                                   {
                                       return std::abs(lhs) < std::abs(rhs);
                                   });
        feature.values.push_back(it != inputBuffers[channel] + mBlockSize ? std::clamp(*it, -1.0f, 1.0f) : 0.0f);
    }

    return featureSet;
}

Vamp::Plugin::FeatureSet AnlVampPlugin::Waveform::getRemainingFeatures()
{
    return {};
}

#ifdef __cplusplus
extern "C"
{
#endif
    VampPluginDescriptor const* vampGetPluginDescriptor(unsigned int version, unsigned int index)
    {
        if(version < 1)
        {
            return nullptr;
        }
        switch(index)
        {
            case 0:
            {
                static Vamp::PluginAdapter<AnlVampPlugin::Waveform> adaptater;
                return adaptater.getDescriptor();
            }
            default:
            {
                return nullptr;
            }
        }
    }

#ifdef __cplusplus
}
#endif
