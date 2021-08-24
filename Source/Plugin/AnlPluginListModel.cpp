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

ANALYSE_FILE_END
