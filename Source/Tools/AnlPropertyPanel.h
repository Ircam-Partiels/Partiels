#pragma once

#include "AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Tools
{
    class PropertyLayout;
    
    class PropertyPanelBase
    : public juce::Component
    , public juce::SettableTooltipClient
    {
    public:
        juce::Label title;
        
        PropertyPanelBase(std::unique_ptr<juce::Component> c, juce::String const& text, juce::String const& tooltip)
        : content(std::move(c))
        {
            title.setText(text + ":", juce::NotificationType::dontSendNotification);
            title.setJustificationType(juce::Justification::centredLeft);
            title.setTooltip(tooltip);
            title.setMinimumHorizontalScale(1.f);
            title.setInterceptsMouseClicks(false, false);
            auto const& borderSize = title.getBorderSize();
            title.setBorderSize({borderSize.getTop(), borderSize.getLeft(), borderSize.getBottom(), 0});
            
            addAndMakeVisible(title);
            addAndMakeVisible(content.get());
            
            setTooltip(tooltip);
            setSize(200, 24);
        }
        
        ~PropertyPanelBase() override = default;
        
    protected:
        std::unique_ptr<juce::Component> content;
        friend class PropertyLayout;
    };
    
    template <class EntryClass> class PropertyPanel
    : public PropertyPanelBase
    {
    public:
        static_assert(std::is_base_of<juce::Component, EntryClass>::value, "Entry should inherit from juce::Component");
        
        EntryClass& entry;
        
        PropertyPanel(juce::String const& text, juce::String const& tooltip)
        : PropertyPanelBase(std::make_unique<EntryClass>(), text, tooltip)
        , entry(*static_cast<EntryClass*>(content.get()))
        {
        }
        
        ~PropertyPanel() override = default;
    };
}

ANALYSE_FILE_END
