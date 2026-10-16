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

        //! @brief Get the temporary directory that contains the installers downloaded by the downloader
        //! @details The directory is specific to the application and is removed when the application starts
        //! @return The directory in which the installers of the plugins are downloaded
        static juce::File getInstallerDirectory();

        //! @brief Download the plugin lists from vamp-plugins.org and from the Partiels distribution
        //! @param callback Called when download completes with result flag
        void download(std::function<void(juce::Result, std::vector<Plugin::WebReference>)> callback);

        //! @brief Download the package of an internet-available plugin and extract its installer
        //! @details Several packages can be downloaded at the same time
        //! @param reference The internet-available plugin to download
        //! @param callback Called when the download completes with the result and the installer of the package
        //! (if the download has been aborted, the result is ok and the installer is invalid)
        void download(Plugin::WebReference const& reference, std::function<void(juce::Result, juce::File)> callback);

        //! @brief Abort the download of the package of an internet-available plugin
        //! @param reference The internet-available plugin of which the package download is aborted
        void abort(Plugin::WebReference const& reference);

        //! @brief Check if the package of an internet-available plugin is being downloaded
        //! @param reference The internet-available plugin of which the package download is checked
        //! @return True if the package of the plugin is being downloaded
        bool isDownloading(Plugin::WebReference const& reference) const;

        //! @brief Get the download progress of the package of an internet-available plugin
        //! @param reference The internet-available plugin of which the package download progress is retrieved
        //! @return The download progress of the package between 0 and 1, negative if the package
        //! is not being downloaded or if the progress is unknown
        double getDownloadProgress(Plugin::WebReference const& reference) const;

    private:
        //! @brief The state of a package download
        //! @details A state is only removed from the downloader when its job has completed,
        //! so the job can safely refer to the state of the download
        struct PackageDownload
        {
            Plugin::WebReference reference;
            std::function<void(juce::Result, juce::File)> callback;
            std::future<std::tuple<juce::Result, juce::File>> process;
            std::atomic<bool> shouldAbort{false};
            std::atomic<double> progress{-1.0};
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

        //! @brief Download the package of the plugin and find or extract its installer
        //! @details The zip packages are extracted to find the installer (a pkg on MacOS or an exe on Windows),
        //! the other packages (a pkg, an exe or a tar.gz archive) are returned as-is.
        //! @param reference The internet-available plugin to download
        //! @param shouldAbort The flag to abort the download
        //! @param progress The download progress of the package (between 0 and 1, negative if the progress is unknown)
        //! @return The result of the download and the installer of the package that must be launched by the user
        static std::tuple<juce::Result, juce::File> downloadPackage(Plugin::WebReference const& reference, std::atomic<bool> const& shouldAbort, std::atomic<double>& progress);

        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        std::atomic<bool> mShouldQuit{false};
        std::function<void(juce::Result, std::vector<Plugin::WebReference>)> mCallback;
        std::future<std::tuple<juce::Result, std::vector<Plugin::WebReference>>> mProcess;
        std::vector<std::unique_ptr<PackageDownload>> mPackageDownloads;
    };
} // namespace PluginList

ANALYSE_FILE_END
