#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

namespace Tooltip
{
    using Client = juce::TooltipClient;

    class Server
    : public juce::Timer
    {
    public:
        Server() = default;
        ~Server() override = default;

        std::function<void(juce::String const&)> onChanged = nullptr;

        // juce::Timer
        void timerCallback() override;

    private:
        juce::String mTip;
    };

    class Display
    : public juce::Component
    {
    public:
        Display();
        ~Display() override = default;

        // juce::Component
        void resized() override;

    private:
        juce::Label mLabel;
        Server mServer;
    };

    class BubbleClient
    : public juce::SettableTooltipClient
    {
    };

    class BubbleWindow
    : public juce::Component
    , public juce::Timer
    {
    public:
        BubbleWindow();
        ~BubbleWindow() override;

        // juce::Component
        void paint(juce::Graphics& g) override;

        // juce::Timer
        void timerCallback() override;

    private:
        juce::String mTooltip;
    };
} // namespace Tooltip

ANALYSE_FILE_END
