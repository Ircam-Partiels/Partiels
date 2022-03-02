#pragma once

#include "AnlGroupPropertyProcessorsSection.h"

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
        ComponentListener mComponentListener;

        PropertyText mPropertyName;
        PropertyColourButton mPropertyBackgroundColour;
        PropertyList mPropertyZoomTrack;
        PropertyTextButton mPropertyChannelLayout;
        bool mChannelLayoutActionStarted{false};

        PropertyProcessorsSection mPropertyProcessorsSection{mDirector};
        ConcertinaTable mProcessorsSection{juce::translate("PROCESSORS"), true,
                                           juce::translate("The processors' parameters of the group")};

        juce::Viewport mViewport;
        LayoutNotifier mLayoutNotifier;
    };
} // namespace Group

ANALYSE_FILE_END
