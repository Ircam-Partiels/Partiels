#include "AnlPluginInternetList.h"
#include <regex>

ANALYSE_FILE_BEGIN

// JSON serialization for InternetPluginInfo
void Plugin::to_json(nlohmann::json& j, InternetPluginInfo const& info)
{
    j = nlohmann::json{{"key", {{"identifier", info.key.identifier}, {"feature", info.key.feature}}},
                       {"name", info.name.toStdString()},
                       {"libraryDescription", info.libraryDescription.toStdString()},
                       {"pluginDescription", info.pluginDescription.toStdString()},
                       {"maker", info.maker.toStdString()},
                       {"downloadUrl", info.downloadUrl.toStdString()},
                       {"isCompatible", info.isCompatible}};
}

void Plugin::from_json(nlohmann::json const& j, InternetPluginInfo& info)
{
    info.key.identifier = j.at("key").at("identifier").get<std::string>();
    info.key.feature = j.at("key").at("feature").get<std::string>();
    info.name = juce::String(j.at("name").get<std::string>());
    info.libraryDescription = juce::String(j.at("libraryDescription").get<std::string>());
    info.pluginDescription = juce::String(j.at("pluginDescription").get<std::string>());
    info.maker = juce::String(j.at("maker").get<std::string>());
    info.downloadUrl = juce::String(j.at("downloadUrl").get<std::string>());
    info.isCompatible = j.at("isCompatible").get<bool>();
}

// XML serialization for InternetPluginInfo
template <>
void XmlParser::toXml<Plugin::InternetPluginInfo>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::InternetPluginInfo const& value)
{
    auto element = std::make_unique<juce::XmlElement>(attributeName);
    element->setAttribute("identifier", value.key.identifier);
    element->setAttribute("feature", value.key.feature);
    element->setAttribute("name", value.name);
    element->setAttribute("libraryDescription", value.libraryDescription);
    element->setAttribute("pluginDescription", value.pluginDescription);
    element->setAttribute("maker", value.maker);
    element->setAttribute("downloadUrl", value.downloadUrl);
    element->setAttribute("isCompatible", value.isCompatible);
    xml.addChildElement(element.release());
}

template <>
auto XmlParser::fromXml<Plugin::InternetPluginInfo>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::InternetPluginInfo const& defaultValue)
    -> Plugin::InternetPluginInfo
{
    auto const* element = xml.getChildByName(attributeName);
    if(element == nullptr)
    {
        return defaultValue;
    }

    Plugin::InternetPluginInfo info;
    info.key.identifier = element->getStringAttribute("identifier", "").toStdString();
    info.key.feature = element->getStringAttribute("feature", "").toStdString();
    info.name = element->getStringAttribute("name", "");
    info.libraryDescription = element->getStringAttribute("libraryDescription", "");
    info.pluginDescription = element->getStringAttribute("pluginDescription", "");
    info.maker = element->getStringAttribute("maker", "");
    info.downloadUrl = element->getStringAttribute("downloadUrl", "");
    info.isCompatible = element->getBoolAttribute("isCompatible", false);
    return info;
}

// RDF Parser implementation
std::vector<Plugin::InternetPluginInfo> PluginList::RdfParser::parseRdfContent(juce::String const& rdfContent)
{
    std::vector<Plugin::InternetPluginInfo> plugins;

    // Simple RDF parsing using string matching
    // This is a basic implementation - a full RDF parser would be more robust

    // Extract library information
    juce::String libraryDescription;
    juce::String downloadPage;

    // Find library dc:description
    auto libDescStart = rdfContent.indexOf("dc:description>");
    if(libDescStart >= 0)
    {
        libDescStart += 15; // length of "dc:description>"
        auto libDescEnd = rdfContent.indexOf(libDescStart, "</dc:description>");
        if(libDescEnd > libDescStart)
        {
            libraryDescription = rdfContent.substring(libDescStart, libDescEnd).trim();
        }
    }

    // Find download page
    auto downloadStart = rdfContent.indexOf("doap:download-page");
    if(downloadStart >= 0)
    {
        auto urlStart = rdfContent.indexOf(downloadStart, "rdf:resource=\"");
        if(urlStart >= 0)
        {
            urlStart += 14; // length of "rdf:resource=\""
            auto urlEnd = rdfContent.indexOf(urlStart, "\"");
            if(urlEnd > urlStart)
            {
                downloadPage = rdfContent.substring(urlStart, urlEnd);
            }
        }
    }

    // Find all plugins in the RDF
    int searchStart = 0;
    while(true)
    {
        auto pluginStart = rdfContent.indexOf(searchStart, "vamp:Plugin");
        if(pluginStart < 0)
            break;

        // Find the end of this plugin definition
        auto pluginEnd = rdfContent.indexOf(pluginStart, "</vamp:Plugin>");
        if(pluginEnd < 0)
            break;

        pluginEnd += 14; // include the closing tag
        juce::String pluginBlock = rdfContent.substring(pluginStart, pluginEnd);

        Plugin::InternetPluginInfo info;

        // Extract plugin identifier (library:feature)
        auto identifierStart = pluginBlock.indexOf("rdf:about=\"");
        if(identifierStart >= 0)
        {
            identifierStart += 11; // length of "rdf:about=\""
            auto identifierEnd = pluginBlock.indexOf(identifierStart, "\"");
            if(identifierEnd > identifierStart)
            {
                juce::String fullId = pluginBlock.substring(identifierStart, identifierEnd);
                // Extract library and plugin parts
                // Format is typically: library_name:plugin_name
                auto colonPos = fullId.lastIndexOf(":");
                if(colonPos > 0)
                {
                    info.key.identifier = fullId.substring(0, colonPos).toStdString();
                    info.key.feature = fullId.substring(colonPos + 1).toStdString();
                }
            }
        }

        // Extract dc:title (plugin name)
        auto titleStart = pluginBlock.indexOf("dc:title>");
        if(titleStart >= 0)
        {
            titleStart += 9; // length of "dc:title>"
            auto titleEnd = pluginBlock.indexOf(titleStart, "</dc:title>");
            if(titleEnd > titleStart)
            {
                info.name = pluginBlock.substring(titleStart, titleEnd).trim();
            }
        }

        // Extract dc:description (plugin description)
        auto descStart = pluginBlock.indexOf("dc:description>");
        if(descStart >= 0)
        {
            descStart += 15; // length of "dc:description>"
            auto descEnd = pluginBlock.indexOf(descStart, "</dc:description>");
            if(descEnd > descStart)
            {
                info.pluginDescription = pluginBlock.substring(descStart, descEnd).trim();
            }
        }

        // Extract foaf:maker
        auto makerStart = pluginBlock.indexOf("foaf:name>");
        if(makerStart >= 0)
        {
            makerStart += 10; // length of "foaf:name>"
            auto makerEnd = pluginBlock.indexOf(makerStart, "</foaf:name>");
            if(makerEnd > makerStart)
            {
                info.maker = pluginBlock.substring(makerStart, makerEnd).trim();
            }
        }

        // Extract API version
        juce::String apiVersion;
        auto apiStart = pluginBlock.indexOf("vamp:vamp_API_version");
        if(apiStart >= 0)
        {
            auto apiResStart = pluginBlock.indexOf(apiStart, "rdf:resource=\"");
            if(apiResStart >= 0)
            {
                apiResStart += 14;
                auto apiResEnd = pluginBlock.indexOf(apiResStart, "\"");
                if(apiResEnd > apiResStart)
                {
                    apiVersion = pluginBlock.substring(apiResStart, apiResEnd);
                }
            }
        }

        // Extract binary platform
        juce::String binaryPlatform;
        auto binaryStart = pluginBlock.indexOf("vamp:has_binary");
        if(binaryStart >= 0)
        {
            auto binaryEnd = pluginBlock.indexOf(binaryStart, "</vamp:has_binary>");
            if(binaryEnd > binaryStart)
            {
                binaryPlatform = pluginBlock.substring(binaryStart, binaryEnd);
            }
        }

        info.libraryDescription = libraryDescription;
        info.downloadUrl = downloadPage;
        info.isCompatible = isPluginCompatible(apiVersion, binaryPlatform);

        if(info.key.identifier.empty() || info.key.feature.empty())
        {
            // Skip plugins without valid identifiers
            searchStart = pluginEnd;
            continue;
        }

        plugins.push_back(info);
        searchStart = pluginEnd;
    }

    return plugins;
}

bool PluginList::RdfParser::isPluginCompatible(juce::String const& apiVersion, juce::String const& binaryPlatform)
{
    // Check API version is vamp_API_version_2
    if(!apiVersion.contains("vamp_API_version_2") && !apiVersion.contains("api_version_2"))
    {
        return false;
    }

    // Check platform compatibility
#if JUCE_WINDOWS
    juce::String requiredPlatform = "win";
#elif JUCE_MAC
    juce::String requiredPlatform = "osx";
#elif JUCE_LINUX
    juce::String requiredPlatform = "linux";
#else
    return false;
#endif

    return binaryPlatform.containsIgnoreCase(requiredPlatform);
}

// InternetPluginManager implementation
PluginList::InternetPluginManager::InternetPluginManager()
{
}

PluginList::InternetPluginManager::~InternetPluginManager()
{
}

void PluginList::InternetPluginManager::downloadPluginList(std::function<void(bool)> callback)
{
    if(mIsDownloading.exchange(true))
    {
        // Already downloading
        if(callback)
        {
            callback(false);
        }
        return;
    }

    downloadPluginIndex([this, callback](bool success, juce::StringArray rdfUrls)
                        {
                            if(!success || rdfUrls.isEmpty())
                            {
                                mIsDownloading = false;
                                if(callback)
                                {
                                    callback(false);
                                }
                                return;
                            }

                            // Download each RDF file sequentially
                            std::vector<Plugin::InternetPluginInfo> allPlugins;
                            std::atomic<int> remaining{static_cast<int>(rdfUrls.size())};
                            std::atomic<bool> hasErrors{false};

                            for(auto const& rdfUrl : rdfUrls)
                            {
                                downloadRdfFile(rdfUrl, [this, &allPlugins, &remaining, &hasErrors, callback](bool rdfSuccess, juce::String content)
                                                {
                                                    if(rdfSuccess && content.isNotEmpty())
                                                    {
                                                        auto plugins = RdfParser::parseRdfContent(content);
                                                        if(!plugins.empty())
                                                        {
                                                            // Thread-safe insertion
                                                            for(auto const& plugin : plugins)
                                                            {
                                                                mPluginList.push_back(plugin);
                                                            }
                                                        }
                                                    }
                                                    else
                                                    {
                                                        hasErrors = true;
                                                    }

                                                    if(--remaining == 0)
                                                    {
                                                        mIsDownloading = false;
                                                        if(callback)
                                                        {
                                                            callback(!mPluginList.empty());
                                                        }
                                                    }
                                                });
                            }
                        });
}

std::vector<Plugin::InternetPluginInfo> const& PluginList::InternetPluginManager::getPluginList() const
{
    return mPluginList;
}

void PluginList::InternetPluginManager::loadFromXml(juce::XmlElement const& xml)
{
    mPluginList.clear();

    for(auto* pluginElement : xml.getChildWithTagNameIterator("InternetPlugin"))
    {
        Plugin::InternetPluginInfo info;
        info.key.identifier = pluginElement->getStringAttribute("identifier", "").toStdString();
        info.key.feature = pluginElement->getStringAttribute("feature", "").toStdString();
        info.name = pluginElement->getStringAttribute("name", "");
        info.libraryDescription = pluginElement->getStringAttribute("libraryDescription", "");
        info.pluginDescription = pluginElement->getStringAttribute("pluginDescription", "");
        info.maker = pluginElement->getStringAttribute("maker", "");
        info.downloadUrl = pluginElement->getStringAttribute("downloadUrl", "");
        info.isCompatible = pluginElement->getBoolAttribute("isCompatible", false);

        if(!info.key.identifier.empty() && !info.key.feature.empty())
        {
            mPluginList.push_back(info);
        }
    }
}

std::unique_ptr<juce::XmlElement> PluginList::InternetPluginManager::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement>("InternetPlugins");

    for(auto const& info : mPluginList)
    {
        auto* pluginElement = xml->createNewChildElement("InternetPlugin");
        pluginElement->setAttribute("identifier", info.key.identifier);
        pluginElement->setAttribute("feature", info.key.feature);
        pluginElement->setAttribute("name", info.name);
        pluginElement->setAttribute("libraryDescription", info.libraryDescription);
        pluginElement->setAttribute("pluginDescription", info.pluginDescription);
        pluginElement->setAttribute("maker", info.maker);
        pluginElement->setAttribute("downloadUrl", info.downloadUrl);
        pluginElement->setAttribute("isCompatible", info.isCompatible);
    }

    return xml;
}

void PluginList::InternetPluginManager::downloadPluginIndex(std::function<void(bool, juce::StringArray)> callback)
{
    juce::URL indexUrl("https://www.vamp-plugins.org/rdf/plugins/index.txt");

    auto stream = indexUrl.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress).withConnectionTimeoutMs(5000));

    if(stream == nullptr)
    {
        callback(false, {});
        return;
    }

    juce::String content = stream->readEntireStreamAsString();

    if(content.isEmpty())
    {
        callback(false, {});
        return;
    }

    // Parse the index file - each line is a URL to an RDF file
    juce::StringArray rdfUrls;
    juce::StringArray lines;
    lines.addLines(content);

    for(auto const& line : lines)
    {
        auto trimmedLine = line.trim();
        if(trimmedLine.isNotEmpty() && !trimmedLine.startsWith("#"))
        {
            rdfUrls.add(trimmedLine);
        }
    }

    callback(true, rdfUrls);
}

void PluginList::InternetPluginManager::downloadRdfFile(juce::String const& url, std::function<void(bool, juce::String)> callback)
{
    juce::URL rdfUrl(url);

    auto stream = rdfUrl.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress).withConnectionTimeoutMs(5000));

    if(stream == nullptr)
    {
        callback(false, "");
        return;
    }

    juce::String content = stream->readEntireStreamAsString();
    callback(!content.isEmpty(), content);
}

ANALYSE_FILE_END
