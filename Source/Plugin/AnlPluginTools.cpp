
#include "AnlPluginTools.h"

ANALYSE_FILE_BEGIN

std::unique_ptr<juce::Component> Plugin::Tools::createProperty(Parameter const& parameter, std::function<void(Parameter const& parameter, float value)> applyChange)
{
    auto const name = juce::String(Format::withFirstCharUpperCase(parameter.name));
    if(!parameter.valueNames.empty())
    {
        return std::make_unique<PropertyList>(name, parameter.description, parameter.unit, parameter.valueNames, [=](size_t index)
                                              {
                                                  if(applyChange == nullptr)
                                                  {
                                                      return;
                                                  }
                                                  applyChange(parameter, static_cast<float>(index));
                                              });
    }
    else if(parameter.isQuantized && std::abs(parameter.quantizeStep - 1.0f) < std::numeric_limits<float>::epsilon() && std::abs(parameter.minValue) < std::numeric_limits<float>::epsilon() && std::abs(parameter.maxValue - 1.0f) < std::numeric_limits<float>::epsilon())
    {
        return std::make_unique<PropertyToggle>(name, parameter.description, [=](bool state)
                                                {
                                                    if(applyChange == nullptr)
                                                    {
                                                        return;
                                                    }
                                                    applyChange(parameter, state ? 1.0f : 0.f);
                                                });
    }

    auto const description = juce::String(parameter.description) + " [" + juce::String(parameter.minValue, 2) + ":" + juce::String(parameter.maxValue, 2) + (!parameter.isQuantized ? "" : ("-" + juce::String(parameter.quantizeStep, 2))) + "]";
    return std::make_unique<PropertyNumber>(name, description, parameter.unit, juce::Range<float>{parameter.minValue, parameter.maxValue}, parameter.isQuantized ? parameter.quantizeStep : 0.0f, [=](float value)
                                            {
                                                if(applyChange == nullptr)
                                                {
                                                    return;
                                                }
                                                applyChange(parameter, value);
                                            });
}

ANALYSE_FILE_END
