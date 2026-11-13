#include "AnlPluginListWebDownloader.h"

ANALYSE_FILE_BEGIN

PluginList::WebDownloader::~WebDownloader()
{
    mShouldQuit = true;
    if(mProcess.valid())
    {
        [[maybe_unused]] auto const result = mProcess.get();
    }
    for(auto& packageDownload : mPackageDownloads)
    {
        packageDownload->shouldAbort.store(true);
    }
    for(auto& packageDownload : mPackageDownloads)
    {
        [[maybe_unused]] auto const result = packageDownload->process.get();
    }
}

void PluginList::WebDownloader::abort(Plugin::WebReference const& reference)
{
    for(auto& packageDownload : mPackageDownloads)
    {
        if(packageDownload->reference.identifier == reference.identifier)
        {
            packageDownload->shouldAbort.store(true);
        }
    }
}

bool PluginList::WebDownloader::isDownloading(Plugin::WebReference const& reference) const
{
    return std::any_of(mPackageDownloads.cbegin(), mPackageDownloads.cend(), [&reference](std::unique_ptr<PackageDownload> const& packageDownload)
                       {
                           return packageDownload->reference.identifier == reference.identifier;
                       });
}

double PluginList::WebDownloader::getDownloadProgress(Plugin::WebReference const& reference) const
{
    auto const packageDownloadIt = std::find_if(mPackageDownloads.cbegin(), mPackageDownloads.cend(), [&reference](std::unique_ptr<PackageDownload> const& packageDownload)
                                                {
                                                    return packageDownload->reference.identifier == reference.identifier;
                                                });
    return packageDownloadIt != mPackageDownloads.cend() ? (*packageDownloadIt)->progress.load() : 1.0;
}

std::vector<Plugin::WebReference> PluginList::WebDownloader::parse(juce::String const& rdfContent, std::atomic<bool> const& shouldQuit)
{
    // Generates sequences of lines
    std::vector<juce::StringArray> sequences;
    bool inSequence = false;
    for(auto const& line : juce::StringArray::fromLines(rdfContent))
    {
        if(!std::exchange(inSequence, !line.endsWith(".")))
        {
            sequences.push_back({});
        }
        auto fline = line.trimCharactersAtEnd(".; \t\n\r").trimStart();
        if(fline.isNotEmpty())
        {
            sequences.back().add(std::move(fline));
        }
    }
    if(shouldQuit)
    {
        return {};
    }

    // Clean empty sequences
    for(auto& sequence : sequences)
    {
        sequence.removeEmptyStrings();
        sequence.trim();
    }
    sequences.erase(std::remove_if(sequences.begin(), sequences.end(), [](auto const& sequence)
                                   {
                                       return sequence.isEmpty();
                                   }),
                    sequences.end());

    // Helpers methods
    auto const getWords = [](juce::String const& line)
    {
        auto words = juce::StringArray::fromTokens(line, true);
        words.removeEmptyStrings();
        words.trim();
        for(auto& word : words)
        {
            word = word.unquoted().unquoted().unquoted();
        }
        return words;
    };

    auto const forEachSequence = [&](std::function<bool(juce::StringArray const&)> startSequenceCallback, std::function<bool(juce::StringArray const&, juce::StringArray const&)> lineCallback)
    {
        for(auto const& sequence : sequences)
        {
            auto const firstWords = getWords(sequence.getReference(0));
            if(!firstWords.isEmpty() && startSequenceCallback(firstWords))
            {
                for(auto index = 1; index < sequence.size(); ++index)
                {
                    auto const words = getWords(sequence.getReference(index));
                    if(!words.isEmpty() && !lineCallback(firstWords, words))
                    {
                        break;
                    }
                }
            }
        }
    };

    // Retrieve the shortcuts
    std::map<juce::String, juce::String> shortcuts;
    forEachSequence([](juce::StringArray const& firstWords)
                    {
                        return firstWords.getReference(0).startsWith(":");
                    },
                    [&](juce::StringArray const& firstWords, juce::StringArray const& words)
                    {
                        if(words.size() >= 2 && words.getReference(0) == "foaf:name")
                        {
                            shortcuts[firstWords.getReference(0)] = words.getReference(1);
                            return false;
                        }
                        return true;
                    });
    if(shouldQuit)
    {
        return {};
    }

    // Retrieve the libraries
    struct Library
    {
        juce::String identifier;
        juce::String title;
        juce::String description;
        juce::String url;
        juce::String maker;
        bool isCompatible = false;
    };
    std::map<juce::String, Library> libraries;
    forEachSequence([&](juce::StringArray const& firstWords)
                    {
                        return firstWords.size() >= 3 && firstWords.getReference(2) == "vamp:PluginLibrary";
                    },
                    [&](juce::StringArray const& firstWords, juce::StringArray const& words)
                    {
                        if(words.size() < 2)
                        {
                            return false;
                        }
                        auto const libName = firstWords.getReference(0).upToFirstOccurrenceOf(":", false, false);
                        auto& library = libraries[libName];
                        auto const& entry = words.getReference(0);
                        auto const& value = words.getReference(1);
                        if(entry == "vamp:identifier")
                        {
                            library.identifier = value;
                        }
                        else if(entry == "dc:title")
                        {
                            library.title = value;
                        }
                        else if(entry == "dc:description")
                        {
                            library.description = value;
                        }
                        else if(entry == "doap:download-page")
                        {
                            library.url = value.trimCharactersAtStart("<").trimCharactersAtEnd(">");
                        }
                        else if(entry == "foaf:maker")
                        {
                            library.maker = shortcuts[value];
                        }
                        else if(entry == "vamp:has_binary")
                        {
#if JUCE_WINDOWS
                            static auto constexpr requiredPlatform = "win";
#elif JUCE_MAC
                            static auto constexpr requiredPlatform = "osx";
#elif JUCE_LINUX
                            static auto constexpr requiredPlatform = "linux";
#else
#error "Unsupported platform: requiredPlatform is not defined. Please add support for your platform."
#endif
                            if(value.containsIgnoreCase(requiredPlatform))
                            {
                                library.isCompatible = true;
                            }
                        }
                        return true;
                    });
    if(shouldQuit)
    {
        return {};
    }
#if JUCE_DEBUG
    for(auto const& library : libraries)
    {
        MiscDebug("PluginList::WebDownloader", "Library: " + library.first);
    }
#endif

    // Retrieve the plugins
    std::vector<Plugin::WebReference> plugins;
    forEachSequence([&](juce::StringArray const& firstWords)
                    {
                        if(firstWords.size() >= 3 && firstWords.getReference(2) == "vamp:Plugin")
                        {
                            auto const libName = firstWords.getReference(0).upToFirstOccurrenceOf(":", false, false);
                            plugins.push_back(Plugin::WebReference{});
                            auto& plugin = plugins.back();
                            auto const hasPlugin = libraries.count(libName);
                            if(hasPlugin || libraries.count("") > 0_z)
                            {
                                auto const& library = hasPlugin ? libraries.at(libName) : libraries.at("");
                                plugin.identifier = library.identifier;
                                plugin.name = library.title;
                                plugin.libraryDescription = library.description;
                                plugin.maker = library.maker;
                                plugin.websiteUrl = library.url;
                                plugin.isCompatible = library.isCompatible;
                            }
                            return true;
                        }
                        return false;
                    },
                    [&](juce::StringArray const&, juce::StringArray const& words)
                    {
                        if(words.size() < 2)
                        {
                            return false;
                        }
                        auto& plugin = plugins.back();
                        auto const& entry = words.getReference(0);
                        auto const& value = words.getReference(1);
                        if(entry == "vamp:name")
                        {
                            plugin.name = value;
                        }
                        else if(entry == "dc:description")
                        {
                            plugin.pluginDescription = value;
                        }
                        else if(entry == "foaf:maker")
                        {
                            plugin.maker = shortcuts[value];
                        }
                        else if(entry == "vamp:vamp_API_version" && plugin.isCompatible)
                        {
                            plugin.isCompatible = value.contains("vamp:api_version_2");
                        }
                        return true;
                    });
    if(shouldQuit)
    {
        return {};
    }
#if JUCE_DEBUG
    for(auto const& plugin : plugins)
    {
        MiscDebug("PluginList::WebDownloader", "Plugin: " + plugin.name);
    }
#endif

    return plugins;
}

namespace
{
    // The name of the asset of the Partiels Vamp Plugins releases that contains the distribution list
    static auto constexpr distributionAssetName = "Partiels-Plugins.json";

    // The distribution list of a release of the Partiels Vamp Plugins
    struct Distribution
    {
        Release::Version minimumPartielsVersion; // Invalid if the plugins are compatible with all the versions of Partiels
        std::vector<Plugin::WebReference> references;
    };

    // The distribution list is either an object with the minimum version of Partiels required by the plugins
    // and the list of plugins ({"minimumPartielsVersion": "3.0.0", "plugins": [...]}), or only the list of
    // plugins (the legacy format that is compatible with all the versions of Partiels)
    Distribution parseDistribution(juce::String const& content)
    {
        nlohmann::json json;
        try
        {
            json = nlohmann::json::parse(content.toStdString());
        }
        catch(nlohmann::json::exception const&)
        {
            throw std::runtime_error("Unable to parse the plugin distribution list.");
        }
        Distribution distribution;
        auto const* plugins = std::addressof(json);
        if(json.is_object())
        {
            auto const versionIt = json.find("minimumPartielsVersion");
            if(versionIt != json.cend())
            {
                distribution.minimumPartielsVersion = versionIt->is_string() ? Release::Version::fromString(juce::String(versionIt->get<std::string>())) : Release::Version{};
                if(!distribution.minimumPartielsVersion.isValid())
                {
                    throw std::runtime_error("The minimum version of Partiels of the plugin distribution list is invalid.");
                }
            }
            auto const pluginsIt = json.find("plugins");
            plugins = pluginsIt != json.cend() ? std::addressof(*pluginsIt) : nullptr;
        }
        if(plugins == nullptr || !plugins->is_array())
        {
            throw std::runtime_error("The plugin distribution list is invalid.");
        }
        for(auto const& entry : *plugins)
        {
            Plugin::WebReference reference;
            try
            {
                reference = entry.get<Plugin::WebReference>();
            }
            catch(nlohmann::json::exception const&)
            {
                continue;
            }
            if(!reference.valid())
            {
                continue;
            }
            reference.isCompatible = reference.getDownloadUrl().isNotEmpty();
#if JUCE_DEBUG
            MiscDebug("PluginList::WebDownloader", "Distribution plugin: " + reference.name);
#endif
            distribution.references.push_back(std::move(reference));
        }
        if(distribution.references.empty())
        {
            throw std::runtime_error("The plugin distribution list is empty.");
        }
        return distribution;
    }

    // The pre-releases of Partiels are compatible with the plugins that require the version of the pre-release
    // (Partiels 3.0.0-beta1 is compatible with the plugins that require Partiels 3.0.0)
    bool isCompatible(Release::Version const& minimumPartielsVersion, Release::Version const& partielsVersion)
    {
        return !minimumPartielsVersion.isValid() || partielsVersion.withoutPreRelease() >= minimumPartielsVersion.withoutPreRelease();
    }

    juce::String downloadContent(juce::URL const& url, juce::String const& headers = {})
    {
        auto statusCode = 0;
        auto const options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                 .withConnectionTimeoutMs(5000)
                                 .withExtraHeaders(headers)
                                 .withStatusCode(&statusCode);
        auto stream = url.createInputStream(options);
        if(stream == nullptr || statusCode < 200 || statusCode >= 300)
        {
            throw std::runtime_error(juce::String("Unable to download URL (STATUS).").replace("URL", url.toString(true)).replace("STATUS", juce::String(statusCode)).toStdString());
        }
        MiscDebug("PluginList::WebDownloader", "URL: " + url.toString(true));
        return stream->readEntireStreamAsString();
    }
} // namespace

std::vector<Plugin::WebReference> PluginList::WebDownloader::downloadDistribution(std::atomic<bool> const& shouldQuit, bool includePreReleases)
{
    // The environment variable replaces the online distribution list by a local file (for example for testing purposes)
    if(auto const environmentPath = juce::SystemStats::getEnvironmentVariable("PARTIELS_PLUGINS_FILE", {}); environmentPath.isNotEmpty())
    {
        auto const file = juce::File(environmentPath);
        if(!file.existsAsFile())
        {
            throw std::runtime_error("The plugin distribution file does not exist.");
        }
        return parseDistribution(file.loadFileAsString()).references;
    }

    // The release of the Partiels Vamp Plugins associated with this version of Partiels when it was built
    // is used unless a newer release compatible with this version of Partiels is available
    static juce::String const associatedTag{PARTIELS_VAMP_PLUGINS_TAG};
    auto const associatedVersion = Release::Version::fromString(associatedTag);
    auto const partielsVersion = Release::Version::fromString(ProjectInfo::versionString);
    std::vector<Release::Info> releases;
    try
    {
        static juce::String const headers{"Accept: application/vnd.github+json\nX-GitHub-Api-Version: 2022-11-28"};
        releases = Release::parseList(downloadContent(juce::URL("https://api.github.com/repos/Ircam-Partiels/Partiels-Vamp-Plugins/releases?per_page=30"), headers), includePreReleases);
    }
    catch(std::runtime_error const& error)
    {
        MiscDebug("PluginList::WebDownloader", juce::String("Plugin release list download failed: ERROR").replace("ERROR", error.what()));
    }
    for(auto const& release : releases)
    {
        if(shouldQuit)
        {
            return {};
        }
        // The releases are sorted from the newest to the oldest
        if(release.version <= associatedVersion)
        {
            break;
        }
        auto const assetIt = release.assets.find(distributionAssetName);
        if(assetIt == release.assets.cend())
        {
            continue;
        }
        try
        {
            auto distribution = parseDistribution(downloadContent(juce::URL(assetIt->second)));
            if(isCompatible(distribution.minimumPartielsVersion, partielsVersion))
            {
                MiscDebug("PluginList::WebDownloader", "Plugin distribution: " + release.tag);
                return std::move(distribution.references);
            }
            MiscDebug("PluginList::WebDownloader", juce::String("Plugin distribution TAG requires Partiels VERSION").replace("TAG", release.tag).replace("VERSION", distribution.minimumPartielsVersion.toString()));
        }
        catch(std::runtime_error const& error)
        {
            MiscDebug("PluginList::WebDownloader", juce::String("Plugin distribution TAG ignored: ERROR").replace("TAG", release.tag).replace("ERROR", error.what()));
        }
    }
    if(shouldQuit)
    {
        return {};
    }

    MiscDebug("PluginList::WebDownloader", "Plugin distribution: " + associatedTag);
    juce::URL const associatedUrl("https://github.com/Ircam-Partiels/Partiels-Vamp-Plugins/releases/download/" + associatedTag + "/" + distributionAssetName);
    return parseDistribution(downloadContent(associatedUrl)).references;
}

std::vector<Plugin::WebReference> PluginList::WebDownloader::downloadRdf(std::atomic<bool> const& shouldQuit)
{
    juce::URL const indexUrl("https://www.vamp-plugins.org/rdf/plugins/index.txt");
    auto const options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress).withConnectionTimeoutMs(5000);
    auto stream = indexUrl.createInputStream(options);
    if(stream == nullptr)
    {
        throw std::runtime_error("Unable to download the plugin list index.");
    }
    if(shouldQuit)
    {
        return {};
    }

    auto const content = stream->readEntireStreamAsString();
    if(content.isEmpty())
    {
        throw std::runtime_error("The plugin list index is empty.");
    }
    if(shouldQuit)
    {
        return {};
    }

    std::vector<Plugin::WebReference> allDescriptions;
    auto const lines = juce::StringArray::fromLines(content);
    for(auto const& line : lines)
    {
        auto const subUrl = line.trim();
        if(subUrl.isNotEmpty() && !subUrl.startsWith("#"))
        {
            stream = juce::URL(subUrl).createInputStream(options);
            if(stream != nullptr)
            {
                MiscDebug("PluginList::WebDownloader", "URL: " + subUrl);
                auto const descriptions = parse(stream->readEntireStreamAsString(), shouldQuit);
                allDescriptions.insert(allDescriptions.end(), descriptions.cbegin(), descriptions.cend());
            }
            else
            {
                MiscDebug("PluginList::WebDownloader", "Failed to download plugin RDF from URL: " + subUrl);
            }
        }
        if(shouldQuit)
        {
            return {};
        }
    }
    return allDescriptions;
}

std::vector<Plugin::WebReference> PluginList::WebDownloader::download(std::atomic<bool> const& shouldQuit, bool includePreReleases)
{
    // The plugins of the Partiels distribution have priority over the plugins of the RDF list
    std::vector<Plugin::WebReference> distribution;
    try
    {
        distribution = downloadDistribution(shouldQuit, includePreReleases);
    }
    catch(std::runtime_error const& error)
    {
        MiscDebug("PluginList::WebDownloader", juce::String("Plugin distribution list download failed: ERROR").replace("ERROR", error.what()));
    }
    if(shouldQuit)
    {
        return {};
    }

    std::vector<Plugin::WebReference> allReferences;
    // The plugins of the RDF list are not compatible with the ARM processors, only the plugins of the Partiels distribution are listed on this system
#if !(JUCE_MAC && JUCE_ARM)
    try
    {
        auto const references = downloadRdf(shouldQuit);
        auto const isDistributed = [&distribution](Plugin::WebReference const& reference)
        {
            return std::any_of(distribution.cbegin(), distribution.cend(), [&reference](Plugin::WebReference const& distributed)
                               {
                                   return distributed.identifier == reference.identifier;
                               });
        };
        std::copy_if(references.cbegin(), references.cend(), std::back_inserter(allReferences), [&](Plugin::WebReference const& reference)
                     {
                         return !isDistributed(reference);
                     });
    }
    catch(std::runtime_error const& error)
    {
        MiscDebug("PluginList::WebDownloader", juce::String("Web plugin list download failed: ERROR").replace("ERROR", error.what()));
    }
    if(shouldQuit)
    {
        return {};
    }
#endif

    allReferences.insert(allReferences.end(), distribution.cbegin(), distribution.cend());
    return allReferences;
}

juce::File PluginList::WebDownloader::getInstallerDirectory()
{
    return juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory).getChildFile(juce::String(ProjectInfo::projectName) + "-Plugins");
}

std::tuple<juce::Result, juce::File> PluginList::WebDownloader::downloadPackage(Plugin::WebReference const& reference, std::atomic<bool> const& shouldAbort, std::atomic<double>& progress)
{
    auto const downloadUrl = reference.getDownloadUrl();
    if(downloadUrl.isEmpty())
    {
        return {juce::Result::fail(juce::translate("The plugin is not available for this system.")), juce::File{}};
    }
    // The download and the extraction are performed in a subdirectory of the plugin
    // that is removed before each download so that a previous version of the installer
    // never interferes with the new one
    auto const directory = getInstallerDirectory().getChildFile(reference.identifier);
    // The identifier comes from the distribution list, the directory must be a subdirectory
    // of the installer directory (the identifier must not be empty or a relative path)
    if(!reference.valid() || !directory.isAChildOf(getInstallerDirectory()))
    {
        return {juce::Result::fail(juce::translate("The plugin installer could not be downloaded.")), juce::File{}};
    }
    if(!directory.deleteRecursively() && directory.exists())
    {
        return {juce::Result::fail(juce::translate("The plugin installer could not be downloaded.")), juce::File{}};
    }
    if(directory.createDirectory().failed())
    {
        return {juce::Result::fail(juce::translate("The plugin installer could not be downloaded.")), juce::File{}};
    }
    auto fileName = juce::URL(downloadUrl).getFileName();
    if(fileName.isEmpty())
    {
        fileName = reference.identifier + "-package";
    }
    auto const file = directory.getChildFile(fileName);

    // Download the package into the temporary file
    progress.store(-1.0);
    auto statusCode = 0;
    auto const options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                             .withConnectionTimeoutMs(5000)
                             .withStatusCode(&statusCode)
                             .withProgressCallback([&shouldAbort, &progress](int bytesSent, int totalBytes)
                                                   {
                                                       progress.store(totalBytes > 0 ? static_cast<double>(bytesSent) / static_cast<double>(totalBytes) : -1.0);
                                                       return !shouldAbort.load();
                                                   });
    auto const downloadResult = Misc::Downloader::download(juce::URL(downloadUrl), file, options);
    // The download aborted by the user is not an error (the downloader fails in this case)
    if(shouldAbort)
    {
        return {juce::Result::ok(), juce::File{}};
    }
    if(!downloadResult.wasOk())
    {
        return {downloadResult, juce::File{}};
    }
    progress.store(1.0);
    if(statusCode < 200 || statusCode >= 300 || !file.existsAsFile() || file.getSize() == 0_z)
    {
        return {juce::Result::fail(juce::translate("The plugin installer could not be downloaded.")), juce::File{}};
    }

    // The zip packages contain the installer (a pkg on MacOS or an exe on Windows),
    // the other packages (a pkg, an exe, a sh script or a tar.gz archive) are returned as-is
    if(!file.hasFileExtension(".zip"))
    {
        // The downloaded installation script must be executable
        if(file.hasFileExtension(".sh"))
        {
            file.setExecutePermission(true);
        }
        return {juce::Result::ok(), file};
    }
    auto const destination = file.getSiblingFile(file.getFileNameWithoutExtension() + "-unpacked");
    if(!destination.deleteRecursively() && destination.exists())
    {
        return {juce::Result::fail(juce::translate("The plugin package could not be extracted.")), juce::File{}};
    }
    if(destination.createDirectory().failed())
    {
        return {juce::Result::fail(juce::translate("The plugin package could not be extracted.")), juce::File{}};
    }
    juce::ZipFile zipFile{file};
    auto const result = zipFile.uncompressTo(destination);
    if(!result.wasOk())
    {
        return {result, juce::File{}};
    }
#if JUCE_MAC
    static juce::String const installerPattern{"*.pkg"};
#elif JUCE_WINDOWS
    static juce::String const installerPattern{"*.exe"};
#else
    // The installation script is searched explicitly so that the uninstallation scripts are ignored
    static juce::String const installerPattern{"Install*.sh"};
#endif
    auto const files = destination.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true);
    auto const findExtracted = [&files](juce::String const& pattern)
    {
        juce::WildcardFileFilter const filter{pattern, {}, {}};
        auto const it = std::find_if(files.begin(), files.end(), [&filter](juce::File const& extracted)
                                     {
                                         return filter.isFileSuitable(extracted);
                                     });
        return it != files.end() ? juce::File{*it} : juce::File{};
    };
    auto const installer = findExtracted(installerPattern);
#if !(JUCE_MAC || JUCE_WINDOWS)
    // Otherwise, the archive of the package is returned so that the user can extract it before running its installation script
    if(!installer.existsAsFile())
    {
        auto const archive = findExtracted("*.tar.gz");
        if(archive.existsAsFile())
        {
            return {juce::Result::ok(), archive};
        }
    }
#endif
    if(!installer.existsAsFile())
    {
        return {juce::Result::fail(juce::translate("The plugin package does not contain any installer.")), juce::File{}};
    }
    // The installer must be launched by the application that then rescan the plugins
    return {juce::Result::ok(), installer};
}

void PluginList::WebDownloader::download(bool includePreReleases, std::function<void(juce::Result, std::vector<Plugin::WebReference>)> callback)
{
    if(mProcess.valid())
    {
        callback(juce::Result::fail(juce::translate("A download is already in progress.")), {});
        return;
    }
    mCallback = callback;
    mProcess = std::async([this, includePreReleases]()
                          {
                              juce::Thread::setCurrentThreadName("PluginList::WebDownloader");
                              juce::Result result = juce::Result::ok();
                              std::vector<Plugin::WebReference> plugins;
                              try
                              {
                                  plugins = download(mShouldQuit, includePreReleases);
                              }
                              catch(std::runtime_error const& error)
                              {
                                  result = juce::Result::fail(error.what());
                              }
                              catch(...)
                              {
                                  result = juce::Result::fail(juce::translate("An unexpected error occurred during plugin list download."));
                              }
                              triggerAsyncUpdate();
                              return std::make_tuple(result, plugins);
                          });
}

void PluginList::WebDownloader::download(Plugin::WebReference const& reference, std::function<void(juce::Result, juce::File)> callback)
{
    if(callback == nullptr)
    {
        return;
    }
    auto& packageDownload = *mPackageDownloads.emplace_back(std::make_unique<PackageDownload>());
    packageDownload.reference = reference;
    packageDownload.callback = std::move(callback);
    auto const shouldAbort = &packageDownload.shouldAbort;
    auto const progress = &packageDownload.progress;
    packageDownload.process = std::async([this, reference, shouldAbort, progress]()
                                         {
                                             juce::Thread::setCurrentThreadName("PluginList::WebDownloader");
                                             std::tuple<juce::Result, juce::File> results{juce::Result::fail(juce::translate("An unexpected error occurred during plugin package download.")), juce::File{}};
                                             try
                                             {
                                                 results = downloadPackage(reference, *shouldAbort, *progress);
                                             }
                                             catch(...)
                                             {
                                             }
                                             triggerAsyncUpdate();
                                             return results;
                                         });
}

void PluginList::WebDownloader::handleAsyncUpdate()
{
    // The update can be triggered by a package download while the plugin list is still being downloaded
    if(mProcess.valid() && mProcess.wait_for(std::chrono::seconds{0}) == std::future_status::ready)
    {
        auto const results = mProcess.get();
        // The callback is released before being called so that it can start a new download
        auto const callback = std::exchange(mCallback, nullptr);
        if(callback != nullptr)
        {
            callback(std::get<0>(results), std::get<1>(results));
        }
    }
    // The completed downloads are removed before calling the callbacks so that the downloads
    // are no longer in progress and so that the callbacks can safely start new downloads
    std::vector<std::tuple<std::function<void(juce::Result, juce::File)>, juce::Result, juce::File>> completedDownloads;
    for(auto it = mPackageDownloads.begin(); it != mPackageDownloads.end();)
    {
        if(it->get()->process.valid() && it->get()->process.wait_for(std::chrono::seconds{0}) == std::future_status::ready)
        {
            auto const results = it->get()->process.get();
            completedDownloads.emplace_back(std::move(it->get()->callback), std::get<0>(results), std::get<1>(results));
            it = mPackageDownloads.erase(it);
        }
        else
        {
            ++it;
        }
    }
    for(auto const& completedDownload : completedDownloads)
    {
        if(std::get<0>(completedDownload) != nullptr)
        {
            std::get<0>(completedDownload)(std::get<1>(completedDownload), std::get<2>(completedDownload));
        }
    }
}

class WebDownloaderTest
: public juce::UnitTest
{
public:
    WebDownloaderTest()
    : juce::UnitTest("WebDownloader", "PluginList")
    {
    }

    ~WebDownloaderTest() override = default;

    void runTest() override
    {
        auto const plugin = juce::String(R"({"identifier": "supervp", "name": "SuperVP", "downloadUrlLinux": "linux", "downloadUrlMacos": "macos", "downloadUrlWindows": "windows"})");

        beginTest("Distribution Legacy Format");
        {
            auto const distribution = parseDistribution("[" + plugin + R"(, {"name": "No identifier"}, {"identifier": 1}])");
            expect(!distribution.minimumPartielsVersion.isValid());
            expectEquals(static_cast<int>(distribution.references.size()), 1);
            if(!distribution.references.empty())
            {
                expectEquals(distribution.references.front().identifier, juce::String("supervp"));
                expect(distribution.references.front().isCompatible);
            }
        }

        beginTest("Distribution Format");
        {
            auto const distribution = parseDistribution(R"({"minimumPartielsVersion": "3.0.0", "plugins": [)" + plugin + "]}");
            expectEquals(distribution.minimumPartielsVersion.toString(), juce::String("3.0.0"));
            expectEquals(static_cast<int>(distribution.references.size()), 1);

            auto const expectFailure = [this](juce::String const& content)
            {
                auto failed = false;
                try
                {
                    [[maybe_unused]] auto const result = parseDistribution(content);
                }
                catch(std::runtime_error const&)
                {
                    failed = true;
                }
                expect(failed, content);
            };
            expectFailure("not json");
            expectFailure("[]");
            expectFailure(R"({"minimumPartielsVersion": "3.0.0"})");
            expectFailure(R"({"minimumPartielsVersion": "latest", "plugins": [)" + plugin + "]}");
            expectFailure(R"({"minimumPartielsVersion": 3, "plugins": [)" + plugin + "]}");
        }

        beginTest("Distribution Compatibility");
        {
            using Version = Release::Version;
            expect(isCompatible(Version{}, Version::fromString("2.6.0")));
            expect(!isCompatible(Version::fromString("3.0.0"), Version::fromString("2.6.0")));
            expect(isCompatible(Version::fromString("3.0.0"), Version::fromString("3.0.0")));
            expect(isCompatible(Version::fromString("3.0.0"), Version::fromString("3.0.0-beta14")));
            expect(isCompatible(Version::fromString("3.0.0"), Version::fromString("3.1.0")));
            expect(!isCompatible(Version::fromString("3.1.0"), Version::fromString("3.0.0")));
        }
    }
};

static WebDownloaderTest webDownloaderTest;

ANALYSE_FILE_END
