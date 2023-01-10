#pragma once

#include "AnlGroupPropertyGraphicalsSection.h"
#include "AnlGroupPropertyProcessorsSection.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class PropertyPanel
    : public juce::Component
    , public juce::DragAndDropContainer
    {
    public:
        PropertyPanel(Director& director);
        ~PropertyPanel() override;

        // juce::Component
        void resized() override;

        class WindowContainer
        : public FloatingWindowContainer
        {
        public:
            WindowContainer(PropertyPanel& propertyPanel);

        private:
            PropertyPanel& mPropertyPanel;
            juce::Viewport mViewport;
            juce::TooltipWindow mTooltip;
        };

    private:
        void updateContent();

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        ComponentListener mComponentListener;

        PropertyText mPropertyName;
        PropertyColourButton mPropertyBackgroundColour;
        PropertyList mPropertyZoomTrack;

        PropertyProcessorsSection mPropertyProcessorsSection{mDirector};
        ConcertinaTable mProcessorsSection{juce::translate("PROCESSORS"), true,
                                           juce::translate("The processors' parameters of the group")};

        PropertyGraphicalsSection mPropertyGraphicalsSection{mDirector};
        ConcertinaTable mGraphicalsSection{juce::translate("GRAPHICALS"), true,
                                           juce::translate("The graphicals' parameters of the group")};

        LayoutNotifier mLayoutNotifier;
    };
} // namespace Group

ANALYSE_FILE_END
