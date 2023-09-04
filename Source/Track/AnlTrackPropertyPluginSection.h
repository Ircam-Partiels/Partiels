#pragma once

#include "AnlTrackDirector.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class PropertyPluginSection
    : public juce::Component
    {
    public:
        PropertyPluginSection(Director& director);
        ~PropertyPluginSection() override;

        // juce::Component
        void resized() override;

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        Plugin::DescriptionPanel mDescriptionPanel;
    };
} // namespace Track

ANALYSE_FILE_END
