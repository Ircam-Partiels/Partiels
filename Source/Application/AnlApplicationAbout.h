#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class About
    : public juce::Component
    {
    public:
        About();
        ~About() override = default;

        // juce::Component
        void paint(juce::Graphics& g) override;

        class WindowContainer
        : public FloatingWindowContainer
        {
        public:
            WindowContainer(About& about);

        private:
            About& mAbout;
            juce::TooltipWindow mTooltip;
        };

    private:
        std::unique_ptr<juce::Drawable> mImage;
    };
} // namespace Application

ANALYSE_FILE_END
