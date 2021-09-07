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

#ifdef JUCE_MAC
#include <sys/xattr.h>

std::vector<juce::File> PluginList::findLibrariesInQuarantine(Accessor const& accessor)
{
    auto isInQuarantine = [](juce::File const& file)
    {
        auto const path = file.getFullPathName();
        auto valLength = getxattr(path.getCharPointer(), "com.apple.quarantine", nullptr, 0, 0, 0);
        if(valLength > 0)
        {
            std::string value(static_cast<size_t>(valLength + 1), '\0');
            valLength = getxattr(path.getCharPointer(), "com.apple.quarantine", value.data(), static_cast<size_t>(valLength), 0, 0);
            if(valLength > 0)
            {
                return value.substr(0_z, 4_z) != "00c1";
            }
        }
        return false;
    };

    std::vector<juce::File> files;
    auto const directories = accessor.getAttr<AttrType::searchPath>();
    for(auto const& directory : directories)
    {
        if(directory.exists())
        {
            auto const libs = directory.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.dylib");
            for(auto const& lib : libs)
            {
                if(isInQuarantine(lib))
                {
                    files.push_back(lib);
                }
            }
        }
    }
    return files;
}

bool PluginList::removeLibrariesFromQuarantine(std::vector<juce::File> const& files)
{
    auto removeFromQuarantine = [](juce::File const& file)
    {
        auto const path = file.getFullPathName();
        auto valLength = getxattr(path.getCharPointer(), "com.apple.quarantine", nullptr, 0_z, 0_z, 0);
        if(valLength > 0)
        {
            std::string value(static_cast<size_t>(valLength + 1), '\0');
            valLength = getxattr(path.getCharPointer(), "com.apple.quarantine", value.data(), static_cast<size_t>(valLength), 0_z, 0);
            anlWeakAssert(valLength >= 4);
            if(valLength > 0 && value.substr(0_z, 4_z) != "00c1")
            {
                value.replace(0_z, 4_z, "00c1");
                value.resize(static_cast<size_t>(valLength));
                return setxattr(path.getCharPointer(), "com.apple.quarantine", value.data(), static_cast<size_t>(valLength), 0_z, XATTR_REPLACE) == 0;
            }
        }
        return true;
    };
    return std::all_of(files.cbegin(), files.cend(), removeFromQuarantine);
}

#endif

ANALYSE_FILE_END
