#include "AnlPluginListModel.h"
#include <vamp-hostsdk/PluginHostAdapter.h>

ANALYSE_FILE_BEGIN

std::vector<juce::File> PluginList::getDefaultSearchPath()
{
#if JUCE_WINDOWS
    return {juce::File::getSpecialLocation(juce::File::SpecialLocationType::globalApplicationsDirectory).getChildFile("Vamp Plugins")};
#elif JUCE_MAC
    return {juce::File("~/Library/Audio/Plug-Ins/Vamp"), juce::File("/Library/Audio/Plug-Ins/Vamp")};
#elif JUCE_LINUX
    return {juce::File("~/vamp"), juce::File("~/.vamp"), juce::File("/usr/local/lib/vamp"), juce::File("/usr/lib/vamp")};
#endif
}

void PluginList::setSearchPath(Accessor const& accessor)
{
#if JUCE_MAC || JUCE_LINUX
    auto constexpr separator = ":";
#elif JUCE_WINDOWS
    auto constexpr separator = ";";
#endif

    std::string value;
    if(accessor.getAttr<AttrType::useEnvVariable>())
    {
        auto const paths = Vamp::PluginHostAdapter::getPluginPath();
        for(auto const& path : paths)
        {
            value += path + separator;
        }
    }
    for(auto const& file : accessor.getAttr<AttrType::searchPath>())
    {
        value += file.getFullPathName().toStdString() + separator;
    }
    if(value.empty())
    {
        value = ":";
    }

#if JUCE_MAC || JUCE_LINUX
    setenv("VAMP_PATH", value.c_str(), 1);
#elif JUCE_WINDOWS
    _putenv_s("VAMP_PATH", value.c_str());
#endif
}

ANALYSE_FILE_END
