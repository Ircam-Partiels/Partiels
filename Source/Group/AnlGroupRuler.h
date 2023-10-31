#pragma once

#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class NavigationBar
    : public juce::Component
    {
    public:
        NavigationBar(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor);
        ~NavigationBar() override = default;

        // juce::Component
        void resized() override;

    private:
        void updateContent();

        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Transport::Accessor& mTransportAccessor;
        std::unique_ptr<juce::Component> mTrackNavigationBar;
        juce::String mTrackIdentifier;
        LayoutNotifier mLayoutNotifier;
    };
} // namespace Group

ANALYSE_FILE_END
