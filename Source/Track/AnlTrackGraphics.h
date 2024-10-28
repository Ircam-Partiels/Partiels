#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Graphics
    : private juce::AsyncUpdater
    {
    public:
        Graphics() = default;
        ~Graphics() override;

        bool runRendering(Accessor const& accessor, std::unique_ptr<Ive::PluginWrapper> plugin);
        void stopRendering();
        bool isRunning() const;
        float getAdvancement() const;

        std::function<void(Graph graph)> onRenderingUpdated = nullptr;
        std::function<void(Graph graph)> onRenderingEnded = nullptr;
        std::function<void(void)> onRenderingAborted = nullptr;

    private:
        struct DrawInfo
        {
            int numColumns{0};
            int numRows{0};
            bool logScale{false};
            double sampleRate{0.0};
            juce::Range<double> range;
            Ive::PluginWrapper* plugin{nullptr};
            int featureIndex{0};
            bool hasDuration{false};
        };

        void abortRendering();
        void performRendering(std::vector<Track::Result::Data::Columns> const& columns, ColourMap const colourMap, DrawInfo const& info);
        juce::Image createImage(std::vector<Track::Result::Data::Columns> const& columns, size_t index, int imageWidth, int imageHeight, std::array<juce::PixelARGB, 256> const& colours, DrawInfo const& info, float& advancement, std::function<bool(void)> predicate);

        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        // clang-format off
        enum class ProcessState
        {
              available
            , aborted
            , running
            , ended
        };
        // clang-format on

        std::mutex mMutex;
        Graph mGraph;
        Result::Data mData;
        std::unique_ptr<Result::Access> mAccess;
        std::atomic<ProcessState> mRenderingState{ProcessState::available};
        std::thread mRenderingProcess;
        std::mutex mRenderingMutex;
        std::atomic<float> mAdvancement{0.0f};
        Chrono mChrono{"Track", "graphics rendering ended"};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Graphics)
    };
} // namespace Track

ANALYSE_FILE_END
