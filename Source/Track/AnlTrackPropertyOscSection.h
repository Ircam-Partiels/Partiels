#pragma once

#include "AnlTrackDirector.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class PropertyOscSection
    : public juce::Component
    , private juce::Timer
    {
    public:
        PropertyOscSection(Director& director);
        ~PropertyOscSection() override;

        // juce::Component
        void resized() override;

    private:
        // juce::Timer
        void timerCallback() override;

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        PropertyText mPropertyIdentifier;
        Icon mCopyButton;
        PropertyToggle mPropertySendViaOsc;
        std::unique_ptr<juce::TooltipWindow> mTooltip;
    };
} // namespace Track

ANALYSE_FILE_END
