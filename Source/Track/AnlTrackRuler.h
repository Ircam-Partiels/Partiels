#pragma once

#include "../Zoom/AnlZoomRuler.h"
#include "AnlTrackModel.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Ruler
    : public juce::Component
    {
    public:
        Ruler(Accessor& accessor);
        ~Ruler() override;

        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void lookAndFeelChanged() override;

    private:
        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        std::vector<std::unique_ptr<Zoom::Ruler>> mRulers;
        Tools::DisplayType mDisplayType{Tools::DisplayType::points};
    };
} // namespace Track

ANALYSE_FILE_END
