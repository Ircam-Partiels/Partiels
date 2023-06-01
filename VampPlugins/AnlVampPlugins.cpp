#include "AnlVampPlugins.h"
#include <IvePluginAdapter.hpp>
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

AnlVampPlugin::NewTrack::NewTrack(float sampleRate)
: Vamp::Plugin(sampleRate)
{
}

bool AnlVampPlugin::NewTrack::initialise(size_t channels, size_t, size_t)
{
    mNumChannels = channels;
    return true;
}

Vamp::Plugin::InputDomain AnlVampPlugin::NewTrack::getInputDomain() const
{
    return TimeDomain;
}

std::string AnlVampPlugin::NewTrack::getIdentifier() const
{
    return "partielsnewtrack";
}

std::string AnlVampPlugin::NewTrack::getName() const
{
    return "New Track";
}

std::string AnlVampPlugin::NewTrack::getDescription() const
{
    return "Add an empty track.";
}
std::string AnlVampPlugin::NewTrack::getMaker() const
{
    return "Ircam";
}

int AnlVampPlugin::NewTrack::getPluginVersion() const
{
    return PARTIELS_VAMP_PLUGINS_VERSION;
}

std::string AnlVampPlugin::NewTrack::getCopyright() const
{
    return "Copyright 2022 Ircam. All rights reserved. Plugin by Pierre Guillot.";
}

size_t AnlVampPlugin::NewTrack::getPreferredBlockSize() const
{
    return 256_z;
}

size_t AnlVampPlugin::NewTrack::getPreferredStepSize() const
{
    return 0_z;
}

Vamp::Plugin::OutputList AnlVampPlugin::NewTrack::getOutputDescriptors() const
{
    OutputList list;
    {
        OutputDescriptor d;
        d.identifier = "markers";
        d.name = "Markers";
        d.description = "Markers";
        d.unit = "";
        d.hasFixedBinCount = true;
        d.binCount = 0;
        d.hasKnownExtents = false;
        d.isQuantized = false;
        d.sampleType = OutputDescriptor::SampleType::VariableSampleRate;
        d.hasDuration = false;
        list.push_back(std::move(d));
    }
    {
        OutputDescriptor d;
        d.identifier = "points";
        d.name = "Points";
        d.description = "Points";
        d.unit = "";
        d.hasFixedBinCount = true;
        d.binCount = 1;
        d.hasKnownExtents = false;
        d.isQuantized = false;
        d.sampleType = OutputDescriptor::SampleType::VariableSampleRate;
        d.hasDuration = false;
        list.push_back(std::move(d));
    }
    return list;
}

void AnlVampPlugin::NewTrack::reset()
{
}

Vamp::Plugin::FeatureSet AnlVampPlugin::NewTrack::process(const float* const*, Vamp::RealTime timeStamp)
{
    if(timeStamp == Vamp::RealTime())
    {
        FeatureSet featureSet;
        featureSet[0].resize(mNumChannels);
        for(auto& feature : featureSet[0])
        {
            feature.hasTimestamp = true;
            feature.timestamp = Vamp::RealTime(0, 0);
        }
        featureSet[1].resize(mNumChannels);
        for(auto& feature : featureSet[1])
        {
            feature.hasTimestamp = true;
            feature.timestamp = Vamp::RealTime(0, 0);
        }
        return featureSet;
    }
    return {};
}

Vamp::Plugin::FeatureSet AnlVampPlugin::NewTrack::getRemainingFeatures()
{
    return {};
}

AnlVampPlugin::Dummy::Dummy(float sampleRate)
: Vamp::Plugin(sampleRate)
{
}

bool AnlVampPlugin::Dummy::initialise(size_t, size_t, size_t)
{
    return true;
}

Vamp::Plugin::InputDomain AnlVampPlugin::Dummy::getInputDomain() const
{
    return TimeDomain;
}

std::string AnlVampPlugin::Dummy::getIdentifier() const
{
    return "partielsdummy";
}

std::string AnlVampPlugin::Dummy::getName() const
{
    return "Dummy";
}

std::string AnlVampPlugin::Dummy::getDescription() const
{
    return "Copy an analysis.";
}
std::string AnlVampPlugin::Dummy::getMaker() const
{
    return "Ircam";
}

int AnlVampPlugin::Dummy::getPluginVersion() const
{
    return PARTIELS_VAMP_PLUGINS_VERSION;
}

std::string AnlVampPlugin::Dummy::getCopyright() const
{
    return "Copyright 2023 Ircam. All rights reserved. Plugin by Pierre Guillot.";
}

size_t AnlVampPlugin::Dummy::getPreferredBlockSize() const
{
    return 256_z;
}

size_t AnlVampPlugin::Dummy::getPreferredStepSize() const
{
    return 0_z;
}

Vamp::Plugin::OutputList AnlVampPlugin::Dummy::getOutputDescriptors() const
{
    OutputList list;
    {
        OutputDescriptor d;
        d.identifier = "markers";
        d.name = "Markers";
        d.description = "Markers";
        d.unit = "";
        d.hasFixedBinCount = true;
        d.binCount = 0;
        d.hasKnownExtents = false;
        d.isQuantized = false;
        d.sampleType = OutputDescriptor::SampleType::VariableSampleRate;
        d.hasDuration = false;
        list.push_back(std::move(d));
    }
    return list;
}

Ive::PluginExtension::InputList AnlVampPlugin::Dummy::getInputDescriptors() const
{
    return getOutputDescriptors();
}

void AnlVampPlugin::Dummy::reset()
{
    mFeatureSet.clear();
}

Vamp::Plugin::FeatureSet AnlVampPlugin::Dummy::process(const float* const*, Vamp::RealTime timeStamp)
{
    return {};
}

Vamp::Plugin::FeatureSet AnlVampPlugin::Dummy::getRemainingFeatures()
{
    return mFeatureSet;
}

void AnlVampPlugin::Dummy::setPreComputingFeatures(FeatureSet const& fs)
{
    auto const it = fs.find(0);
    if(it != fs.cend())
    {
        mFeatureSet[0] = it->second;
    }
    else
    {
        mFeatureSet.clear();
    }
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
            case 1:
            {
                static Vamp::PluginAdapter<AnlVampPlugin::NewTrack> adaptater;
                return adaptater.getDescriptor();
            }
            case 2:
            {
                static Vamp::PluginAdapter<AnlVampPlugin::Dummy> adaptater;
                return adaptater.getDescriptor();
            }
            default:
            {
                return nullptr;
            }
        }
    }

    IVE_EXTERN IvePluginDescriptor const* iveGetPluginDescriptor(unsigned int version, unsigned int index)
    {
#if NDEBUG
        return nullptr;
#endif
        if(version < 2)
        {
            return nullptr;
        }
        switch(index)
        {
            case 0:
            {
                return Ive::PluginAdapter::getDescriptor<AnlVampPlugin::Dummy>();
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
