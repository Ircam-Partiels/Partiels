#include "AnlAnalyzerPropertyPanel.h"
#include "AnlAnalyzerProcessor.h"
#include <vamp-hostsdk/PluginLoader.h>

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
                mPropertySection.setPanels({}, Layout::PropertyPanelBase::left);
                mProperties.clear();
                
                auto instance = createProcessor(acsr, 44100.0, AlertType::silent);
                if(instance == nullptr)
                {
                    resized();
                    return;
                }
                
                mPluginName.entry.setText(instance->getName(), juce::NotificationType::dontSendNotification);
                auto const selectedFeature = acsr.getAttr<AttrType::feature>();
                auto const outputDescriptors = instance->getOutputDescriptors();
                anlWeakAssert(outputDescriptors.size() > selectedFeature);
                int itemId = 0;
                mPluginName.entry.setEnabled(outputDescriptors.size() > 1);
                mFeatures.entry.clear();
                for(auto const& descriptor : outputDescriptors)
                {
                    mFeatures.entry.addItem(descriptor.name, ++itemId);
                }
                mFeatures.entry.setSelectedItemIndex(static_cast<int>(selectedFeature));
                
                auto getParameterTextValue = [](Vamp::Plugin::ParameterDescriptor const& descriptor, float pvalue) -> juce::String
                {
                    if(descriptor.isQuantized && !descriptor.valueNames.empty())
                    {
                        return descriptor.valueNames[static_cast<size_t>(pvalue)];
                    }
                    return juce::String(pvalue, 2) + descriptor.unit;
                };

                auto const parameterDescriptors = instance->getParameterDescriptors();
                for(auto const& descriptor : parameterDescriptors)
                {
                    if(descriptor.isQuantized && !descriptor.valueNames.empty())
                    {
                        juce::StringArray strings;
                        for(auto const& names : descriptor.valueNames)
                        {
                            strings.add(names);
                        }
                        auto property = std::make_unique<Layout::PropertyComboBox>(descriptor.name, descriptor.description, strings, instance->getParameter(descriptor.identifier), [&, uid = descriptor.identifier](juce::ComboBox const& entry)
                        {
                            auto parameters = mAccessor.getAttr<AttrType::parameters>();
                            parameters[uid] = static_cast<float>(entry.getSelectedItemIndex());
                            mAccessor.setAttr<AttrType::parameters>(parameters, NotificationType::synchronous);
                        });
                        if(property != nullptr)
                        {
                            mProperties.push_back(std::move(property));
                        }
                    }
                    else
                    {
                        auto property = std::make_unique<Layout::PropertyLabel>(descriptor.name, descriptor.description, "", [&, uid = descriptor.identifier](juce::Label const& entry)
                                                                                {
                            auto parameters = mAccessor.getAttr<AttrType::parameters>();
                            parameters[uid] = entry.getText().getFloatValue();
                            mAccessor.setAttr<AttrType::parameters>(parameters, NotificationType::synchronous);
                        });
                        if(property != nullptr)
                        {
                            auto const pvalue = instance->getParameter(descriptor.identifier);
                            property->entry.setText(getParameterTextValue(descriptor, pvalue), juce::NotificationType::dontSendNotification);
                            mProperties.push_back(std::move(property));
                        }
                    }
                }
                
                std::vector<Layout::PropertySection::PanelRef> panels;
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
                
                mPropertySection.setPanels(panels, Layout::PropertyPanelBase::left);
                setSize(300, std::min(600, static_cast<int>(panels.size()) * 30));
                resized();
            }
            case AttrType::name:
            case AttrType::parameters:
                break;
            case AttrType::colourMap:
            {
                mColourMap.entry.setSelectedItemIndex(static_cast<int>(acsr.getAttr<AttrType::colourMap>()));
            }
                break;
        }
    };
    
    mPluginName.entry.setEnabled(false);
    mFeatures.callback = [&](juce::ComboBox const& entry)
    {
        mAccessor.setAttr<AttrType::feature>(static_cast<size_t>(entry.getSelectedItemIndex()), NotificationType::synchronous);
    };
    
    mColour.callback = [&](juce::TextButton const&)
    {
        mColourSelector.setCurrentColour(mAccessor.getAttr<AttrType::colour>(), juce::NotificationType::dontSendNotification);
        juce::DialogWindow::showModalDialog("Set Colour", &mColourSelector, this, juce::Colours::black, true);
    };
    mColourMap.callback = [&](juce::ComboBox const& entry)
    {
        mAccessor.setAttr<AttrType::colourMap>(static_cast<ColorMap>(entry.getSelectedItemIndex()), NotificationType::synchronous);
    };
    
    mColourMap.entry.clear();
    mColourMap.entry.addItemList({"Parula", "Heat", "Jet", "Turbo", "Hot", "Gray", "Magma", "Inferno", "Plasma", "Viridis", "Cividis", "Github"}, 1);
    
    addAndMakeVisible(mPropertySection);
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
    mPropertySection.setBounds(getLocalBounds());
}

void Analyzer::PropertyPanel::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    juce::ignoreUnused(source);
    anlWeakAssert(source == &mColourSelector);
    mAccessor.setAttr<AttrType::colour>(mColourSelector.getCurrentColour(), NotificationType::synchronous);
}

ANALYSE_FILE_END
