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
        
        void show();
        
        // juce::Component
        void paint(juce::Graphics& g) override;
        
    private:
        std::unique_ptr<juce::Drawable> mImage;
        FloatingWindow mFloatingWindow {"About Partiels - v" + juce::String(ProjectInfo::versionString)};
    };
}

ANALYSE_FILE_END

