#pragma once

#include "../Tools/AnlMisc.h"

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
    
    class PropertyTitle
    : public PropertyPanel<juce::Component>
    {
    public:
        PropertyTitle(juce::String const& name, juce::String const& tooltip = {});
        ~PropertyTitle() override = default;
    };
    
    class PropertyTextButton
    : public PropertyPanel<juce::TextButton>
    {
    public:
        PropertyTextButton(juce::String const& name, juce::String const& tooltip = {}, callback_type fn = nullptr);
        ~PropertyTextButton() override = default;
        
        // juce::Component
        void resized() override;
    };
    
    class PropertyLabel
    : public PropertyPanel<juce::Label>
    {
    public:
        PropertyLabel(juce::String const& name, juce::String const& tooltip = {}, juce::String const& text = {}, callback_type fn = nullptr);
        ~PropertyLabel() override = default;
    };
    
    class PropertyComboBox
    : public PropertyPanel<juce::ComboBox>
    {
    public:
        PropertyComboBox(juce::String const& text, juce::String const& tooltip = {}, juce::StringArray const& items = {}, size_t index = 0, callback_type fn = nullptr);
        ~PropertyComboBox() override = default;
    };
}

ANALYSE_FILE_END
