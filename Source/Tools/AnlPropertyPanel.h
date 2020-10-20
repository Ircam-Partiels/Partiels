#pragma once

#include "AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Tools
{
    //! @brief The base class of a property panel that display a title label with an associated component
    class PropertyPanelBase
    : public juce::Component
    , public juce::SettableTooltipClient
    {
    public:
        
        enum Positioning
        {
            left = false, //!< The label postion is at the left of component
            top = true //!< The label postion is on top of the component
        };
        
        juce::Label title;
        Positioning positioning;
        
        PropertyPanelBase(std::unique_ptr<juce::Component> c, juce::String const& text, juce::String const& tooltip = {}, Positioning p = Positioning::left);
        ~PropertyPanelBase() override = default;
        
        // juce::Component
        void resized() override;
        
    protected:
        std::unique_ptr<juce::Component> content;
    };
    
    template <class EntryClass> class PropertyPanel
    : public PropertyPanelBase
    {
    public:
        static_assert(std::is_base_of<juce::Component, EntryClass>::value, "Entry should inherit from juce::Component");
        
        EntryClass& entry;
        
        PropertyPanel(juce::String const& text, juce::String const& tooltip = {})
        : PropertyPanelBase(std::make_unique<EntryClass>(), text, tooltip)
        , entry(*static_cast<EntryClass*>(content.get()))
        {
        }
        
        ~PropertyPanel() override = default;
    };
}

ANALYSE_FILE_END
