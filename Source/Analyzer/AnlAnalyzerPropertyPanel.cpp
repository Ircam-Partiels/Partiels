#include "AnlAnalyzerPropertyPanel.h"

ANALYSE_FILE_BEGIN

Analyzer::PropertyPanel::ColourSelector::ColourSelector()
{
    addChangeListener(this);
}

Analyzer::PropertyPanel::ColourSelector::~ColourSelector()
{
    removeChangeListener(this);
}

void Analyzer::PropertyPanel::ColourSelector::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    juce::ignoreUnused(source);
    anlWeakAssert(source == this);
    if(onColourChanged != nullptr)
    {
        onColourChanged(getCurrentColour());
    }
}

Analyzer::PropertyPanel::PropertyPanel(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::name:
            {
                mNameProperty.entry.setText(acsr.getAttr<AttrType::name>(), juce::NotificationType::dontSendNotification);
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
//                auto const description = mAccessor.getAttr<AttrType::description>();
//                auto const state = mAccessor.getAttr<AttrType::state>();
//                if(description.inputDomain == Plugin::InputDomain::FrequencyDomain)
//                {
//                    juce::StringArray const windowNames {"Rectangular", "Triangular", "Hamming", "Hanning", "Blackman", "Nuttall", "BlackmanHarris"};
//                    mDefaultProperties.push_back(std::make_unique<Layout::PropertyComboBox>(juce::translate("Window Type"), juce::translate("The window type"), windowNames, 0, [=](juce::ComboBox const& entry)
//                                                                                            {
//                        auto state = mAccessor.getAttr<AttrType::state>();
//                        state.windowType = static_cast<Plugin::WindowType>(entry.getSelectedItemIndex());
//                        mAccessor.setAttr<AttrType::state>(state, NotificationType::asynchronous);
//                    }));
//
//                    mDefaultProperties.push_back(createProperty("Window Overlapping", "The window overlapping", "512 (samples)"));
//
//                    mDefaultProperties.push_back(std::make_unique<Layout::PropertyComboBox>(juce::translate("Window Type"), juce::translate("The window type"), juce::StringArray{"1x", "2x", "4x", "8x", "16x", "32x", "64x"}, 0, [=](juce::ComboBox const& entry)
//                                                                                            {
//                        auto state = mAccessor.getAttr<AttrType::state>();
//                        state.stepSize = std::max(state.blockSize / static_cast<size_t>(entry.getSelectedItemIndex()), 1_z);
//                        mAccessor.setAttr<AttrType::state>(state, NotificationType::asynchronous);
//                    }));
//                }
//                else
//                {
//                    anlWeakAssert(mDefaultProperties.size() == 2);
//                    if(mDefaultProperties.size() >= 1 && mDefaultProperties[0] != nullptr)
//                    {
//                        static_cast<Layout::PropertyLabel*>(mDefaultProperties[0].get())->entry.setText(<#const String &newText#>, juce::NotificationType::dontSendNotification);
//                    }
//                    mDefaultProperties.push_back(createProperty("Block Size", "The block size", "512 (samples)"));
//                    mDefaultProperties.push_back(createProperty("Step Size", "The step size", "512 (samples)"));
//                }
                
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
    
    mNameProperty.callback = [&](juce::Label const& l)
    {
        mAccessor.setAttr<AttrType::name>(l.getText(), NotificationType::synchronous);
    };
    mNameProperty.entry.setEditable(true, true);
    
    auto onResized = [&]()
    {
        resized();
    };
    mProcessorSection.onResized = onResized;
    mGraphicalSection.onResized = onResized;
    mPluginSection.onResized = onResized;
    
    addAndMakeVisible(mNameProperty);
    addAndMakeVisible(mProcessorSection);
    addAndMakeVisible(mGraphicalSection);
    addAndMakeVisible(mPluginSection);
    
    
    setSize(300, 200);
    mFloatingWindow.setContentNonOwned(this, true);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Analyzer::PropertyPanel::~PropertyPanel()
{
    mAccessor.removeListener(mListener);
}

void Analyzer::PropertyPanel::resized()
{
    auto bound = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    mNameProperty.setBounds(bound.removeFromTop(mNameProperty.getHeight()));
    mProcessorSection.setBounds(bound.removeFromTop(mProcessorSection.getHeight()));
    mGraphicalSection.setBounds(bound.removeFromTop(mGraphicalSection.getHeight()));
    mPluginSection.setBounds(bound.removeFromTop(mPluginSection.getHeight()));
    setSize(bound.getWidth(), std::max(bound.getY(), 120) + 2);
}

void Analyzer::PropertyPanel::updateProcessorProperties()
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
 
    auto createParameterProperty = [&](Plugin::Parameter const& parameter) -> std::unique_ptr<Layout::PropertyPanelBase>
    {
        enum class PropertyType
        {
              comboBox
            , slider
        };
        
        auto const setValue = [=, this](float value)
        {
            auto state = mAccessor.getAttr<AttrType::state>();
            anlWeakAssert(value >= parameter.minValue && value <= parameter.maxValue);
            state.parameters[parameter.identifier] = std::min(std::max(value, parameter.minValue), parameter.maxValue);
            mAccessor.setAttr<AttrType::state>(state, NotificationType::asynchronous);
        };
        
        auto const propertyType = parameter.valueNames.empty() ? PropertyType::slider : PropertyType::comboBox;
        switch(propertyType)
        {
            case PropertyType::comboBox:
            {
                juce::StringArray array;
                auto const& names = parameter.valueNames;
                for(auto const& name : names)
                {
                    array.add(name);
                }
                
                return std::make_unique<Layout::PropertyComboBox>(juce::translate(parameter.name), juce::translate(parameter.description), array, 0, [=](juce::ComboBox const& entry)
                {
                    setValue(static_cast<float>(entry.getSelectedItemIndex()));
                });
            }
                break;
            case PropertyType::slider:
            {
                return std::make_unique<Layout::PropertySlider>(juce::translate(parameter.name), juce::translate(parameter.description), juce::Range<double>{static_cast<double>(parameter.minValue), static_cast<double>(parameter.maxValue)}, parameter.isQuantized ? parameter.quantizeStep : 0.0, parameter.unit, 0.0, [=](juce::Slider const& entry)
                {
                    setValue(static_cast<float>(entry.getValue()));
                });
            }
                break;
        }
        return nullptr;
    };
    
    mDefaultProperties.clear();
    auto const description = mAccessor.getAttr<AttrType::description>();
    if(description.inputDomain == Plugin::InputDomain::FrequencyDomain)
    {
        juce::StringArray const windowNames {"Rectangular", "Triangular", "Hamming", "Hanning", "Blackman", "Nuttall", "BlackmanHarris"};
        mDefaultProperties.push_back(std::make_unique<Layout::PropertyComboBox>(juce::translate("Window Type"), juce::translate("The window type"), windowNames, 0, [=](juce::ComboBox const& entry)
        {
            auto state = mAccessor.getAttr<AttrType::state>();
            state.windowType = static_cast<Plugin::WindowType>(entry.getSelectedItemIndex());
            mAccessor.setAttr<AttrType::state>(state, NotificationType::asynchronous);
        }));
        
        mDefaultProperties.push_back(createProperty("Window Overlapping", "The window overlapping", "512 (samples)"));
        
        mDefaultProperties.push_back(std::make_unique<Layout::PropertyComboBox>(juce::translate("Window Type"), juce::translate("The window type"), juce::StringArray{"1x", "2x", "4x", "8x", "16x", "32x", "64x"}, 0, [=](juce::ComboBox const& entry)
        {
            auto state = mAccessor.getAttr<AttrType::state>();
            state.stepSize = std::max(state.blockSize / static_cast<size_t>(entry.getSelectedItemIndex()), 1_z);
            mAccessor.setAttr<AttrType::state>(state, NotificationType::asynchronous);
        }));
    }
    else
    {
        mDefaultProperties.push_back(createProperty("Block Size", "The block size", "512 (samples)"));
        mDefaultProperties.push_back(createProperty("Step Size", "The step size", "512 (samples)"));
    }
    
    mParameterProperties.clear();
    for(auto const& parameter : description.parameters)
    {
        mParameterProperties[parameter.identifier] = createParameterProperty(parameter);
    }
    
    std::vector<Layout::PropertySection::PanelRef> panels;
    for(auto const& property : mDefaultProperties)
    {
        panels.push_back(*property.get());
    }
    for(auto const& property : mParameterProperties)
    {
        panels.push_back(*property.second.get());
    }
    mProcessorSection.setPanels(panels);
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
    
    std::vector<Layout::PropertySection::PanelRef> panels;
    for(auto const& property : mGraphicalProperties)
    {
        panels.push_back(*property.get());
    }
    mGraphicalSection.setPanels(panels);
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
    
    std::vector<Layout::PropertySection::PanelRef> panels;
    for(auto const& property : mPluginProperties)
    {
        panels.push_back(*property.get());
    }
    mPluginSection.setPanels(panels);
}

void Analyzer::PropertyPanel::show()
{
    mFloatingWindow.setVisible(true);
}

ANALYSE_FILE_END
