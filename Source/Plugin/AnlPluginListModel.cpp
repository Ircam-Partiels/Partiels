#include "AnlPluginListModel.h"
#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginLoader.h>

ANALYSE_FILE_BEGIN

PluginList::Accessor::Accessor()
: Accessor(AttrContainer({{true}, {QuarantineMode::force}, {getDefaultSearchPath()}, {ColumnType::name}, {true}}))
{
}

std::unique_ptr<juce::XmlElement> PluginList::Accessor::parseXml(juce::XmlElement const& xml, int version)
{
    auto copy = std::make_unique<juce::XmlElement>(xml);
    if(copy != nullptr && version <= 0x8)
    {
        XmlParser::toXml(*copy.get(), "useEnvVariable", true);
        XmlParser::toXml(*copy.get(), "searchPath", getDefaultSearchPath());
    }
    return copy;
}

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

void PluginList::setEnvironment(Accessor const& accessor)
{
#if JUCE_MAC
    auto const quarantineMode = accessor.getAttr<AttrType::quarantineMode>();
    Vamp::HostExt::PluginLoader::setIgnoreQuanrantineLibs(quarantineMode == QuarantineMode::force || quarantineMode == QuarantineMode::ignore);
#endif

    std::set<juce::File> files;
    std::string value;
    auto addToValue = [&](std::string const& path)
    {
#if JUCE_MAC || JUCE_LINUX
        auto constexpr separator = ":";
#elif JUCE_WINDOWS
        auto constexpr separator = ";";
#endif
        if(juce::File::isAbsolutePath(path))
        {
            auto const result = files.insert(juce::File(path));
            if(result.second)
            {
                value += result.first->getFullPathName().toStdString() + separator;
            }
        }
        else
        {
            value += path + separator;
        }
    };
    if(accessor.getAttr<AttrType::useEnvVariable>())
    {
        auto const paths = Vamp::PluginHostAdapter::getPluginPath();
        for(auto const& path : paths)
        {
            addToValue(path);
        }
    }
    for(auto const& file : accessor.getAttr<AttrType::searchPath>())
    {
        addToValue(file.getFullPathName().toStdString());
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

#if JUCE_MAC
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
    std::vector<std::string> names;
    for(auto const& file : files)
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
                if(setxattr(path.getCharPointer(), "com.apple.quarantine", value.data(), static_cast<size_t>(valLength), 0_z, XATTR_REPLACE) == 0)
                {
                    names.push_back(file.getFileNameWithoutExtension().toStdString());
                }
            }
        }
    }
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    anlStrongAssert(pluginLoader != nullptr);
    if(pluginLoader != nullptr)
    {
        pluginLoader->listPluginsIn(names);
    }
    return !names.empty();
}

#endif

ANALYSE_FILE_END
