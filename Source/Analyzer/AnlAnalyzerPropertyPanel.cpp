#include "AnlAnalyzerPropertyPanel.h"
#include "AnlAnalyzerProcessor.h"
#include <vamp-hostsdk/PluginLoader.h>
#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginInputDomainAdapter.h>

ANALYSE_FILE_BEGIN

Analyzer::PropertyPanel::PropertyPanel(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::key:
            case AttrType::feature:
            {
                mPropertyLayout.setPanels({}, Tools::PropertyPanelBase::left);
                mProperties.clear();
                
                auto instance = createPlugin(acsr, 44100.0, true);
                if(instance == nullptr)
                {
                    resized();
                    return;
                }
                
                mPluginName.entry.setText(instance->getName(), juce::NotificationType::dontSendNotification);
                auto const outputDescriptors = instance->getOutputDescriptors();
                anlWeakAssert(!outputDescriptors.empty());
                int itemId = 0;
                mPluginName.entry.setEnabled(outputDescriptors.size() > 1);
                mFeatures.entry.clear();
                for(auto const& descriptor : outputDescriptors)
                {
                    mFeatures.entry.addItem(descriptor.name, ++itemId);
                }
                auto const selectedFeature = acsr.getAttr<AttrType::feature>();
                mFeatures.entry.setSelectedItemIndex(static_cast<int>(selectedFeature));
                
                auto getParameterTextValue = [](Vamp::Plugin::ParameterDescriptor const& descriptor, float pvalue) -> juce::String
                {
                    if(descriptor.isQuantized && !descriptor.valueNames.empty())
                    {
                        return descriptor.valueNames[static_cast<size_t>(pvalue)];
                    }
                    return juce::String(pvalue, 2) + descriptor.unit;
                };

                if(auto* wrapper = dynamic_cast<Vamp::HostExt::PluginWrapper*>(instance.get()))
                {
                    if(auto* adapt = wrapper->getWrapper<Vamp::HostExt::PluginInputDomainAdapter>())
                    {
                        juce::StringArray const names {"Rectangular", "Triangular", "Hamming", "Hanning", "Blackman", "Blackman-Harris"};
                        auto property = std::make_unique<Tools::PropertyComboBox>("Window Type", "The feature fo the plugin", names, static_cast<size_t>(adapt->getWindowType()), [&](juce::ComboBox const& entry)
                        {
                            using WindowType = Vamp::HostExt::PluginInputDomainAdapter::WindowType;
                            //adapt->setWindowType(static_cast<WindowType>(entry.getSelectedItemIndex()));
                        });
                        mProperties.push_back(std::move(property));
                    }
                }
                
                
                auto const parameterDescriptors = instance->getParameterDescriptors();
                for(auto const& descriptor : parameterDescriptors)
                {
                    auto property = std::make_unique<Tools::PropertyLabel>(descriptor.name, descriptor.description);
                    if(property != nullptr)
                    {
                        auto const pvalue = instance->getParameter(descriptor.identifier);
                        property->entry.setText(getParameterTextValue(descriptor, pvalue), juce::NotificationType::dontSendNotification);
                        mProperties.push_back(std::move(property));
                    }
                }
                
                std::vector<Tools::PropertyLayout::PanelRef> panels;
                panels.push_back(mPluginName);
                panels.push_back(mFeatures);
                if(!mProperties.empty())
                {
                    panels.push_back(mAnalysisParameters);
                    for(auto const& property : mProperties)
                    {
                        panels.push_back(*property.get());
                    }
                }
                panels.push_back(mGraphicalParameters);
                if(selectedFeature < outputDescriptors.size())
                {
                    auto const& outputDescriptor = outputDescriptors[selectedFeature];
                    if(outputDescriptor.binCount > 1)
                    {
                        panels.push_back(mColourMap);
                    }
                    else
                    {
                        panels.push_back(mColour);
                    }
                }
                panels.push_back(mAnalyse);
                
                mPropertyLayout.setPanels(panels, Tools::PropertyPanelBase::left);
                setSize(300, std::min(600, static_cast<int>(panels.size()) * 30));
                resized();
            }
            case AttrType::name:
            case AttrType::parameters:
                break;
        }
    };
    
    mPluginName.entry.setEnabled(false);
    mFeatures.callback = [&](juce::ComboBox const& entry)
    {
        mAccessor.setAttr<AttrType::feature>(static_cast<size_t>(entry.getSelectedItemIndex()));
    };
    mAnalyse.callback = [&](juce::TextButton const&)
    {
        if(onAnalyse != nullptr)
        {
            onAnalyse();
        }
    };
    
    mColour.callback = [&](juce::TextButton const&)
    {
        juce::DialogWindow::showModalDialog("Set Colour", &mColourSelector, this, juce::Colours::black, true);
    };
    mColourMap.callback = [&](juce::ComboBox const& entry)
    {
        mAccessor.setAttr<AttrType::colourMap>(static_cast<ColorMap>(entry.getSelectedItemIndex()));
    };
    
    mColourMap.entry.clear();
    mColourMap.entry.addItemList({"Parula", "Heat", "Jet", "Turbo", "Hot", "Gray", "Magma", "Inferno", "Plasma", "Viridis", "Cividis", "Github"}, 1);
    
    addAndMakeVisible(mPropertyLayout);
    setSize(300, 200);
    mColourSelector.setSize(400, 300);
    mColourSelector.addChangeListener(this);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Analyzer::PropertyPanel::~PropertyPanel()
{
    mAccessor.removeListener(mListener);
}

void Analyzer::PropertyPanel::resized()
{
    mPropertyLayout.setBounds(getLocalBounds());
}

void Analyzer::PropertyPanel::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    mAccessor.setAttr<AttrType::colour>(mColourSelector.getCurrentColour());
}

ANALYSE_FILE_END
