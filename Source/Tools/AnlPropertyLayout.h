#pragma once

#include "AnlPropertyPanel.h"

ANALYSE_FILE_BEGIN

namespace Tools
{
    class PropertyLayout
    : public juce::Component
    {
    public:
        
        enum ColourIds : int
        {
            separatorColourId = 0x2000000
        };
        
        enum Orientation : bool
        {
            vertical = false,
            horizontal = true
        };
        
        using PanelInfo = std::tuple<std::reference_wrapper<PropertyPanelBase>, int>;
        
        PropertyLayout();
        ~PropertyLayout() override = default;
        
        // juce::Component
        void resized() override;
        
        void setPanels(std::vector<PanelInfo> const& panels);
        void organizePanels(Orientation orientation, int size);
        
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
        
        struct Container
        {
            PanelInfo panelInfos;
            Separator separator;
            
            Container(PanelInfo const& p);
            ~Container() = default;
        };
        
        juce::Component mContent;
        juce::Viewport mViewport;
        std::vector<std::unique_ptr<Container>> mContainers;
    };
}

ANALYSE_FILE_END
