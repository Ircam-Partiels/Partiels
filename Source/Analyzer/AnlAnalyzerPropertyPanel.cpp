#include "AnlAnalyzerPropertyPanel.h"
#include "AnlAnalyzerProcessor.h"
#include <vamp-hostsdk/PluginLoader.h>

ANALYSE_FILE_BEGIN

Analyzer::PropertyPanel::PropertyPanel(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::key:
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

                auto createParameterEntry = [&](Processor::ParameterDescriptor const& descriptor) -> std::unique_ptr<Layout::PropertyPanelBase>
                {
                    auto const uid = descriptor.identifier;
                    if(descriptor.isQuantized && !descriptor.valueNames.empty())
                    {
                        juce::StringArray names;
                        for(auto const& name : descriptor.valueNames)
                        {
                            names.add(name + descriptor.unit);
                        }
                        auto property = std::make_unique<Layout::PropertyComboBox>(descriptor.name, descriptor.description, names, 0, [&, uid](juce::ComboBox const& entry)
                        {
                            auto parameters = mAccessor.getAttr<AttrType::parameters>();
                            parameters[uid] = static_cast<float>(entry.getSelectedItemIndex());
                            mAccessor.setAttr<AttrType::parameters>(parameters, NotificationType::synchronous);
                        });
                        return property;
                    }
                    
                    auto property = std::make_unique<Layout::PropertyLabel>(descriptor.name, descriptor.description, "", [&, uid = descriptor.identifier](juce::Label const& entry)
                    {
                        JUCE_COMPILER_WARNING("limit the value");
                        auto parameters = mAccessor.getAttr<AttrType::parameters>();
                        parameters[uid] = entry.getText().getFloatValue();
                        mAccessor.setAttr<AttrType::parameters>(parameters, NotificationType::synchronous);
                    });
                    
                    property->entry.onEditorShow = [rawProperty = property.get()]()
                    {
                        if(auto* textEditor = rawProperty->entry.getCurrentTextEditor())
                        {
                            textEditor->setKeyboardType(juce::TextInputTarget::VirtualKeyboardType::decimalKeyboard);
                            textEditor->setInputRestrictions(0, "0123456789.");
                        }
                    };
                    return property;
                };
                
                auto const parameterDescriptors = instance->getParameterDescriptors();
                for(auto const& descriptor : parameterDescriptors)
                {
                    auto property = createParameterEntry(descriptor);
                    if(property != nullptr)
                    {
                        mProperties[descriptor.identifier] = std::move(property);
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
                        panels.push_back(*property.second.get());
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
            case AttrType::feature:
                break;
            case AttrType::name:
                break;
            case AttrType::parameters:
            {
                auto const parameters = acsr.getAttr<AttrType::parameters>();
                for(auto& property : mProperties)
                {
                    auto const uid = property.first;
                    auto const parameter = parameters.find(uid);
                    if(parameter != parameters.cend())
                    {
                        if(auto* comboxBox = dynamic_cast<Layout::PropertyComboBox*>(property.second.get()))
                        {
                            comboxBox->entry.setSelectedItemIndex(static_cast<int>(parameter->second), juce::NotificationType::dontSendNotification);
                        }
                        else if(auto* label = dynamic_cast<Layout::PropertyLabel*>(property.second.get()))
                        {
                            label->entry.setText(juce::String(parameter->second, 2), juce::NotificationType::dontSendNotification);
                        }
                        else
                        {
                            anlWeakAssert(false && "Unsupported layout");
                        }
                    }
                }
            }
                break;
            case AttrType::colourMap:
            {
                mColourMap.entry.setSelectedItemIndex(static_cast<int>(acsr.getAttr<AttrType::colourMap>()));
            }
                break;
            case AttrType::colour:
                break;
            case AttrType::results:
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
