#pragma once

#include "MiscBasics.h"
#include "MiscColourSelector.h"
#include "MiscHMSmsField.h"
#include "MiscNumberField.h"

MISC_FILE_BEGIN

//! @brief The base class of a property panel that display a title label with an associated component
class PropertyComponentBase
: public juce::Component
, public juce::SettableTooltipClient
{
public:
    static auto constexpr defaultHeight = 24;
    juce::Label title;

    PropertyComponentBase(std::unique_ptr<juce::Component> c, juce::String const& name, juce::String const& tooltip = {});
    ~PropertyComponentBase() override = default;

    // juce::Component
    void resized() override;

    // juce::SettableTooltipClient
    void setTooltip(juce::String const& newTooltip) override;

protected:
    std::unique_ptr<juce::Component> content;
};

template <class entry_t>
class PropertyComponent
: public PropertyComponentBase
{
public:
    // clang-format off
    enum ColourIds : int
    {
          backgroundColourId = 0x2000700
        , textColourId
    };
    // clang-format on

    static_assert(std::is_base_of<juce::Component, entry_t>::value, "Entry should inherit from juce::Component");
    using entry_type = entry_t;
    using callback_type = std::function<void(entry_type const&)>;

    entry_type& entry;
    callback_type callback = nullptr;

    PropertyComponent(juce::String const& name, juce::String const& tooltip = {}, callback_type fn = nullptr)
    : PropertyComponentBase(std::make_unique<entry_type>(), name, tooltip)
    , entry(*static_cast<entry_type*>(content.get()))
    , callback(fn)
    {
    }

    ~PropertyComponent() override = default;
};

class PropertyTextButton
: public PropertyComponent<juce::TextButton>
{
public:
    PropertyTextButton(juce::String const& name, juce::String const& tooltip, std::function<void(void)> fn);
    ~PropertyTextButton() override = default;

    // juce::Component
    void resized() override;
};

class PropertyText
: public PropertyComponent<juce::Label>
{
public:
    PropertyText(juce::String const& name, juce::String const& tooltip, std::function<void(juce::String)> fn);
    ~PropertyText() override = default;
};

class PropertyLabel
: public PropertyText
{
public:
    PropertyLabel(juce::String const& name, juce::String const& tooltip);
    ~PropertyLabel() override = default;
};

class PropertyNumber
: public PropertyComponent<NumberField>
{
public:
    PropertyNumber(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, juce::Range<float> const& range, float interval, std::function<void(float)> fn);
    ~PropertyNumber() override = default;
};

class PropertySlider
: public PropertyComponent<juce::Slider>
{
public:
    PropertySlider(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, juce::Range<float> const& range, float interval, std::function<void(void)> startFn, std::function<void(float)> fn, std::function<void(void)> endFn, bool showNumberField = true);
    ~PropertySlider() override = default;

    NumberField numberField;

    // juce::Component
    void resized() override;

    // juce::SettableTooltipClient
    void setTooltip(juce::String const& newTooltip) override;
};

class PropertyRangeSlider
: public PropertyComponent<juce::Slider>
{
public:
    PropertyRangeSlider(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, juce::Range<float> const& range, float interval, std::function<void(void)> startFn, std::function<void(float, float)> fn, std::function<void(void)> endFn, bool showNumberField = true);
    ~PropertyRangeSlider() override = default;

    NumberField numberFieldLow;
    NumberField numberFieldHigh;

    // juce::Component
    void resized() override;

    // juce::SettableTooltipClient
    void setTooltip(juce::String const& newTooltip) override;
};

class PropertyToggle
: public PropertyComponent<juce::ToggleButton>
{
public:
    PropertyToggle(juce::String const& name, juce::String const& tooltip, std::function<void(bool)> fn);
    ~PropertyToggle() override = default;

    // juce::Component
    void resized() override;
};

class PropertyList
: public PropertyComponent<juce::ComboBox>
{
public:
    PropertyList(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, std::vector<std::string> const& values, std::function<void(size_t)> fn);
    ~PropertyList() override = default;
};

class PropertyColourButton
: public PropertyComponent<ColourButton>
{
public:
    PropertyColourButton(juce::String const& name, juce::String const& tooltip, juce::String const& header, std::function<void(juce::Colour)> fn, std::function<void()> onEditorShow, std::function<void()> onEditorHide);
    ~PropertyColourButton() override = default;

    // juce::Component
    void resized() override;
};

class PropertyHMSmsField
: public PropertyComponent<HMSmsField>
{
public:
    PropertyHMSmsField(juce::String const& name, juce::String const& tooltip, std::function<void(double)> fn);
    ~PropertyHMSmsField() override = default;

    // juce::Component
    void resized() override;
};

MISC_FILE_END
