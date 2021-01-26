#include "AnlAnalyzerPropertyPanel.h"
#include "AnlAnalyzerProcessor.h"
#include <vamp-hostsdk/PluginLoader.h>

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
            case AttrType::key:
                break;
            case AttrType::name:
            {
                mNameProperty.entry.setText(acsr.getAttr<AttrType::name>(), juce::NotificationType::dontSendNotification);
            }
                break;
            case AttrType::description:
            {
                updateProcessorProperties();
                updatePluginProperties();
            }
            case AttrType::parameters:
            case AttrType::zoomMode:
            case AttrType::colour:
            case AttrType::colourMap:
            case AttrType::resultsType:
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
    
    mProcessorProperties.clear();
    auto const description = mAccessor.getAttr<AttrType::description>();
    if(description.inputDomain == Plugin::InputDomain::FrequencyDomain)
    {
        mProcessorProperties.push_back(createProperty("Window Type", "The window type", "Hanning"));
        mProcessorProperties.push_back(createProperty("Window Size", "The window size", "512 (samples)"));
        mProcessorProperties.push_back(createProperty("Window Overlapping", "The window overlapping", "4x"));
    }
    else
    {
        mProcessorProperties.push_back(createProperty("Block Size", "The block size", "512 (samples)"));
        mProcessorProperties.push_back(createProperty("Step Size", "The step size", "512 (samples)"));
    }
    
    for(auto const& parameter : description.parameters)
    {
        mProcessorProperties.push_back(createProperty(parameter.name, parameter.description, juce::String(parameter.defaultValue)));
    }
    
    std::vector<Layout::PropertySection::PanelRef> panels;
    for(auto const& property : mProcessorProperties)
    {
        panels.push_back(*property.get());
    }
    mProcessorSection.setPanels(panels);
}

void Analyzer::PropertyPanel::updateGraphicalProperties()
{
    auto const output = mAccessor.getAttr<AttrType::output>();
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
    mPluginProperties.push_back(createProperty("Specialization", "The specialization of the plugin", description.specialization));
    auto property = std::make_unique<Layout::PropertyText>(juce::translate("Details"), juce::translate("The details of the plugin"), description.details);
    if(property != nullptr)
    {
        property->entry.setEditable(false, false);
    }
    
    mPluginProperties.push_back(std::move(property));
    
    mPluginProperties.push_back(createProperty("Maker", "The maker of the plugin", description.maker));
    mPluginProperties.push_back(createProperty("Version", "The version of the plugin", juce::String(description.version)));
    mPluginProperties.push_back(createProperty("Category", "The category of the plugin", description.category.isEmpty() ? "-" : description.category));
    
    std::vector<Layout::PropertySection::PanelRef> panels;
    for(auto const& property : mPluginProperties)
    {
        panels.push_back(*property.get());
    }
    mPluginSection.setPanels(panels);
}

ANALYSE_FILE_END
