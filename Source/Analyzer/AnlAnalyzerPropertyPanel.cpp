#include "AnlAnalyzerPropertyPanel.h"
#include <vamp-hostsdk/PluginLoader.h>
#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginInputDomainAdapter.h>

ANALYSE_FILE_BEGIN

Analyzer::PropertyPanel::Property::Property(juce::String const& text, juce::String const& tooltip)
: Tools::PropertyPanel<juce::Label>(text, tooltip)
{
    entry.setJustificationType(juce::Justification::right);
    entry.setMinimumHorizontalScale(1.0f);
}

Analyzer::PropertyPanel::PropertyPanel(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onChanged = [&](Accessor const& acsr, Attribute attribute)
    {
        if(attribute == Attribute::key)
        {
            
        }
    };
    
    mReceiver.onSignal = [&](Accessor const& acsr, Signal signal, juce::var value)
    {
        if(signal == Signal::pluginInstanceChanged)
        {
            mPropertyLayout.setPanels({}, Tools::PropertyPanelBase::left);
            mProperties.clear();
            
            auto instance = acsr.getInstance();
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
            
            
            
            mProperties.push_back(std::make_unique<Property>("Name", "The name fo the plugin"));
            mProperties[0]->entry.setText(instance->getName(), juce::NotificationType::dontSendNotification);
            
            if(auto* wrapper = dynamic_cast<Vamp::HostExt::PluginWrapper*>(instance.get()))
            {
                if(auto* adapt = wrapper->getWrapper<Vamp::HostExt::PluginInputDomainAdapter>())
                {
                    auto property = std::make_unique<Property>("Window Type", "The window type...");
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
            
            
            auto const& parameterDescriptors = instance->getParameterDescriptors();
            mProperties.reserve(mProperties.size() + parameterDescriptors.size());
            
            for(auto const& descriptor : parameterDescriptors)
            {
                auto property = std::make_unique<Property>(descriptor.name, descriptor.description);
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
    };
    
    addAndMakeVisible(mPropertyLayout);
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.addReceiver(mReceiver);
    setSize(300, 200);
}

Analyzer::PropertyPanel::~PropertyPanel()
{
    mAccessor.removeReceiver(mReceiver);
    mAccessor.removeListener(mListener);
}

void Analyzer::PropertyPanel::resized()
{
    mPropertyLayout.setBounds(getLocalBounds());
}

ANALYSE_FILE_END
