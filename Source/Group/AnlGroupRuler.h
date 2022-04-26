#pragma once

#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class SelectionBar
    : public juce::Component
    {
    public:
        SelectionBar(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor);
        ~SelectionBar() override;

        // juce::Component
        void resized() override;
        void colourChanged() override;

    private:
        void updateLayout();

        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Transport::Accessor& mTransportAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        std::vector<std::unique_ptr<Transport::SelectionBar>> mSelectionBars;
        LayoutNotifier mLayoutNotifier;
    };
} // namespace Group

ANALYSE_FILE_END
