#pragma once

#include "AnlPropertyPanel.h"

ANALYSE_FILE_BEGIN

namespace Layout
{
    class PropertySection
    : public juce::Component
    , public juce::SettableTooltipClient
    , private juce::ComponentListener
    , private juce::Timer
    {
    public:
        
        enum ColourIds : int
        {
              headerBackgroundColourId = 0x2000200
            , headerTitleColourId
            , headerButtonColourId
            , separatorColourId
        };
        
        using PanelRef = std::reference_wrapper<PropertyPanelBase>;
        
        PropertySection(juce::String const& title, bool resizeOnClick, juce::String const& tooltip = "");
        ~PropertySection() override = default;
        
        juce::String getTitle() const;
        void setPanels(std::vector<PanelRef> const& panels);
        void setOpen(bool isOpen, bool shouldAnimate = false);
        
        std::function<void(void)> onResized = nullptr;
        
        // juce::Component
        void resized() override;
        
        struct LookAndFeelMethods
        {
            virtual ~LookAndFeelMethods() = default;
            
            virtual int getSeparatorHeight(PropertySection const& section) const = 0;
            virtual int getHeaderHeight(PropertySection const& section) const = 0;
            virtual juce::Font getHeaderFont(PropertySection const& section, int headerHeight) const = 0;
            virtual void drawHeaderBackground(juce::Graphics& g, PropertySection const& section, juce::Rectangle<int> area, bool isMouseDown, bool isMouseOver) const = 0;
            virtual void drawHeaderButton(juce::Graphics& g, PropertySection const& section, juce::Rectangle<int> area, float sizeRatio, bool isMouseDown, bool isMouseOver) const = 0;
            virtual void drawHeaderTitle(juce::Graphics& g, PropertySection const& section, juce::Rectangle<int> area, juce::Font font, bool isMouseDown, bool isMouseOver) const = 0;
        };
        
    private:
        
        // juce::ComponentListener
        void componentMovedOrResized(juce::Component& component, bool wasMoved, bool wasResized) override;
        
        // juce::Timer
        void timerCallback() override;
        
        class Header
        : public juce::Component
        {
        public:
            Header() = default;
            ~Header() override = default;
            
            std::function<void(void)> onClicked = nullptr;
            
            // juce::Component
            void paint(juce::Graphics& g) override;
            void mouseDown(juce::MouseEvent const& event) override;
        };
        
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
            Container(PanelRef ref);
            ~Container() = default;
            
            juce::Component::SafePointer<PropertyPanelBase> panel;
            Separator separator;
        };
        
        Header mHeader;
        juce::String const mTitle;
        std::vector<std::unique_ptr<Container>> mContainers;
        float mSizeRatio = 1.0f;
        bool mOpened = true;
    };
}

ANALYSE_FILE_END
