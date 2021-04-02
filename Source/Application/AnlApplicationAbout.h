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
        juce::HyperlinkButton mPartielsLink {"Partiels", juce::URL("https://forum.ircam.fr/projects/detail/partiels/")};
        juce::HyperlinkButton mIrcamLink {"Ircam", juce::URL("https://www.ircam.fr/")};
        juce::HyperlinkButton mVampLink {"Vamp", juce::URL("https://www.vamp-plugins.org/")};
    };
}

ANALYSE_FILE_END

