#include "AnlAnalyzerPropertyPanel.h"
#include "AnlAnalyzerProcessor.h"
#include <vamp-hostsdk/PluginLoader.h>
#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginInputDomainAdapter.h>

ANALYSE_FILE_BEGIN

Analyzer::PropertyPanel::Title::Title(juce::String const& text, juce::String const& tooltip)
: Tools::PropertyPanel<juce::Label>(text, tooltip)
{
    entry.setVisible(false);
}

Analyzer::PropertyPanel::PropertyPanel(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::key:
            {
                mPropertyLayout.setPanels({}, Tools::PropertyPanelBase::left);
                mProperties.clear();
                
                auto instance = createPlugin(acsr, 44100.0, true);
                if(instance == nullptr)
                {
                    resized();
                    return;
                }
                
                auto getParameterTextValue = [](Vamp::Plugin::ParameterDescriptor const& descriptor, float pvalue) -> juce::String
                {
                    if(descriptor.isQuantized && !descriptor.valueNames.empty())
                    {
                        return descriptor.valueNames[static_cast<size_t>(pvalue)];
                    }
                    return juce::String(pvalue, 2) + descriptor.unit;
                };
                
                mProperties.push_back(std::make_unique<Tools::PropertyLabel>("Name", "The name fo the plugin"));
                static_cast<Tools::PropertyLabel*>(mProperties[0].get())->entry.setText(instance->getName(), juce::NotificationType::dontSendNotification);
                
                auto const outputDescriptors = instance->getOutputDescriptors();
                if(!outputDescriptors.empty())
                {
                    juce::StringArray names;
                    names.ensureStorageAllocated(static_cast<int>(outputDescriptors.size()));
                    for(auto const& descriptor : outputDescriptors)
                    {
                        names.add(descriptor.name);
                    }
                    
                    auto property = std::make_unique<Tools::PropertyComboBox>("Feature", "The feature fo the plugin", names, [&](juce::ComboBox const& entry)
                    {
                        mAccessor.setValue<AttrType::feature>(entry.getSelectedItemIndex());
                    });
                    property->entry.setSelectedItemIndex(static_cast<int>(acsr.getValue<AttrType::feature>()), juce::NotificationType::dontSendNotification);
                    mProperties.push_back(std::move(property));
                }
                
                mProperties.push_back(std::make_unique<Title>("Parameters", "The parameters of the plugin"));
                
                if(auto* wrapper = dynamic_cast<Vamp::HostExt::PluginWrapper*>(instance.get()))
                {
                    if(auto* adapt = wrapper->getWrapper<Vamp::HostExt::PluginInputDomainAdapter>())
                    {
                        auto property = std::make_unique<Tools::PropertyLabel>("Window Type", "The window type...");
                        if(property != nullptr)
                        {
                            auto getWindowText = [&]() -> juce::String
                            {
                                using WindowType = Vamp::HostExt::PluginInputDomainAdapter::WindowType;
                                switch (adapt->getWindowType())
                                {
                                    case WindowType::RectangularWindow:
                                        return "Rectangular";
                                        break;
                                    case WindowType::TriangularWindow:
                                        return "Triangular";
                                        break;
                                    case WindowType::HammingWindow:
                                        return "Hamming";
                                        break;
                                    case WindowType::HanningWindow:
                                        return "Hanning";
                                        break;
                                    case WindowType::BlackmanWindow:
                                        return "Blackman";
                                        break;
                                    case WindowType::NuttallWindow:
                                        return "Nuttall";
                                        break;
                                    case WindowType::BlackmanHarrisWindow:
                                        return "Blackman-Harris";
                                        break;
                                }
                            };
                            property->entry.setText(getWindowText(), juce::NotificationType::dontSendNotification);
                            mProperties.push_back(std::move(property));
                        }
                    }
                }
                
                
                auto const parameterDescriptors = instance->getParameterDescriptors();
                mProperties.reserve(mProperties.size() + parameterDescriptors.size());
                
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
                for(auto const& property : mProperties)
                {
                    panels.push_back(*property.get());
                }
                mPropertyLayout.setPanels(panels, Tools::PropertyPanelBase::left);
                setSize(300, std::min(600, static_cast<int>(mProperties.size()) * 30));
                resized();
            }
                break;
            case AttrType::name:
            case AttrType::parameters:
                break;
        }
    };
    
    addAndMakeVisible(mPropertyLayout);
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    setSize(300, 200);
}

Analyzer::PropertyPanel::~PropertyPanel()
{
    mAccessor.removeListener(mListener);
}

void Analyzer::PropertyPanel::resized()
{
    mPropertyLayout.setBounds(getLocalBounds());
}

ANALYSE_FILE_END
