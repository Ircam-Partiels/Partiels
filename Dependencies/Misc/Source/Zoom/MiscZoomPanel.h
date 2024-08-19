#pragma once

#include "MiscZoomModel.h"

MISC_FILE_BEGIN

namespace Zoom
{
    class Panel
    : public juce::Component
    {
    public:
        Panel(Zoom::Accessor& accessor, juce::String const& name, juce::String const& unit);
        ~Panel() override;

        // juce::Component
        void resized() override;

    private:
        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        juce::Label mName;
        PropertyNumber mPropertyStart;
        PropertyNumber mPropertyEnd;
    };

    class TimePanel
    : public juce::Component
    {
    public:
        TimePanel(Accessor& accessor, juce::String const& name = juce::translate("Time"));
        ~TimePanel() override;

        // juce::Component
        void resized() override;

    private:
        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        juce::Label mName;
        PropertyHMSmsField mPropertyStart;
        PropertyHMSmsField mPropertyEnd;
    };
} // namespace Zoom

MISC_FILE_END
