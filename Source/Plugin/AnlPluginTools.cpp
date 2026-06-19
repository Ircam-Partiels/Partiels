#include "AnlPluginTools.h"

ANALYSE_FILE_BEGIN

Plugin::LoadingError::LoadingError(char const* message)
: std::runtime_error(message)
{
}

Plugin::ParametersError::ParametersError(char const* message)
: std::runtime_error(message)
{
}

juce::String Plugin::Tools::rangeToString(float min, float max, bool quantized, float step)
{
    return "[" + juce::String(min, 2) + ":" + juce::String(max, 2) + (quantized ? ("-" + juce::String(step, 2)) : "") + "]";
}

std::unique_ptr<juce::Component> Plugin::Tools::createProperty(Parameter const& parameter, std::function<void(Parameter const& parameter, float value)> applyChange)
{
    auto const name = juce::String(Format::withFirstCharUpperCase(parameter.name));
    if(!parameter.valueNames.empty())
    {
        auto const minValue = parameter.minValue;
        auto const quantizeStep = parameter.isQuantized && std::abs(parameter.quantizeStep) > std::numeric_limits<float>::epsilon() ? parameter.quantizeStep : 1.0f;
        juce::StringArray names;
        for(auto const& valueName : parameter.valueNames)
        {
            names.add(juce::String(valueName));
        }
        return std::make_unique<PropertyList>(name, parameter.description, parameter.unit, names, [=](size_t index)
                                              {
                                                  if(applyChange == nullptr)
                                                  {
                                                      return;
                                                  }
                                                  applyChange(parameter, static_cast<float>(index) * quantizeStep + minValue);
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

    auto const description = juce::String(parameter.description) + " " + rangeToString(parameter.minValue, parameter.maxValue, parameter.isQuantized, parameter.quantizeStep);
    return std::make_unique<PropertyNumber>(name, description, parameter.unit, juce::Range<double>{parameter.minValue, parameter.maxValue}, parameter.isQuantized ? static_cast<double>(parameter.quantizeStep) : 0.0, [=](double value)
                                            {
                                                if(applyChange == nullptr)
                                                {
                                                    return;
                                                }
                                                applyChange(parameter, static_cast<float>(value));
                                            });
}

void Plugin::Tools::setPropertyValue(juce::Component& property, Parameter const& parameter, std::set<float> const& values, juce::NotificationType notification)
{

    if(auto* propertyList = dynamic_cast<PropertyList*>(&property))
    {
        if(values.empty())
        {
            propertyList->entry.setText(juce::translate("No Value"), notification);
        }
        else if(values.size() == 1_z)
        {
            auto const minValue = parameter.minValue;
            auto const quantizeStep = parameter.isQuantized && std::abs(parameter.quantizeStep) > std::numeric_limits<float>::epsilon() ? parameter.quantizeStep : 1.0f;
            propertyList->entry.setSelectedItemIndex(static_cast<int>(std::round((*values.cbegin() - minValue) / quantizeStep)), notification);
        }
        else
        {
            propertyList->entry.setText(juce::translate("Multiple Values"), notification);
        }
    }
    else if(auto* propertyNumber = dynamic_cast<PropertyNumber*>(&property))
    {
        if(values.empty())
        {
            propertyNumber->entry.setText(juce::translate("No Value"), notification);
        }
        else if(values.size() == 1_z)
        {
            propertyNumber->entry.setValue(static_cast<double>(*values.cbegin()), notification);
        }
        else
        {
            propertyNumber->entry.setText(juce::translate("Multiple Values"), notification);
        }
    }
    else if(auto* propertyToggle = dynamic_cast<PropertyToggle*>(&property))
    {
        if(values.empty())
        {
            propertyToggle->entry.getProperties().remove("Multiple Values");
        }
        else if(values.size() == 1_z)
        {
            propertyToggle->entry.getProperties().remove("Multiple Values");
            propertyToggle->entry.setToggleState(*values.cbegin() > 0.5f, notification);
        }
        else
        {
            propertyToggle->entry.getProperties().set("Multiple Values", {true});
            propertyToggle->entry.setToggleState(false, notification);
        }
    }
    else
    {
        MiscWeakAssert(false && "property unsupported");
    }
}

std::unique_ptr<Ive::PluginWrapper> Plugin::Tools::createPluginWrapper(Key const& key, double readerSampleRate)
{
    using namespace Vamp::HostExt;
    auto* pluginLoader = PluginLoader::getInstance();
    MiscStrongAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        throw std::runtime_error("plugin loader is not available");
    }

    MiscStrongAssert(!key.identifier.empty());
    if(key.identifier.empty())
    {
        throw LoadingError("plugin key is not defined.");
    }

    auto instance = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key.identifier, static_cast<float>(readerSampleRate), PluginLoader::ADAPT_INPUT_DOMAIN));
    if(instance == nullptr)
    {
        throw LoadingError("plugin library couldn't be loaded");
    }
    return std::make_unique<Ive::PluginWrapper>(instance.release(), key.identifier);
}

std::vector<std::unique_ptr<Ive::PluginWrapper>> Plugin::Tools::createPluginWrappers(Key const& key, State const& state, size_t numReaderChannels, double readerSampleRate)
{
    using namespace Vamp::HostExt;
    auto* pluginLoader = PluginLoader::getInstance();
    MiscStrongAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        throw std::runtime_error("plugin loader is not available");
    }

    MiscStrongAssert(!key.identifier.empty());
    if(key.identifier.empty())
    {
        throw LoadingError("plugin key is not defined.");
    }
    MiscStrongAssert(!key.feature.empty());
    if(key.feature.empty())
    {
        throw LoadingError("plugin feature is not defined.");
    }
    MiscStrongAssert(state.blockSize > 0);
    if(state.blockSize == 0)
    {
        throw ParametersError("plugin block size is invalid");
    }
    MiscStrongAssert(state.stepSize > 0);
    if(state.stepSize == 0)
    {
        throw ParametersError("plugin step size is invalid");
    }

    std::vector<std::unique_ptr<Ive::PluginWrapper>> plugins;
    auto const sampleRate = static_cast<float>(readerSampleRate);
    auto instance = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key.identifier, sampleRate, PluginLoader::ADAPT_INPUT_DOMAIN));
    if(instance == nullptr)
    {
        throw LoadingError("plugin library couldn't be loaded");
    }
    auto wrapper = std::make_unique<Ive::PluginWrapper>(instance.release(), key.identifier);
    auto const maxChannels = wrapper->getMaxChannelCount();
    while(wrapper != nullptr)
    {
        auto const numChannels = std::min(maxChannels, numReaderChannels);
        numReaderChannels -= numChannels;
        plugins.push_back(std::move(wrapper));
        if(numReaderChannels > 0_z)
        {
            instance = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key.identifier, sampleRate, PluginLoader::ADAPT_INPUT_DOMAIN));
            if(instance == nullptr)
            {
                throw LoadingError("plugin library couldn't be loaded");
            }
            wrapper = std::make_unique<Ive::PluginWrapper>(instance.release(), key.identifier);
        }
    }
    return plugins;
}

std::optional<size_t> Plugin::Tools::getFeatureIndex(Vamp::Plugin const& plugin, std::string const& feature)
{
    auto const outputs = plugin.getOutputDescriptors();
    auto const it = std::find_if(outputs.cbegin(), outputs.cend(), [&](auto const& output)
                                 {
                                     return output.identifier == feature;
                                 });
    if(it == outputs.cend())
    {
        return {};
    }
    return static_cast<size_t>(std::distance(outputs.cbegin(), it));
}

Plugin::InputProperty::InputProperty(Plugin::Input const& input, std::function<void(Plugin::Input const&, juce::String const&)> fn)
: PropertyList(juce::translate("INPUTNAME input track").replace("INPUTNAME", input.name), juce::translate("The track used for the INPUTNAME input to prepare the preprocessing.").replace("INPUTNAME", input.name), "", {}, [=, this](size_t index)
               {
                   if(fn != nullptr)
                   {
                       auto const numIds = static_cast<size_t>(mInputIds.size());
                       if(index == 0_z || index - 1_z >= numIds)
                       {
                           fn(mInput, {});
                       }
                       else
                       {
                           fn(mInput, mInputIds.getReference(static_cast<int>(index - 1_z)));
                       }
                   }
               })
, mInput(input)
{
}

void Plugin::InputProperty::setInputs(juce::StringPairArray const& availableInputs, juce::StringArray const& selectedInputs)
{
    mInputIds = availableInputs.getAllKeys();
    entry.clear(juce::NotificationType::dontSendNotification);
    entry.setEnabled(availableInputs.size() > 0);
    entry.setTextWhenNothingSelected(juce::translate("Not found"));
    entry.addItem(juce::translate("Undefined"), 1);
    entry.addItemList(availableInputs.getAllValues(), 2);
    auto inputCopy = selectedInputs;
    inputCopy.removeEmptyStrings();
    inputCopy.removeDuplicates(false);
    if(inputCopy.isEmpty())
    {
        entry.setSelectedId(1, juce::NotificationType::dontSendNotification);
    }
    else if(inputCopy.size() > 1)
    {
        entry.setTextWhenNothingSelected(juce::translate("Multiple Values"));
        entry.setText(juce::translate("Multiple Values"), juce::NotificationType::dontSendNotification);
    }
    else
    {
        entry.setSelectedId(mInputIds.indexOf(inputCopy.getReference(0)) + 2, juce::NotificationType::dontSendNotification);
    }
}

ANALYSE_FILE_END
