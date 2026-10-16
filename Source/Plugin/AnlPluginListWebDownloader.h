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

        //! @brief Download the plugin lists from vamp-plugins.org and from the Partiels distribution
        //! @param callback Called when download completes with result flag
        void download(std::function<void(juce::Result, std::vector<Plugin::WebReference>)> callback);

        //! @brief Download the package of an internet-available plugin and extract its installer
        //! @details Several packages can be downloaded at the same time
        //! @param reference The internet-available plugin to download
        //! @param callback Called when the download completes with the result and the installer of the package
        void download(Plugin::WebReference const& reference, std::function<void(juce::Result, juce::File)> callback);

    private:
        //! @brief The state of a package download
        struct PackageDownload
        {
            std::function<void(juce::Result, juce::File)> callback;
            std::future<std::tuple<juce::Result, juce::File>> process;
        };

        //! @brief Download the plugin lists from vamp-plugins.org and from the Partiels distribution
        //! @return Vector of internet plugin information
        static std::vector<Plugin::WebReference> download(std::atomic<bool> const& shouldQuit);

        //! @brief Download the plugin list from vamp-plugins.org
        //! @return Vector of internet plugin information
        static std::vector<Plugin::WebReference> downloadRdf(std::atomic<bool> const& shouldQuit);

        //! @brief Download the plugin list of the Partiels distribution
        //! @return Vector of internet plugin information
        static std::vector<Plugin::WebReference> downloadDistribution(std::atomic<bool> const& shouldQuit);

        //! @brief Parse an RDF file and extract plugin information
        //! @param rdfContent The content of the RDF file
        //! @return Vector of internet plugin information
        static std::vector<Plugin::WebReference> parse(juce::String const& rdfContent, std::atomic<bool> const& shouldQuit);

        //! @brief Download the package of the plugin and find its installer
        //! @details The zip packages are extracted to find the installer (a pkg on MacOS or an exe on Windows),
        //! the other packages (a pkg, an exe or a tar.gz archive) are returned as-is
        //! @param reference The internet-available plugin to download
        //! @param shouldQuit The flag to stop the download
        //! @return The result of the download and the installer of the package
        static std::tuple<juce::Result, juce::File> downloadPackage(Plugin::WebReference const& reference, std::atomic<bool> const& shouldQuit);

        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        std::atomic<bool> mShouldQuit{false};
        std::function<void(juce::Result, std::vector<Plugin::WebReference>)> mCallback;
        std::future<std::tuple<juce::Result, std::vector<Plugin::WebReference>>> mProcess;
        std::vector<PackageDownload> mPackageDownloads;
    };
} // namespace PluginList

ANALYSE_FILE_END
