#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

namespace Layout
{
    //! @brief The base class of a property panel that display a title label with an associated component
    class PropertyPanelBase
    : public juce::Component
    , public juce::SettableTooltipClient
    {
    public:
        
        juce::Label title;
        
        PropertyPanelBase(std::unique_ptr<juce::Component> c, juce::String const& name, juce::String const& tooltip = {});
        ~PropertyPanelBase() override = default;
        
        // juce::Component
        void resized() override;
        
    protected:
        std::unique_ptr<juce::Component> content;
    };
    
    template <class entry_t> class PropertyPanel
    : public PropertyPanelBase
    {
    public:
        enum ColourIds : int
        {
              backgroundColourId = 0x2000100
            , textColourId
        };
        
        static_assert(std::is_base_of<juce::Component, entry_t>::value, "Entry should inherit from juce::Component");
        using entry_type = entry_t;
        using callback_type = std::function<void(entry_type const&)>;
        
        entry_type& entry;
        callback_type callback = nullptr;
        
        PropertyPanel(juce::String const& name, juce::String const& tooltip = {}, callback_type fn = nullptr)
        : PropertyPanelBase(std::make_unique<entry_type>(), name, tooltip)
        , entry(*static_cast<entry_type*>(content.get()))
        , callback(fn)
        {
        }
        
        ~PropertyPanel() override = default;
    };
    
    class PropertyLabel
    : public PropertyPanel<juce::Label>
    {
    public:
        PropertyLabel(juce::String const& name, juce::String const& tooltip = {}, juce::String const& text = {}, callback_type fn = nullptr);
        ~PropertyLabel() override = default;
    };
}

ANALYSE_FILE_END
