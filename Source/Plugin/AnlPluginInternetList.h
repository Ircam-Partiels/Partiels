#pragma once

#include "AnlPluginModel.h"

ANALYSE_FILE_BEGIN

namespace Plugin
{
    //! @brief Information about an internet-available plugin
    struct InternetPluginInfo
    {
        Plugin::Key key;                 //!< The plugin identifier
        juce::String name;               //!< The name of the plugin
        juce::String libraryDescription; //!< Library description
        juce::String pluginDescription;  //!< Plugin description
        juce::String maker;              //!< The creator of the plugin
        juce::String downloadUrl;        //!< The download link
        bool isCompatible{false};        //!< Whether the plugin is compatible with this system

        inline bool operator==(InternetPluginInfo const& rhd) const noexcept
        {
            return key == rhd.key &&
                   name == rhd.name &&
                   libraryDescription == rhd.libraryDescription &&
                   pluginDescription == rhd.pluginDescription &&
                   maker == rhd.maker &&
                   downloadUrl == rhd.downloadUrl &&
                   isCompatible == rhd.isCompatible;
        }

        inline bool operator!=(InternetPluginInfo const& rhd) const noexcept
        {
            return !(*this == rhd);
        }
    };

    void to_json(nlohmann::json& j, InternetPluginInfo const& info);
    void from_json(nlohmann::json const& j, InternetPluginInfo& info);
} // namespace Plugin

namespace XmlParser
{
    template <>
    void toXml<Plugin::InternetPluginInfo>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::InternetPluginInfo const& value);

    template <>
    auto fromXml<Plugin::InternetPluginInfo>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::InternetPluginInfo const& defaultValue)
        -> Plugin::InternetPluginInfo;
} // namespace XmlParser

namespace PluginList
{
    //! @brief Parser for Vamp RDF files
    class RdfParser
    {
    public:
        //! @brief Parse an RDF file and extract plugin information
        //! @param rdfContent The content of the RDF file
        //! @return Vector of internet plugin information
        static std::vector<Plugin::InternetPluginInfo> parseRdfContent(juce::String const& rdfContent);

        //! @brief Check if a plugin is compatible with the current system
        //! @param apiVersion The vamp:vamp_API_version from the RDF
        //! @param binaryPlatform The vamp:has_binary from the RDF
        //! @return true if compatible
        static bool isPluginCompatible(juce::String const& apiVersion, juce::String const& binaryPlatform);
    };

    //! @brief Manager for downloading and caching internet plugin information
    class InternetPluginManager
    {
    public:
        InternetPluginManager();
        ~InternetPluginManager();

        //! @brief Download plugin index and RDF files from vamp-plugins.org
        //! @param callback Called when download completes with success flag
        void downloadPluginList(std::function<void(bool)> callback);

        //! @brief Get the cached list of internet plugins
        std::vector<Plugin::InternetPluginInfo> const& getPluginList() const;

        //! @brief Load cached plugin list from XML
        void loadFromXml(juce::XmlElement const& xml);

        //! @brief Save plugin list to XML
        std::unique_ptr<juce::XmlElement> toXml() const;

    private:
        void downloadPluginIndex(std::function<void(bool, juce::StringArray)> callback);
        void downloadRdfFile(juce::String const& url, std::function<void(bool, juce::String)> callback);

        std::vector<Plugin::InternetPluginInfo> mPluginList;
        std::atomic<bool> mIsDownloading{false};
    };
} // namespace PluginList

ANALYSE_FILE_END
