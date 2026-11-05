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
    // The bin names are the center frequencies of the bins, from 0 Hz to the Nyquist frequency inclusive
    auto const binWidth = getInputSampleRate() / (static_cast<double>(d.binCount - 1_z) * 2.0);
    d.binNames.resize(d.binCount);
    for(auto i = 0_z; i < d.binNames.size(); ++i)
    {
        d.binNames[i] = toString(static_cast<double>(i) * binWidth) + "Hz";
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

AnlVampPlugin::NoteAggregator::NoteAggregator(float inputSampleRate)
: Base(inputSampleRate)
{
}

std::string AnlVampPlugin::NoteAggregator::getIdentifier() const
{
    return "noteaggregator";
}

std::string AnlVampPlugin::NoteAggregator::getName() const
{
    return "Aggregator";
}

std::string AnlVampPlugin::NoteAggregator::getDescription() const
{
    return "The plugin segments frequency points given a set of markers to estimate musical notes.";
}

size_t AnlVampPlugin::NoteAggregator::getPreferredBlockSize() const
{
    return 64;
}

Vamp::Plugin::InputDomain AnlVampPlugin::NoteAggregator::getInputDomain() const
{
    return TimeDomain;
}

bool AnlVampPlugin::NoteAggregator::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    // Partiels centers the blocks on their timestamps by padding the audio stream with half a block at the
    // beginning and at the end, so the last timestamp plus the step size exceeds the end of the audio stream
    // by half a block. The remaining error is less than the step size.
    auto const sampleRate = static_cast<unsigned int>(std::round(getInputSampleRate()));
    auto const stepDuration = Vamp::RealTime::frame2RealTime(static_cast<long>(stepSize), sampleRate);
    auto const paddingDuration = Vamp::RealTime::frame2RealTime(static_cast<long>((blockSize + 1_z) / 2_z), sampleRate);
    mEndOffset = stepDuration - paddingDuration;
    mEndTime.reset();
    return channels == 1;
}

void AnlVampPlugin::NoteAggregator::reset()
{
    mEndTime.reset();
}

Vamp::Plugin::OutputList AnlVampPlugin::NoteAggregator::getOutputDescriptors() const
{
    OutputDescriptor d;
    d.identifier = "notes";
    d.name = "Notes";
    d.description = "The notes generated using the input tracks";
    d.unit = "Hz";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = true;
    d.minValue = 0.0f;
    d.maxValue = getInputSampleRate() / 2.0f;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::SampleType::VariableSampleRate;
    d.sampleRate = 0.0f;
    d.hasDuration = true;
    return {d};
}

Vamp::Plugin::FeatureSet AnlVampPlugin::NoteAggregator::process(const float* const*, Vamp::RealTime timestamp)
{
    // The audio is not analysed, the blocks are only used to estimate the end of the audio stream.
    mEndTime = timestamp + mEndOffset;
    return {};
}

Vamp::Plugin::FeatureSet AnlVampPlugin::NoteAggregator::getRemainingFeatures()
{
    static auto const maxTime = Vamp::RealTime(1000000000, 0);
    auto const endTime = mEndTime.value_or(maxTime);

    // Returns the median of the voiced frequencies (strictly positive values) in [start, end).
    auto const getFrequency = [this](Vamp::RealTime const& start, Vamp::RealTime const& end) -> std::optional<float>
    {
        auto const compare = [](Feature const& feature, Vamp::RealTime const& time)
        {
            return feature.timestamp < time;
        };
        auto const first = std::lower_bound(mPoints.cbegin(), mPoints.cend(), start, compare);
        auto const last = std::lower_bound(first, mPoints.cend(), end, compare);

        std::vector<float> frequencies;
        frequencies.reserve(static_cast<size_t>(std::distance(first, last)));
        for(auto it = first; it != last; ++it)
        {
            if(!it->values.empty() && std::isfinite(it->values.front()) && it->values.front() > 0.0f)
            {
                frequencies.push_back(it->values.front());
            }
        }
        if(frequencies.empty())
        {
            return {};
        }

        auto const middle = std::next(frequencies.begin(), static_cast<std::ptrdiff_t>(frequencies.size() / 2_z));
        std::nth_element(frequencies.begin(), middle, frequencies.end());
        if(frequencies.size() % 2_z == 1_z)
        {
            return *middle;
        }
        // Geometric mean of the two central values (mean in the log-frequency domain)
        auto const lower = *std::max_element(frequencies.begin(), middle);
        return std::sqrt(lower * *middle);
    };

    FeatureList notes;
    auto const addNote = [&](Vamp::RealTime const& start, Vamp::RealTime const& end)
    {
        if(start >= endTime)
        {
            return;
        }
        auto const noteEnd = std::clamp(end, start, endTime);
        Feature note;
        note.hasTimestamp = true;
        note.timestamp = start;
        note.hasDuration = true;
        note.duration = noteEnd - start;
        // A segment without voiced frequency produces a note without value that cuts the segment
        if(auto const frequency = getFrequency(start, noteEnd))
        {
            note.values.push_back(*frequency);
        }
        notes.push_back(std::move(note));
    };

    if(!mHasMarkers)
    {
        addNote(Vamp::RealTime::zeroTime, endTime);
    }
    else
    {
        for(auto index = 0_z; index < mMarkers.size(); ++index)
        {
            auto const& marker = mMarkers.at(index);
            auto const nextTime = index + 1_z < mMarkers.size() ? mMarkers.at(index + 1_z).timestamp : endTime;
            addNote(marker.timestamp, mUseMarkersDurations ? marker.timestamp + marker.duration : nextTime);
        }
    }
    return {{0, std::move(notes)}};
}

Vamp::Plugin::ParameterList AnlVampPlugin::NoteAggregator::getParameterDescriptors() const
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

float AnlVampPlugin::NoteAggregator::getParameter(std::string paramid) const
{
    if(paramid == "useduration")
    {
        return mUseMarkersDurations ? 1.0f : 0.0f;
    }
    std::cerr << "Invalid parameter : " << paramid << "\n";
    return 0.0f;
}

void AnlVampPlugin::NoteAggregator::setParameter(std::string paramid, float newval)
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

Ive::PluginExtension::InputList AnlVampPlugin::NoteAggregator::getInputDescriptors() const
{
    InputList list;
    {
        OutputDescriptor d;
        d.identifier = "frequencies";
        d.name = "Frequencies";
        d.description = "The frequencies points to convert into notes";
        d.unit = "Hz";
        d.hasFixedBinCount = true;
        d.binCount = 1;
        d.hasKnownExtents = true;
        d.minValue = 0.0f;
        d.maxValue = getInputSampleRate() / 2.0f;
        d.isQuantized = false;
        d.sampleType = OutputDescriptor::SampleType::VariableSampleRate;
        d.hasDuration = false;
        list.push_back(d);
    }
    {
        OutputDescriptor d;
        d.identifier = "transients";
        d.name = "Transients";
        d.description = "The transient markers used to segment the frequency points.";
        d.unit = "";
        d.hasFixedBinCount = true;
        d.binCount = 0;
        d.hasKnownExtents = false;
        d.minValue = 0.0f;
        d.maxValue = 0.0f;
        d.isQuantized = false;
        d.sampleType = OutputDescriptor::SampleType::VariableSampleRate;
        // The host only transmits the durations of the markers if the input expects them (used by the useduration parameter)
        d.hasDuration = true;
        list.push_back(d);
    }
    return list;
}

void AnlVampPlugin::NoteAggregator::setPreComputingFeatures(FeatureSet const& fs)
{
    // The inputs are only stored here, the notes are computed in getRemainingFeatures()
    // so the result doesn't depend on the order of the calls and the end of the audio is known.
    auto const sortByTime = [](FeatureList& list)
    {
        std::stable_sort(list.begin(), list.end(), [](Feature const& lhs, Feature const& rhs)
                         {
                             return lhs.timestamp < rhs.timestamp;
                         });
    };

    auto const pointIt = fs.find(0);
    mPoints = pointIt != fs.cend() ? pointIt->second : FeatureList{};
    sortByTime(mPoints);

    auto const markerIt = fs.find(1);
    mHasMarkers = markerIt != fs.cend();
    mMarkers = mHasMarkers ? markerIt->second : FeatureList{};
    sortByTime(mMarkers);
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
                static Vamp::PluginAdapter<AnlVampPlugin::NoteAggregator> adaptater;
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
                return Ive::PluginAdapter::getDescriptor<AnlVampPlugin::NoteAggregator>();
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
