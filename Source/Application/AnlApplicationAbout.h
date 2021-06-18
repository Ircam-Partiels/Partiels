#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class About
    : public FloatingWindowContainer
    {
    public:
        About();
        ~About() override = default;

        // juce::Component
        void paint(juce::Graphics& g) override;

    private:
        std::unique_ptr<juce::Drawable> mImage;
    };
} // namespace Application

ANALYSE_FILE_END
