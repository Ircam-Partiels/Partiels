#include "AnlVampPlugins.h"
#include <IvePluginAdapter.hpp>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <numeric>
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

AnlVampPlugin::SelfSimilarity::SelfSimilarity(float inputSampleRate)
: Base(inputSampleRate)
{
}

std::string AnlVampPlugin::SelfSimilarity::getIdentifier() const
{
    return "selfsimilarity";
}

std::string AnlVampPlugin::SelfSimilarity::getName() const
{
    return "Self-Similarity";
}

std::string AnlVampPlugin::SelfSimilarity::getDescription() const
{
    return "The plugin generates the self-similarity matrix of the columns of an input matrix.";
}

size_t AnlVampPlugin::SelfSimilarity::getPreferredBlockSize() const
{
    // The audio is not analysed, a large block size reduces the number of calls to process()
    return 4096_z;
}

Vamp::Plugin::InputDomain AnlVampPlugin::SelfSimilarity::getInputDomain() const
{
    return TimeDomain;
}

bool AnlVampPlugin::SelfSimilarity::initialise(size_t channels, size_t, size_t)
{
    return channels == 1;
}

Vamp::Plugin::OutputList AnlVampPlugin::SelfSimilarity::getOutputDescriptors() const
{
    OutputDescriptor d;
    d.identifier = "matrix";
    d.name = "Matrix";
    d.description = "The self-similarity matrix of the input columns";
    d.unit = "";
    // The number of bins is the number of frames of the matrix that depends on the input
    d.hasFixedBinCount = false;
    d.binCount = 0;
    d.hasKnownExtents = true;
    d.minValue = mMetric == Metric::euclidean ? 0.0f : -1.0f;
    d.maxValue = 1.0f;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::SampleType::VariableSampleRate;
    d.sampleRate = 0.0f;
    d.hasDuration = false;
    return {d};
}

Vamp::Plugin::ParameterList AnlVampPlugin::SelfSimilarity::getParameterDescriptors() const
{
    ParameterList list;
    {
        ParameterDescriptor param;
        param.identifier = "metric";
        param.name = "Metric";
        param.description = "The metric used to compare the columns";
        param.unit = "";
        param.minValue = 0.0f;
        param.maxValue = 2.0f;
        param.defaultValue = 0.0f;
        param.isQuantized = true;
        param.quantizeStep = 1.0f;
        param.valueNames = {"Cosine", "Correlation", "Euclidean"};
        list.push_back(std::move(param));
    }
    {
        ParameterDescriptor param;
        param.identifier = "normalization";
        param.name = "Normalization";
        param.description = "The normalization of each bin over time applied before the comparison of the columns";
        param.unit = "";
        param.minValue = 0.0f;
        param.maxValue = 2.0f;
        param.defaultValue = 1.0f;
        param.isQuantized = true;
        param.quantizeStep = 1.0f;
        param.valueNames = {"None", "Centering", "Standardization"};
        list.push_back(std::move(param));
    }
    {
        ParameterDescriptor param;
        param.identifier = "downsampling";
        param.name = "Downsampling";
        param.description = "The number of consecutive input columns averaged into one frame of the matrix";
        param.unit = "";
        param.minValue = 1.0f;
        param.maxValue = 64.0f;
        param.defaultValue = 1.0f;
        param.isQuantized = true;
        param.quantizeStep = 1.0f;
        list.push_back(std::move(param));
    }
    return list;
}

float AnlVampPlugin::SelfSimilarity::getParameter(std::string paramid) const
{
    if(paramid == "metric")
    {
        return static_cast<float>(static_cast<size_t>(mMetric));
    }
    if(paramid == "normalization")
    {
        return static_cast<float>(static_cast<size_t>(mNormalization));
    }
    if(paramid == "downsampling")
    {
        return static_cast<float>(mDownsampling);
    }
    std::cerr << "Invalid parameter : " << paramid << "\n";
    return 0.0f;
}

void AnlVampPlugin::SelfSimilarity::setParameter(std::string paramid, float newval)
{
    if(paramid == "metric")
    {
        mMetric = static_cast<Metric>(static_cast<size_t>(std::clamp(std::round(newval), 0.0f, 2.0f)));
    }
    else if(paramid == "normalization")
    {
        mNormalization = static_cast<Normalization>(static_cast<size_t>(std::clamp(std::round(newval), 0.0f, 2.0f)));
    }
    else if(paramid == "downsampling")
    {
        mDownsampling = static_cast<size_t>(std::clamp(std::round(newval), 1.0f, 64.0f));
    }
    else
    {
        std::cerr << "Invalid parameter : " << paramid << "\n";
    }
}

Ive::PluginExtension::InputList AnlVampPlugin::SelfSimilarity::getInputDescriptors() const
{
    OutputDescriptor d;
    d.identifier = "columns";
    d.name = "Columns";
    d.description = "The columns of the matrix to compare (e.g. the energies of a spectrogram)";
    d.unit = "";
    // A fixed bin count greater than one only specifies that the input expects a matrix: the columns can have any size
    d.hasFixedBinCount = true;
    d.binCount = 2;
    d.hasKnownExtents = false;
    d.minValue = 0.0f;
    d.maxValue = 0.0f;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::SampleType::VariableSampleRate;
    d.hasDuration = false;
    return {d};
}

void AnlVampPlugin::SelfSimilarity::setPreComputingFeatures(FeatureSet const& fs)
{
    // The input is only stored here, the matrix is computed in getRemainingFeatures()
    // so the result doesn't depend on the order of the calls and of the parameters.
    auto const it = fs.find(0);
    mColumns = it != fs.cend() ? it->second : FeatureList{};
    std::stable_sort(mColumns.begin(), mColumns.end(), [](Feature const& lhs, Feature const& rhs)
                     {
                         return lhs.timestamp < rhs.timestamp;
                     });
}

Vamp::Plugin::FeatureSet AnlVampPlugin::SelfSimilarity::process(const float* const*, Vamp::RealTime)
{
    // The audio is not analysed, the matrix only depends on the input columns
    return {};
}

Vamp::Plugin::FeatureSet AnlVampPlugin::SelfSimilarity::getRemainingFeatures()
{
    if(mColumns.empty())
    {
        return {};
    }

    // Averages the groups of consecutive input columns into frames of the same size (the missing bins and the
    // non-finite values are replaced by zero)
    auto const numBins = std::max_element(mColumns.cbegin(), mColumns.cend(), [](Feature const& lhs, Feature const& rhs)
                                          {
                                              return lhs.values.size() < rhs.values.size();
                                          })
                             ->values.size();
    auto const numFrames = (mColumns.size() + mDownsampling - 1_z) / mDownsampling;
    std::vector<std::vector<double>> frames(numFrames, std::vector<double>(numBins, 0.0));
    std::vector<Vamp::RealTime> timestamps(numFrames);
    for(auto frameIndex = 0_z; frameIndex < numFrames; ++frameIndex)
    {
        auto& frame = frames[frameIndex];
        auto const start = frameIndex * mDownsampling;
        auto const end = std::min(start + mDownsampling, mColumns.size());
        timestamps[frameIndex] = mColumns.at(start).timestamp;
        for(auto columnIndex = start; columnIndex < end; ++columnIndex)
        {
            auto const& values = mColumns.at(columnIndex).values;
            for(auto binIndex = 0_z; binIndex < values.size(); ++binIndex)
            {
                frame[binIndex] += std::isfinite(values[binIndex]) ? static_cast<double>(values[binIndex]) : 0.0;
            }
        }
        auto const scale = 1.0 / static_cast<double>(end - start);
        for(auto& value : frame)
        {
            value *= scale;
        }
    }

    // Normalizes each bin over time: the centering removes the components shared by all the frames (e.g. the offset
    // of the decibels and the spectral envelope of a spectrogram) that make all the frames similar, and the
    // standardization also gives the same weight to all the bins
    if(mNormalization != Normalization::none)
    {
        for(auto binIndex = 0_z; binIndex < numBins; ++binIndex)
        {
            auto mean = 0.0;
            for(auto const& frame : frames)
            {
                mean += frame[binIndex];
            }
            mean /= static_cast<double>(numFrames);
            auto variance = 0.0;
            for(auto& frame : frames)
            {
                frame[binIndex] -= mean;
                variance += frame[binIndex] * frame[binIndex];
            }
            variance /= static_cast<double>(numFrames);
            // A constant bin is null once centered and doesn't need to be scaled
            if(mNormalization == Normalization::standardization && variance > std::numeric_limits<double>::epsilon())
            {
                auto const scale = 1.0 / std::sqrt(variance);
                for(auto& frame : frames)
                {
                    frame[binIndex] *= scale;
                }
            }
        }
    }

    // The correlation is the cosine similarity of the centered frames
    if(mMetric == Metric::correlation && numBins > 0_z)
    {
        for(auto& frame : frames)
        {
            auto const mean = std::accumulate(frame.cbegin(), frame.cend(), 0.0) / static_cast<double>(numBins);
            for(auto& value : frame)
            {
                value -= mean;
            }
        }
    }

    // Computes the upper triangle of the symmetric matrix and copies it to the lower triangle
    std::vector<std::vector<float>> matrix(numFrames, std::vector<float>(numFrames, 0.0f));
    if(mMetric == Metric::euclidean)
    {
        // The distances are normalized by the maximum distance so the similarity is between 0 and 1
        auto maxDistance = 0.0f;
        for(auto i = 0_z; i < numFrames; ++i)
        {
            for(auto j = i + 1_z; j < numFrames; ++j)
            {
                auto sum = 0.0;
                for(auto binIndex = 0_z; binIndex < numBins; ++binIndex)
                {
                    auto const diff = frames[i][binIndex] - frames[j][binIndex];
                    sum += diff * diff;
                }
                auto const distance = static_cast<float>(std::sqrt(sum));
                matrix[i][j] = distance;
                maxDistance = std::max(maxDistance, distance);
            }
        }
        for(auto i = 0_z; i < numFrames; ++i)
        {
            matrix[i][i] = 1.0f;
            for(auto j = i + 1_z; j < numFrames; ++j)
            {
                matrix[i][j] = matrix[j][i] = maxDistance > 0.0f ? 1.0f - matrix[i][j] / maxDistance : 1.0f;
            }
        }
    }
    else
    {
        std::vector<double> norms(numFrames);
        std::transform(frames.cbegin(), frames.cend(), norms.begin(), [](std::vector<double> const& frame)
                       {
                           return std::sqrt(std::inner_product(frame.cbegin(), frame.cend(), frame.cbegin(), 0.0));
                       });
        for(auto i = 0_z; i < numFrames; ++i)
        {
            for(auto j = i; j < numFrames; ++j)
            {
                // Two null frames are similar but a null frame is not similar to a non-null frame
                auto const similarity = [&]()
                {
                    if(norms[i] <= 0.0 || norms[j] <= 0.0)
                    {
                        return norms[i] <= 0.0 && norms[j] <= 0.0 ? 1.0 : 0.0;
                    }
                    auto const dot = std::inner_product(frames[i].cbegin(), frames[i].cend(), frames[j].cbegin(), 0.0);
                    return std::clamp(dot / (norms[i] * norms[j]), -1.0, 1.0);
                }();
                matrix[i][j] = matrix[j][i] = static_cast<float>(similarity);
            }
        }
    }

    FeatureList features(numFrames);
    for(auto frameIndex = 0_z; frameIndex < numFrames; ++frameIndex)
    {
        auto& feature = features[frameIndex];
        feature.hasTimestamp = true;
        feature.timestamp = timestamps[frameIndex];
        feature.hasDuration = false;
        feature.values = std::move(matrix[frameIndex]);
    }
    return {{0, std::move(features)}};
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
            case 4:
            {
                static Vamp::PluginAdapter<AnlVampPlugin::SelfSimilarity> adaptater;
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
            case 1:
            {
                return Ive::PluginAdapter::getDescriptor<AnlVampPlugin::SelfSimilarity>();
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
