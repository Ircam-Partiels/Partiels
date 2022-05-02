#pragma once

#include <vamp-sdk/Plugin.h>

namespace AnlVampPlugin
{
    constexpr std::size_t operator""_z(unsigned long long n) { return n; }

    class Waveform
    : public Vamp::Plugin
    {
    public:
        Waveform(float sampleRate);
        ~Waveform() override = default;

        bool initialise(size_t channels, size_t stepSize, size_t blockSize) override;

        InputDomain getInputDomain() const override;
        std::string getIdentifier() const override;
        std::string getName() const override;
        std::string getDescription() const override;
        std::string getMaker() const override;
        int getPluginVersion() const override;
        std::string getCopyright() const override;

        size_t getPreferredBlockSize() const override;
        size_t getPreferredStepSize() const override;

        OutputList getOutputDescriptors() const override;

        void reset() override;
        FeatureSet process(const float* const* inputBuffers, Vamp::RealTime timestamp) override;
        FeatureSet getRemainingFeatures() override;

    private:
        size_t mNumChannels{0_z};
        size_t mBlockSize{0_z};
        size_t mStepSize{0_z};
    };
} // namespace AnlVampPlugin
