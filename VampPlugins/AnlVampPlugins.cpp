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
            auto const amp = std::sqrt(real * real + imag * imag);
            value = std::clamp(std::log10(amp) * 20.0f, -120.0f, 12.0f);
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

/// Marker Stats

AnlVampPlugin::Stats::Stats(float inputSampleRate)
: Base(inputSampleRate)
{
}

bool AnlVampPlugin::Stats::initialise(size_t channels, size_t, size_t)
{
    return channels == 1;
}

Vamp::Plugin::InputDomain AnlVampPlugin::Stats::getInputDomain() const
{
    return TimeDomain;
}

std::string AnlVampPlugin::Stats::getIdentifier() const
{
    return "transformermarker";
}

std::string AnlVampPlugin::Stats::getName() const
{
    return "Combine";
}

std::string AnlVampPlugin::Stats::getDescription() const
{
    return "The plugin generates the statistics of an input point track for each time segment given by an input marker track.";
}

Vamp::Plugin::OutputList AnlVampPlugin::Stats::getOutputDescriptors() const
{
    OutputDescriptor d;
    d.identifier = "result";
    d.name = "Statistics";
    d.description = "The statistics of the input point track";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.minValue = -1.0f;
    d.maxValue = 1.0f;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::SampleType::VariableSampleRate;
    d.sampleRate = 0.0f;
    d.hasDuration = true;
    return {d};
}

Ive::PluginExtension::OutputExtraList AnlVampPlugin::Stats::getOutputExtraDescriptors(size_t outputDescriptorIndex) const
{
    OutputExtraList list;
    if(outputDescriptorIndex != 0)
    {
        return list;
    }
    {
        OutputExtraDescriptor d;
        d.identifier = "min";
        d.name = "Min.";
        d.description = "The minimum value of the input points";
        d.unit = "";
        d.hasKnownExtents = false;
        d.minValue = 0.0f;
        d.maxValue = 1.0f;
        d.isQuantized = false;
        d.quantizeStep = 0.0f;
        list.push_back(std::move(d));
    }
    {
        OutputExtraDescriptor d;
        d.identifier = "max";
        d.name = "Max.";
        d.description = "The maximum value of the input points";
        d.unit = "";
        d.hasKnownExtents = false;
        d.minValue = 0.0f;
        d.maxValue = 1.0f;
        d.isQuantized = false;
        d.quantizeStep = 0.0f;
        list.push_back(std::move(d));
    }
    {
        OutputExtraDescriptor d;
        d.identifier = "median";
        d.name = "Median.";
        d.description = "The median value of the input points";
        d.unit = "";
        d.hasKnownExtents = false;
        d.minValue = 0.0f;
        d.maxValue = 1.0f;
        d.isQuantized = false;
        d.quantizeStep = 0.0f;
        list.push_back(std::move(d));
    }
    return list;
}

size_t AnlVampPlugin::Stats::getPreferredBlockSize() const
{
    return 64;
}

Vamp::Plugin::FeatureSet AnlVampPlugin::Stats::process(const float* const*, Vamp::RealTime)
{
    return {};
}

Vamp::Plugin::FeatureSet AnlVampPlugin::Stats::getRemainingFeatures()
{
    return {{0, mResults}};
}

Vamp::Plugin::ParameterList AnlVampPlugin::Stats::getParameterDescriptors() const
{
    ParameterList list;
    {
        ParameterDescriptor param;
        param.identifier = "useduration";
        param.name = "Use markers duration";
        param.description = "Toggle the use of markers' durations";
        param.unit = "";
        param.minValue = 0;
        param.maxValue = 1;
        param.defaultValue = 0;
        param.isQuantized = true;
        param.quantizeStep = 1.0f;
        list.push_back(std::move(param));
    }
    return list;
}

float AnlVampPlugin::Stats::getParameter(std::string paramid) const
{
    if(paramid == "useduration")
    {
        return mUseMarkersDurations ? 1.0f : 0.0f;
    }
    std::cerr << "Invalid parameter : " << paramid << "\n";
    return 0.0f;
}

void AnlVampPlugin::Stats::setParameter(std::string paramid, float newval)
{
    if(paramid == "useduration")
    {
        mUseMarkersDurations = newval >= 0.5f ? true : false;
    }
    else
    {
        std::cerr << "Invalid parameter : " << paramid << "\n";
    }
}

Ive::PluginExtension::InputList AnlVampPlugin::Stats::getInputDescriptors() const
{
    InputList list;
    {
        OutputDescriptor d;
        d.identifier = "points";
        d.name = "Points";
        d.description = "The points to analyze by markers";
        d.unit = "";
        d.hasFixedBinCount = true;
        d.binCount = 1;
        d.hasKnownExtents = false;
        d.minValue = 0.0f;
        d.maxValue = 0.0f;
        d.isQuantized = false;
        d.sampleType = OutputDescriptor::SampleType::VariableSampleRate;
        d.hasDuration = false;
        list.push_back(d);
    }
    {
        OutputDescriptor d;
        d.identifier = "markers";
        d.name = "Markers";
        d.description = "The markers used to split the points.";
        d.unit = "";
        d.hasFixedBinCount = true;
        d.binCount = 0;
        d.hasKnownExtents = false;
        d.minValue = 0.0f;
        d.maxValue = 0.0f;
        d.isQuantized = false;
        d.sampleType = OutputDescriptor::SampleType::VariableSampleRate;
        d.hasDuration = false;
        list.push_back(d);
    }
    return list;
}

void AnlVampPlugin::Stats::setPreComputingFeatures(FeatureSet const& fs)
{
    auto const pointFeature = fs.find(0);
    auto const markerFeature = fs.find(1);
    if(pointFeature == fs.cend())
    {
        return;
    }
    auto const& points = pointFeature->second;
    auto const& markers = markerFeature->second;
    auto const useDuration = mUseMarkersDurations;
    auto lastPoint = points.cbegin();
    static auto const maxRt = Vamp::RealTime::fromSeconds(std::numeric_limits<double>::max());

    auto const perform = [&, this](Vamp::RealTime const& start, Vamp::RealTime const& end)
    {
        auto const firstIt = std::lower_bound(lastPoint, points.cend(), start, [](auto const& f, auto const& t)
                                              {
                                                  return f.timestamp < t;
                                              });
        auto const secondIt = std::upper_bound(firstIt, points.cend(), end, [](auto const& t, auto const& f)
                                               {
                                                   return t < f.timestamp;
                                               });

        std::vector<float> sorted;
        float sum = 0.0f;
        sorted.reserve(static_cast<size_t>(std::distance(firstIt, secondIt)));
        for(auto it = firstIt; it < secondIt; ++it)
        {
            if(!it->values.empty())
            {
                sorted.push_back(it->values.at(0));
                sum += it->values.at(0);
            }
        }
        if(!sorted.empty())
        {
            std::sort(sorted.begin(), sorted.end());
            auto const min = sorted.front();
            auto const max = sorted.back();
            auto const mean = sum / static_cast<float>(sorted.size());
            auto const medianIndex = std::min(static_cast<size_t>(std::floor(static_cast<double>(sorted.size()) / 2.0 + 0.5)), sorted.size() - 1);
            auto const median = sorted.at(medianIndex);
            Feature f;
            f.hasTimestamp = true;
            f.timestamp = start;
            f.hasDuration = true;
            f.duration = end - start;
            f.values.push_back(mean);
            f.values.push_back(min);
            f.values.push_back(max);
            f.values.push_back(median);
            mResults.push_back(std::move(f));
        }
        lastPoint = secondIt;
    };
    if(markers.empty())
    {
        perform(Vamp::RealTime(), maxRt);
    }
    else
    {
        for(size_t markerIndex = 0; markerIndex < markers.size() && lastPoint != points.cend(); ++markerIndex)
        {
            auto const& marker = markers.at(markerIndex);
            auto const start = marker.timestamp;
            auto const end = useDuration ? start + marker.duration : (markerIndex + 1 < markers.size() ? markers.at(markerIndex + 1).timestamp : maxRt);
            perform(start, end);
        }
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
                static Vamp::PluginAdapter<AnlVampPlugin::Spectrogram> adaptater;
                return adaptater.getDescriptor();
            }
            case 2:
            {
                static Vamp::PluginAdapter<AnlVampPlugin::NewTrack> adaptater;
                return adaptater.getDescriptor();
            }
            case 3:
            {
                static Vamp::PluginAdapter<AnlVampPlugin::Stats> adaptater;
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
        if(version < 2)
        {
            return nullptr;
        }
        switch(index)
        {
            case 0:
            {
                return Ive::PluginAdapter::getDescriptor<AnlVampPlugin::Stats>();
            }
            default:
            {
                return nullptr;
            }
        }
        return nullptr;
    }
#ifdef __cplusplus
}
#endif
