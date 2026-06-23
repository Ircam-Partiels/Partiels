#pragma once

#include <IvePluginAdapter.hpp>

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

    class Stats
    : public Base
    , public Ive::PluginExtension
    {
    public:
        Stats(float sampleRate);
        ~Stats() = default;

        bool initialise(size_t channels, size_t stepSize, size_t blockSize) override;

        InputDomain getInputDomain() const override;
        std::string getIdentifier() const override;
        std::string getName() const override;
        std::string getDescription() const override;
        OutputList getOutputDescriptors() const override;

        size_t getPreferredBlockSize() const override;

        FeatureSet process(const float* const* inputBuffers, Vamp::RealTime timestamp) override;
        FeatureSet getRemainingFeatures() override;

        ParameterList getParameterDescriptors() const override;
        float getParameter(std::string paramid) const override;
        void setParameter(std::string paramid, float newval) override;

        // Ive::PluginExtension
        InputList getInputDescriptors() const override;
        void setPreComputingFeatures(FeatureSet const& fs) override;
        OutputExtraList getOutputExtraDescriptors(size_t outputDescriptorIndex) const override;

    private:
        // clang-format off
        enum class Operator
        {
              min
            , max
            , mean
            , median
        };
        // clang-format on

        bool mUseMarkersDurations{false};
        FeatureList mResults;
    };
} // namespace AnlVampPlugin
