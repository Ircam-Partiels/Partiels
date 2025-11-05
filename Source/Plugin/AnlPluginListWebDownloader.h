#pragma once

#include "AnlPluginListModel.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    //! @brief Manager for downloading and caching internet plugin information
    class WebDownloader
    : private juce::AsyncUpdater
    {
    public:
        WebDownloader() = default;
        ~WebDownloader() override;

        //! @brief Download plugin list from vamp-plugins.org
        //! @param callback Called when download completes with result flag
        void download(std::function<void(juce::Result, std::vector<Plugin::WebReference>)> callback);

    private:
        //! @brief Download plugin list from vamp-plugins.org
        //! @return Vector of internet plugin information
        static std::vector<Plugin::WebReference> download(std::atomic<bool> const& shouldQuit);

        //! @brief Parse an RDF file and extract plugin information
        //! @param rdfContent The content of the RDF file
        //! @return Vector of internet plugin information
        static std::vector<Plugin::WebReference> parse(juce::String const& rdfContent, std::atomic<bool> const& shouldQuit);

        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        std::atomic<bool> mShouldQuit{false};
        std::function<void(juce::Result, std::vector<Plugin::WebReference>)> mCallback;
        std::future<std::tuple<juce::Result, std::vector<Plugin::WebReference>>> mProcess;
    };
} // namespace PluginList

ANALYSE_FILE_END
