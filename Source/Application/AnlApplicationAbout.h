#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class AboutContent
    : public juce::Component
    {
    public:
        AboutContent();
        ~AboutContent() override = default;

        // juce::Component
        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        Misc::Icon mImage;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutContent)
    };

    class AboutPanel
    : public HideablePanelTyped<AboutContent>
    {
    public:
        AboutPanel();
        ~AboutPanel() override = default;
    };
} // namespace Application

ANALYSE_FILE_END
