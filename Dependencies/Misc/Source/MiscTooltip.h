#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

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
    {
    protected:
        BubbleClient() = default;

    public:
        ~BubbleClient() = default;

        void setTooltip(juce::String const& tooltip);
        juce::String getTooltip() const;

    private:
        juce::String mTooltip;
    };

    class BubbleWindow
    : public juce::Component
    , public juce::Timer
    {
    public:
        BubbleWindow(juce::Component* owner = nullptr, bool acceptsMouseDown = false);
        ~BubbleWindow() override;

        // juce::Component
        void paint(juce::Graphics& g) override;

        // juce::Timer
        void timerCallback() override;

    private:
        bool const mAcceptsMouseDown;
        juce::String mTooltip;
    };

    class BasicWindow
    : public juce::TooltipWindow
    {
    public:
        using juce::TooltipWindow::TooltipWindow;
        ~BasicWindow() override = default;

        // juce::TooltipWindow
        juce::String getTipFor(juce::Component& component) override;
    };
} // namespace Tooltip

MISC_FILE_END
