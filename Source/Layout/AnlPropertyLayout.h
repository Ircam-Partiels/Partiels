#pragma once

#include "AnlPropertyPanel.h"

ANALYSE_FILE_BEGIN

namespace Layout
{
    class PropertyLayout
    : public juce::Component
    {
    public:
        
        enum ColourIds : int
        {
            separatorColourId = 0x2000100
        };
        
        using PanelRef = std::reference_wrapper<PropertyPanelBase>;
        using Positioning = PropertyPanelBase::Positioning;
        
        PropertyLayout();
        ~PropertyLayout() override = default;
        
        // juce::Component
        void resized() override;
        
        void setPanels(std::vector<PanelRef> const& panels, Positioning position);
        
    private:
        
        class Separator
        : public juce::Component
        {
        public:
            
            Separator() = default;
            ~Separator() override = default;
            
            // juce::Component
            void paint(juce::Graphics& g) override;
        };
        
        using Container = std::tuple<PanelRef, Separator>;
        
        juce::Component mContent;
        juce::Viewport mViewport;
        std::vector<std::unique_ptr<Container>> mContainers;
        Positioning mPositioning = Positioning::left;
    };
}

ANALYSE_FILE_END
