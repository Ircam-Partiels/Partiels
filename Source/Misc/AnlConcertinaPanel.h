#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class ConcertinaPanel
: public juce::Component
, public juce::SettableTooltipClient
, private juce::ComponentListener
, private juce::Timer
{
public:
    
    enum ColourIds : int
    {
          headerBackgroundColourId = 0x2000200
        , headerBorderColourId
        , headerTitleColourId
        , headerButtonColourId
    };
    
    using ComponentRef = std::reference_wrapper<juce::Component>;
    
    ConcertinaPanel(juce::String const& title, bool resizeOnClick, juce::String const& tooltip = "");
    ~ConcertinaPanel() override = default;
    
    juce::String getTitle() const;
    void setComponents(std::vector<ComponentRef> const& component);
    void setOpen(bool isOpen, bool shouldAnimate = false);
    
    std::function<void(void)> onResized = nullptr;
    
    // juce::Component
    void resized() override;
    
    struct LookAndFeelMethods
    {
        virtual ~LookAndFeelMethods() = default;
        
        virtual int getHeaderHeight(ConcertinaPanel const& section) const = 0;
        virtual juce::Font getHeaderFont(ConcertinaPanel const& section, int headerHeight) const = 0;
        virtual void drawHeaderBackground(juce::Graphics& g, ConcertinaPanel const& section, juce::Rectangle<int> area, bool isMouseDown, bool isMouseOver) const = 0;
        virtual void drawHeaderButton(juce::Graphics& g, ConcertinaPanel const& section, juce::Rectangle<int> area, float sizeRatio, bool isMouseDown, bool isMouseOver) const = 0;
        virtual void drawHeaderTitle(juce::Graphics& g, ConcertinaPanel const& section, juce::Rectangle<int> area, juce::Font font, bool isMouseDown, bool isMouseOver) const = 0;
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
    
    Header mHeader;
    juce::String const mTitle;
    std::vector<juce::Component::SafePointer<juce::Component>> mContents;
    float mSizeRatio = 1.0f;
    bool mOpened = true;
};

ANALYSE_FILE_END
