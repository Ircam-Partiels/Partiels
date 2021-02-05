#include "AnlAnalyzerPropertyPanel.h"

ANALYSE_FILE_BEGIN

Analyzer::PropertyPanel::PropertyInt::PropertyInt(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, juce::Range<int> const& range, std::function<void(int)> fn)
: Layout::PropertyPanel<NumberField>(juce::translate(name), juce::translate(tooltip))
{
    entry.setRange({static_cast<double>(range.getStart()), static_cast<double>(range.getEnd())}, juce::NotificationType::dontSendNotification);
    entry.setTooltip(juce::translate(tooltip));
    entry.setJustificationType(juce::Justification::centredRight);
    entry.setNumDecimalsDisplayed(0_z);
    entry.setNumDecimalsEdited(0_z);
    entry.setTextValueSuffix(suffix);
    entry.onValueChanged = [=](double value)
    {
        if(fn != nullptr)
        {
            fn(static_cast<int>(std::ceil(value)));
        }
    };
}

Analyzer::PropertyPanel::PropertyFloat::PropertyFloat(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, juce::Range<float> const& range, std::function<void(float)> fn, size_t numDecimals)
: Layout::PropertyPanel<NumberField>(juce::translate(name), juce::translate(tooltip))
{
    entry.setRange({static_cast<double>(range.getStart()), static_cast<double>(range.getEnd())}, juce::NotificationType::dontSendNotification);
    entry.setTooltip(juce::translate(tooltip));
    entry.setJustificationType(juce::Justification::centredRight);
    entry.setNumDecimalsDisplayed(numDecimals);
    entry.setNumDecimalsEdited(8);
    entry.setTextValueSuffix(suffix);
    entry.onValueChanged = [=](double value)
    {
        if(fn != nullptr)
        {
            fn(static_cast<float>(value));
        }
    };
}

Analyzer::PropertyPanel::PropertyList::PropertyList(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, std::vector<std::string> const& values, std::function<void(size_t)> fn)
: Layout::PropertyPanel<juce::ComboBox>(juce::translate(name), juce::translate(tooltip))
{
    entry.setTooltip(tooltip);
    juce::StringArray items;
    for(auto const& value : values)
    {
        items.add(juce::String(value)+suffix);
    }
    entry.addItemList(items, 1);
    entry.setJustificationType(juce::Justification::centredRight);
    entry.onChange = [=, this]()
    {
        if(fn != nullptr)
        {
            fn(entry.getSelectedItemIndex() > 0 ? static_cast<size_t>(entry.getSelectedItemIndex()) : 0_z);
        }
    };
}

Analyzer::PropertyPanel::PropertyPanel(Accessor& accessor)
: mAccessor(accessor)

, mPropertyWindowType("Window Type", "The window type", "", std::vector<std::string>{"Rectangular", "Triangular", "Hamming", "Hanning", "Blackman", "Nuttall", "BlackmanHarris"}, [=](size_t index)
{
    auto state = mAccessor.getAttr<AttrType::state>();
    state.windowType = static_cast<Plugin::WindowType>(index);
    mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
})
, mPropertyWindowSize("Window Size", "The window size", "samples", std::vector<std::string>{"8", "16", "32", "64", "128", "256", "512", "1024", "2048", "4096"}, [=](size_t index)
{
    auto state = mAccessor.getAttr<AttrType::state>();
    auto const overlapping = state.blockSize / state.stepSize;
    state.blockSize = static_cast<size_t>(std::pow(2.0, static_cast<int>(index) + 3));
    state.stepSize = state.blockSize / overlapping;
    mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
})
, mPropertyWindowOverlapping("Window Overlapping", "The window overlapping", "x", std::vector<std::string>{"1", "2", "4", "8", "16", "32", "64"}, [=](size_t index)
{
    auto state = mAccessor.getAttr<AttrType::state>();
    state.stepSize = state.blockSize / std::max(static_cast<size_t>(std::pow(2.0, static_cast<int>(index))), 1_z);
    mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
})
, mPropertyBlockSize("Block Size", "The block size", "samples", juce::Range<int>{1, 65536}, [=](int value)
{
    auto state = mAccessor.getAttr<AttrType::state>();
    state.blockSize = static_cast<size_t>(value);
    mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
})
, mPropertyStepSize("Step Size", "The step size", "samples", juce::Range<int>{1, 65536}, [=](int value)
{
    auto state = mAccessor.getAttr<AttrType::state>();
    state.stepSize = static_cast<size_t>(value);
    mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
})
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::name:
            {
                mPropertyName.entry.setText(acsr.getAttr<AttrType::name>(), juce::NotificationType::dontSendNotification);
                mFloatingWindow.setName(juce::translate("ANLNAME Properties").replace("ANLNAME", acsr.getAttr<AttrType::name>()).toUpperCase());
            }
                break;
                
            case AttrType::key:
                break;
            case AttrType::description:
            {
                updateProcessorProperties();
                updateGraphicalProperties();
                updatePluginProperties();
            }
            case AttrType::state:
            {
                auto const description = mAccessor.getAttr<AttrType::description>();
                auto const state = mAccessor.getAttr<AttrType::state>();
                auto constexpr silent = juce::NotificationType::dontSendNotification;
                if(description.inputDomain == Plugin::InputDomain::FrequencyDomain)
                {
                    mPropertyWindowType.entry.setSelectedItemIndex(static_cast<int>(state.windowType), silent);
                    mPropertyWindowSize.entry.setSelectedId(static_cast<int>(std::log(state.blockSize) / std::log(2)), silent);
                    mPropertyWindowOverlapping.entry.clear(silent);
                    int itemId = 0;
                    for(size_t i = 1; i <= state.blockSize; i *= 2)
                    {
                        mPropertyWindowOverlapping.entry.addItem(juce::String(static_cast<int>(i)), ++itemId);
                    }
                }
                else
                {
                    mPropertyBlockSize.entry.setValue(static_cast<double>(state.blockSize), silent);
                    mPropertyStepSize.entry.setValue(static_cast<double>(state.stepSize), silent);
                }
                
            }
                break;
            case AttrType::zoomMode:
            case AttrType::colour:
            case AttrType::colourMap:
            case AttrType::results:
            case AttrType::warnings:
                break;
        }
    };
    
    mPropertyName.callback = [&](juce::Label const& l)
    {
        mAccessor.setAttr<AttrType::name>(l.getText(), NotificationType::synchronous);
    };
    mPropertyName.entry.setEditable(true, true);
    
    auto onResized = [&]()
    {
        resized();
    };
    mProcessorSection.onResized = onResized;
    mGraphicalSection.onResized = onResized;
    mPluginSection.onResized = onResized;
    
    addAndMakeVisible(mPropertyName);
    addAndMakeVisible(mProcessorSection);
    addAndMakeVisible(mGraphicalSection);
    addAndMakeVisible(mPluginSection);
    
    setSize(300, 400);
    mViewport.setSize(300 + mViewport.getVerticalScrollBar().getWidth(), 400);
    mViewport.setScrollBarsShown(true, false, true, false);
    mViewport.setViewedComponent(this, false);
    mFloatingWindow.setContentNonOwned(&mViewport, true);
    mFloatingWindow.setResizable(true, false);
    mBoundsConstrainer.setMinimumWidth(mViewport.getWidth() + mFloatingWindow.getBorderThickness().getTopAndBottom());
    mBoundsConstrainer.setMaximumWidth(mViewport.getWidth() + mFloatingWindow.getBorderThickness().getTopAndBottom());
    mBoundsConstrainer.setMinimumHeight(120);
    mBoundsConstrainer.setMinimumOnscreenAmounts(0, 40, 40, 40);
    mFloatingWindow.setConstrainer(&mBoundsConstrainer);
    mFloatingWindow.onChanged = [&]()
    {
        mAccessor.setAttr<AttrType::display>(mFloatingWindow.getWindowStateAsString(), NotificationType::synchronous);
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Analyzer::PropertyPanel::~PropertyPanel()
{
    mAccessor.removeListener(mListener);
}

void Analyzer::PropertyPanel::resized()
{
    auto bound = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    mPropertyName.setBounds(bound.removeFromTop(mPropertyName.getHeight()));
    mProcessorSection.setBounds(bound.removeFromTop(mProcessorSection.getHeight()));
    mGraphicalSection.setBounds(bound.removeFromTop(mGraphicalSection.getHeight()));
    mPluginSection.setBounds(bound.removeFromTop(mPluginSection.getHeight()));
    setSize(300, std::max(bound.getY(), 120) + 2);
}

void Analyzer::PropertyPanel::updateProcessorProperties()
{
    auto createParameterProperty = [&](Plugin::Parameter const& parameter) -> std::unique_ptr<Layout::PropertyPanelBase>
    {
        enum class PropertyType
        {
              integer
            , floating
            , list
        };
        
        auto const setPararmeterValue = [=, this](float value)
        {
            auto state = mAccessor.getAttr<AttrType::state>();
            anlWeakAssert(value >= parameter.minValue && value <= parameter.maxValue);
            state.parameters[parameter.identifier] = std::min(std::max(value, parameter.minValue), parameter.maxValue);
            mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
        };
        
        auto getPropertyType = [&]
        {
            if(!parameter.valueNames.empty())
            {
                return PropertyType::list;
            }
            else if(parameter.isQuantized && (parameter.quantizeStep - std::ceil(parameter.quantizeStep)) < std::numeric_limits<float>::epsilon())
            {
                return PropertyType::integer;
            }
            return PropertyType::floating;
        };
        
        switch(getPropertyType())
        {
            case PropertyType::list:
            {
                return std::make_unique<PropertyList>(parameter.name, parameter.description, parameter.unit, parameter.valueNames, [=](size_t index)
                {
                    setPararmeterValue(static_cast<float>(index));
                });
            }
                break;
            case PropertyType::floating:
            {
                JUCE_COMPILER_WARNING("improve quantization management")
                return std::make_unique<PropertyFloat>(parameter.name, parameter.description, parameter.unit, juce::Range<float>{parameter.minValue, parameter.maxValue}, [=](float value)
                {
                    setPararmeterValue(value);
                });
            }
                break;
            case PropertyType::integer:
            {
                JUCE_COMPILER_WARNING("improve quantization management")
                return std::make_unique<PropertyInt>(parameter.name, parameter.description, parameter.unit, juce::Range<int>{static_cast<int>(parameter.minValue), static_cast<int>(parameter.maxValue)}, [=](int value)
                {
                    setPararmeterValue(static_cast<float>(value));
                });
            }
                break;
        }
        return nullptr;
    };
    
    mParameterProperties.clear();
    auto const description = mAccessor.getAttr<AttrType::description>();
    for(auto const& parameter : description.parameters)
    {
        mParameterProperties[parameter.identifier] = createParameterProperty(parameter);
    }
    
    std::vector<ConcertinaPanel::ComponentRef> components;
    if(description.inputDomain == Plugin::InputDomain::FrequencyDomain)
    {
        components.push_back(mPropertyWindowType);
        components.push_back(mPropertyWindowSize);
        components.push_back(mPropertyWindowOverlapping);
    }
    else
    {
        components.push_back(mPropertyBlockSize);
        components.push_back(mPropertyStepSize);
    }
    for(auto const& property : mParameterProperties)
    {
        components.push_back(*property.second.get());
    }
    mProcessorSection.setComponents(components);
}

void Analyzer::PropertyPanel::updateGraphicalProperties()
{
    auto createProperty = [](juce::String const& name, juce::String const& tootip, juce::String const& text)
    {
        auto property = std::make_unique<Layout::PropertyLabel>(juce::translate(name), juce::translate(tootip), juce::translate(text));
        if(property != nullptr)
        {
            property->entry.setEditable(false, false);
        }
        return property;
    };
    
    mGraphicalProperties.clear();
    auto const output = mAccessor.getAttr<AttrType::description>().output;
    mGraphicalProperties.push_back(createProperty("Color", "Color", "Color"));
    mGraphicalProperties.push_back(createProperty("Range Mode", "Range Mode", "Range Mode"));
    mGraphicalProperties.push_back(createProperty("Range", "Value Range", "Value Range"));
    
    std::vector<ConcertinaPanel::ComponentRef> components;
    for(auto const& property : mGraphicalProperties)
    {
        components.push_back(*property.get());
    }
    mGraphicalSection.setComponents(components);
}

void Analyzer::PropertyPanel::updatePluginProperties()
{
    auto createProperty = [](juce::String const& name, juce::String const& tootip, juce::String const& text)
    {
        auto property = std::make_unique<Layout::PropertyLabel>(juce::translate(name), juce::translate(tootip), juce::translate(text));
        if(property != nullptr)
        {
            property->entry.setEditable(false, false);
        }
        return property;
    };
    
    mPluginProperties.clear();
    auto const description = mAccessor.getAttr<AttrType::description>();
    mPluginProperties.push_back(createProperty("Name", "The name of the plugin", description.name));
    mPluginProperties.push_back(createProperty("Feature", "The feature of the plugin", description.output.name));
    {
        auto property = std::make_unique<Layout::PropertyText>(juce::translate("Details"), juce::translate("The details of the plugin"), description.details + " - " + description.output.description);
        if(property != nullptr)
        {
            property->entry.setEditable(false, false);
        }
        mPluginProperties.push_back(std::move(property));
    }
    
    mPluginProperties.push_back(createProperty("Maker", "The maker of the plugin", description.maker));
    mPluginProperties.push_back(createProperty("Version", "The version of the plugin", juce::String(description.version)));
    if(!description.category.isEmpty())
    {
        mPluginProperties.push_back(createProperty("Category", "The category of the plugin", description.category.isEmpty() ? "-" : description.category));
    }
    
    std::vector<ConcertinaPanel::ComponentRef> components;
    for(auto const& property : mPluginProperties)
    {
        components.push_back(*property.get());
    }
    mPluginSection.setComponents(components);
}

void Analyzer::PropertyPanel::show()
{
    auto const& displayInfo = mAccessor.getAttr<AttrType::display>();
    if(displayInfo.isEmpty())
    {
        auto const& desktop = juce::Desktop::getInstance();
        auto const mousePosition = desktop.getMainMouseSource().getScreenPosition().toInt();
        auto const* display = desktop.getDisplays().getDisplayForPoint(mousePosition);
        anlStrongAssert(display != nullptr);
        if(display != nullptr)
        {
            auto const bounds = display->userArea.withSizeKeepingCentre(mFloatingWindow.getWidth(), mFloatingWindow.getHeight());
            mFloatingWindow.setBounds(bounds);
        }
    }
    else
    {
        mFloatingWindow.restoreWindowStateFromString(displayInfo);
    }
    mFloatingWindow.setVisible(true);
}

ANALYSE_FILE_END
