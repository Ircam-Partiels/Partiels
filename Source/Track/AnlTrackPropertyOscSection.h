#pragma once

#include "AnlTrackDirector.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class PropertyOscSection
    : public juce::Component
    {
    public:
        PropertyOscSection(Director& director);
        ~PropertyOscSection() override;

        // juce::Component
        void resized() override;

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        PropertyText mPropertyIdentifier;
        Icon mCopyButton;
        PropertyToggle mPropertySendViaOsc;
    };
} // namespace Track

ANALYSE_FILE_END
