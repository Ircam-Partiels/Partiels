#pragma once

#include "AnlPropertyPanel.h"
#include "AnlColouredPanel.h"

ANALYSE_FILE_BEGIN

namespace Tools
{
    class PropertyLayout
    : public juce::Component
    {
    public:
        
        using PanelRef = std::reference_wrapper<PropertyPanelBase>;
        using Positioning = PropertyPanelBase::Positioning;
        
        PropertyLayout();
        ~PropertyLayout() override = default;
        
        // juce::Component
        void resized() override;
        
        void setPanels(std::vector<PanelRef> const& panels, Positioning position);
        
    private:
        
        using Container = std::tuple<PanelRef, ColouredPanel>;
        
        juce::Component mContent;
        juce::Viewport mViewport;
        std::vector<std::unique_ptr<Container>> mContainers;
        Positioning mPositioning = Positioning::left;
    };
}

ANALYSE_FILE_END
