#pragma once

#include <IvePluginAdapter.hpp>
#include <optional>

namespace AnlVampPlugin
{
    constexpr std::size_t operator""_z(unsigned long long n) { return n; }

    class Base
    : public Vamp::Plugin
    {
    public:
        Base(float sampleRate);
        ~Base() override = default;

        std::string getMaker() const override;
        int getPluginVersion() const override;
        std::string getCopyright() const override;

        void reset() override;
        FeatureSet getRemainingFeatures() override;
    };

    class Waveform
    : public Base
    {
    public:
        Waveform(float sampleRate);
        ~Waveform() override = default;

        bool initialise(size_t channels, size_t stepSize, size_t blockSize) override;

        size_t getPreferredBlockSize() const override;
        InputDomain getInputDomain() const override;
        std::string getIdentifier() const override;
        std::string getName() const override;
        std::string getDescription() const override;

        OutputList getOutputDescriptors() const override;

        FeatureSet process(const float* const* inputBuffers, Vamp::RealTime timestamp) override;

    private:
        size_t mNumChannels{0_z};
        size_t mBlockSize{0_z};
        size_t mStepSize{0_z};
    };

    class Spectrogram
    : public Base
    {
    public:
        Spectrogram(float sampleRate);
        ~Spectrogram() override = default;

        bool initialise(size_t channels, size_t stepSize, size_t blockSize) override;

        InputDomain getInputDomain() const override;
        std::string getIdentifier() const override;
        std::string getName() const override;
        std::string getDescription() const override;

        OutputList getOutputDescriptors() const override;

        FeatureSet process(const float* const* inputBuffers, Vamp::RealTime timestamp) override;

    private:
        size_t mNumChannels{0_z};
        size_t mBlockSize{0_z};
        size_t mStepSize{0_z};
    };

    class NewTrack
    : public Base
    {
    public:
        NewTrack(float sampleRate);
        ~NewTrack() override = default;

        bool initialise(size_t channels, size_t stepSize, size_t blockSize) override;

        InputDomain getInputDomain() const override;
        std::string getIdentifier() const override;
        std::string getName() const override;
        std::string getDescription() const override;

        size_t getPreferredBlockSize() const override;

        OutputList getOutputDescriptors() const override;

        FeatureSet process(const float* const* inputBuffers, Vamp::RealTime timestamp) override;
        FeatureSet getRemainingFeatures() override;

    private:
        size_t mNumChannels{0_z};
    };

    // Base must remain the first base class: the IVE adapter reinterprets
    // the Vamp plugin handle (a Vamp::Plugin pointer) as a NoteAggregator pointer.
    class NoteAggregator
    : public Base
    , public Ive::PluginExtension
    {
    public:
        NoteAggregator(float sampleRate);
        ~NoteAggregator() override = default;

        std::string getIdentifier() const override;
        std::string getName() const override;
        std::string getDescription() const override;

        InputDomain getInputDomain() const override;
        size_t getPreferredBlockSize() const override;
        bool initialise(size_t channels, size_t stepSize, size_t blockSize) override;
        void reset() override;

        OutputList getOutputDescriptors() const override;
        ParameterList getParameterDescriptors() const override;
        float getParameter(std::string paramid) const override;
        void setParameter(std::string paramid, float newval) override;

        // Ive::PluginExtension
        InputList getInputDescriptors() const override;
        void setPreComputingFeatures(FeatureSet const& fs) override;

        FeatureSet process(const float* const* inputBuffers, Vamp::RealTime timestamp) override;
        FeatureSet getRemainingFeatures() override;

    private:
        bool mUseMarkersDurations{false};
        Vamp::RealTime mEndOffset;
        std::optional<Vamp::RealTime> mEndTime;
        bool mHasMarkers{false};
        FeatureList mPoints;
        FeatureList mMarkers;
    };
} // namespace AnlVampPlugin
