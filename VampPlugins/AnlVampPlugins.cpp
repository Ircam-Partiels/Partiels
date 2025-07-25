#include "AnlVampPlugins.h"
#include <IvePluginAdapter.hpp>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <random>
#include <sstream>
#include <vamp-sdk/PluginAdapter.h>

AnlVampPlugin::Base::Base(float sampleRate)
: Vamp::Plugin(sampleRate)
{
}

std::string AnlVampPlugin::Base::getMaker() const
{
    return "Factory";
}

int AnlVampPlugin::Base::getPluginVersion() const
{
    return PARTIELS_VAMP_PLUGINS_VERSION;
}

std::string AnlVampPlugin::Base::getCopyright() const
{
    return "Partiels factory plugin.";
}

void AnlVampPlugin::Base::reset()
{
}

Vamp::Plugin::FeatureSet AnlVampPlugin::Base::getRemainingFeatures()
{
    return {};
}

AnlVampPlugin::Waveform::Waveform(float sampleRate)
: Base(sampleRate)
{
}

size_t AnlVampPlugin::Waveform::getPreferredBlockSize() const
{
    return 1024_z;
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

Vamp::Plugin::OutputList AnlVampPlugin::Waveform::getOutputDescriptors() const
{
    OutputDescriptor d;
    d.identifier = "peaks";
    d.name = "Peak";
    d.description = "Peak from the input signal";
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

Vamp::Plugin::FeatureSet AnlVampPlugin::Waveform::process(const float* const*, Vamp::RealTime)
{
    // The process should be managed by Partiels directly. The method is here for legacy reason.
    return {};
    //    if(mBlockSize == 0 || mStepSize == 0)
    //    {
    //        std::cerr << "AnlVampPlugin::Waveform::process invalid parameters.\n";
    //        return {};
    //    }
    //
    //    FeatureSet featureSet;
    //    auto& featureList = featureSet[0];
    //    featureList.resize(mNumChannels);
    //    for(auto channel = 0_z; channel < mNumChannels; ++channel)
    //    {
    //        auto& feature = featureList[channel];
    //        feature.timestamp = timeStamp;
    //        feature.hasTimestamp = true;
    //        auto it = std::max_element(inputBuffers[channel], inputBuffers[channel] + mBlockSize, [](auto const& lhs, auto const& rhs)
    //                                   {
    //                                       return std::abs(lhs) < std::abs(rhs);
    //                                   });
    //        feature.values.push_back(it != inputBuffers[channel] + mBlockSize ? std::clamp(*it, -1.0f, 1.0f) : 0.0f);
    //    }
    //
    //    return featureSet;
}

AnlVampPlugin::Spectrogram::Spectrogram(float sampleRate)
: Base(sampleRate)
{
}

bool AnlVampPlugin::Spectrogram::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    mNumChannels = channels;
    mStepSize = stepSize;
    mBlockSize = blockSize;
    return true;
}

Vamp::Plugin::InputDomain AnlVampPlugin::Spectrogram::getInputDomain() const
{
    return FrequencyDomain;
}

std::string AnlVampPlugin::Spectrogram::getIdentifier() const
{
    return "partielsspectrogram";
}

std::string AnlVampPlugin::Spectrogram::getName() const
{
    return "Spectrogram";
}

std::string AnlVampPlugin::Spectrogram::getDescription() const
{
    return "Display the spectrogram of the audio.";
}

Vamp::Plugin::OutputList AnlVampPlugin::Spectrogram::getOutputDescriptors() const
{
    OutputDescriptor d;
    d.identifier = "energies";
    d.name = "Energy";
    d.description = "Energy from the input signal";
    d.unit = "dB";
    d.hasFixedBinCount = true;
    d.binCount = mBlockSize == 0_z ? 513_z : mBlockSize / 2_z + 1_z;
    d.hasKnownExtents = true;
    d.minValue = -120.0f;
    d.maxValue = 12.0f;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::SampleType::OneSamplePerStep;
    d.hasDuration = false;
    auto const toString = [](double value)
    {
        std::ostringstream out;
        out.precision(1);
        out << std::fixed << value;
        return out.str();
    };
    auto const binWidth = getInputSampleRate() / (static_cast<double>(d.binCount) * 2.0);
    d.binNames.resize(d.binCount);
    for(auto i = 0_z; i < d.binNames.size(); ++i)
    {
        d.binNames[i] = toString((static_cast<double>(i) + 1.0) * binWidth) + "Hz";
    }
    return {d};
}

Vamp::Plugin::FeatureSet AnlVampPlugin::Spectrogram::process(const float* const* inputBuffers, Vamp::RealTime)
{
    if(mBlockSize == 0 || mStepSize == 0)
    {
        std::cerr << "AnlVampPlugin::Waveform::process invalid parameters.\n";
        return {};
    }

    FeatureSet fs;
    for(auto channel = 0_z; channel < mNumChannels; ++channel)
    {
        Feature feature;
        feature.hasTimestamp = false;
        feature.values.resize(mBlockSize / 2_z + 1_z);
        const float* inputBuffer = inputBuffers[channel];
        for(auto& value : feature.values)
        {
            auto const real = *inputBuffer++;
            auto const imag = *inputBuffer++;
            value = std::clamp(std::log10(real * real + imag * imag) * 20.0f, -120.0f, 12.0f);
        }
        fs[0].push_back(std::move(feature));
    }
    return fs;
}

AnlVampPlugin::NewTrack::NewTrack(float sampleRate)
: Base(sampleRate)
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

size_t AnlVampPlugin::NewTrack::getPreferredBlockSize() const
{
    return 256_z;
}

Vamp::Plugin::OutputList AnlVampPlugin::NewTrack::getOutputDescriptors() const
{
    OutputList list;
    {
        OutputDescriptor d;
        d.identifier = "markers";
        d.name = "Marker";
        d.description = "Marker";
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
        d.name = "Point";
        d.description = "Point";
        d.unit = "";
        d.hasFixedBinCount = true;
        d.binCount = 1;
        d.hasKnownExtents = true;
        d.minValue = 0.0f;
        d.maxValue = 1.0f;
        d.isQuantized = false;
        d.sampleType = OutputDescriptor::SampleType::VariableSampleRate;
        d.hasDuration = false;
        list.push_back(std::move(d));
    }
    return list;
}

Vamp::Plugin::FeatureSet AnlVampPlugin::NewTrack::process(const float* const*, Vamp::RealTime)
{
    return {};
}

Vamp::Plugin::FeatureSet AnlVampPlugin::NewTrack::getRemainingFeatures()
{
    FeatureSet featureSet;
    featureSet[0].resize(mNumChannels);
    for(auto& feature : featureSet[0])
    {
        feature.hasTimestamp = true;
        feature.timestamp = Vamp::RealTime();
    }
    featureSet[1].resize(mNumChannels);
    for(auto& feature : featureSet[1])
    {
        feature.hasTimestamp = true;
        feature.timestamp = Vamp::RealTime();
    }
    return featureSet;
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
                static Vamp::PluginAdapter<AnlVampPlugin::Spectrogram> adaptater;
                return adaptater.getDescriptor();
            }
            case 2:
            {
                static Vamp::PluginAdapter<AnlVampPlugin::NewTrack> adaptater;
                return adaptater.getDescriptor();
            }
            default:
            {
                return nullptr;
            }
        }
    }

    IVE_EXTERN IvePluginDescriptor const* iveGetPluginDescriptor(unsigned int version, unsigned int)
    {
        if(version < 2)
        {
            return nullptr;
        }
        return nullptr;
    }
#ifdef __cplusplus
}
#endif
