#pragma once

#include "AnlGroupDirector.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class PropertyPanel
    : public FloatingWindowContainer
    , public juce::DragAndDropContainer
    {
    public:
        PropertyPanel(Director& director);
        ~PropertyPanel() override;

        // juce::Component
        void resized() override;

    private:
        void updateContent();
        void showChannelLayout();

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};

        PropertyText mPropertyName;
        PropertyColourButton mPropertyBackgroundColour;
        PropertyList mPropertyZoomTrack;
        PropertyTextButton mPropertyChannelLayout;

        bool mChannelLayoutActionStarted{false};

        LayoutNotifier mLayoutNotifier;

        static auto constexpr sInnerWidth = 300;
    };
} // namespace Group

ANALYSE_FILE_END
