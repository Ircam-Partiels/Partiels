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
        [[maybe_unused]] auto const result = packageDownload.process.get();
    }
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

std::vector<Plugin::WebReference> PluginList::WebDownloader::downloadDistribution(std::atomic<bool> const& shouldQuit)
{
    static auto constexpr rootUrl = "https://github.com/Ircam-Partiels/Partiels/releases/download/";
    static auto constexpr fileName{"Partiels-Plugins.json"};
    juce::URL const fileUrl(juce::String(rootUrl) + juce::String(ProjectInfo::versionString) + "/" + fileName);
    auto const options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress).withConnectionTimeoutMs(5000);
    auto stream = fileUrl.createInputStream(options);
    if(stream == nullptr)
    {
        throw std::runtime_error("Unable to download the plugin distribution list.");
    }
    if(shouldQuit)
    {
        return {};
    }
    auto const content = stream->readEntireStreamAsString();
    if(shouldQuit)
    {
        return {};
    }
    auto json = nlohmann::json{};
    try
    {
        json = nlohmann::json::parse(content.toStdString());
    }
    catch(nlohmann::json::exception const&)
    {
        throw std::runtime_error("Unable to parse the plugin distribution list.");
    }
    if(!json.is_array())
    {
        throw std::runtime_error("The plugin distribution list is invalid.");
    }
    std::vector<Plugin::WebReference> references;
    for(auto const& entry : json)
    {
        auto reference = entry.get<Plugin::WebReference>();
        reference.isCompatible = reference.getDownloadUrl().isNotEmpty();
#if JUCE_DEBUG
        MiscDebug("PluginList::WebDownloader", "Distribution plugin: " + reference.name);
#endif
        references.push_back(std::move(reference));
    }
    if(references.empty())
    {
        throw std::runtime_error("The plugin distribution list is empty.");
    }
    MiscDebug("PluginList::WebDownloader", "URL: " + fileUrl.toString(true));
    return references;
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

std::vector<Plugin::WebReference> PluginList::WebDownloader::download(std::atomic<bool> const& shouldQuit)
{
    // The plugins of the Partiels distribution have priority over the plugins of the RDF list
    std::vector<Plugin::WebReference> distribution;
    try
    {
        distribution = downloadDistribution(shouldQuit);
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

    allReferences.insert(allReferences.end(), distribution.cbegin(), distribution.cend());
    return allReferences;
}

std::tuple<juce::Result, juce::File> PluginList::WebDownloader::downloadPackage(Plugin::WebReference const& reference, std::atomic<bool> const& shouldQuit)
{
    auto const downloadUrl = reference.getDownloadUrl();
    if(downloadUrl.isEmpty())
    {
        return {juce::Result::fail("The plugin is not available for this system."), juce::File{}};
    }
    auto const directory = juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory).getChildFile("Plugins");
    if(directory.createDirectory().failed())
    {
        return {juce::Result::fail("The plugin installer could not be downloaded."), juce::File{}};
    }
    auto fileName = juce::URL(downloadUrl).getFileName();
    if(fileName.isEmpty())
    {
        fileName = reference.identifier + "-package";
    }
    auto const file = directory.getChildFile(fileName);

    // Download the package into the temporary file
    auto statusCode = 0;
    auto const options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                             .withConnectionTimeoutMs(5000)
                             .withStatusCode(&statusCode)
                             .withProgressCallback([&shouldQuit](int, int)
                                                   {
                                                       return !shouldQuit.load();
                                                   });
    auto const downloadResult = Misc::Downloader::download(juce::URL(downloadUrl), file, options);
    if(!downloadResult.wasOk())
    {
        return {downloadResult, juce::File{}};
    }
    if(statusCode < 200 || statusCode >= 300 || !file.existsAsFile() || file.getSize() == 0_z)
    {
        return {juce::Result::fail("The plugin installer could not be downloaded."), juce::File{}};
    }

    // The zip packages contain the installer (a pkg on MacOS or an exe on Windows),
    // the other packages (a pkg, an exe or a tar.gz archive) are returned as-is
    if(!file.hasFileExtension(".zip"))
    {
        return {juce::Result::ok(), file};
    }
    auto const destination = file.getSiblingFile(file.getFileNameWithoutExtension() + "-unpacked");
    if(!destination.deleteRecursively() && destination.exists())
    {
        return {juce::Result::fail("The plugin package could not be extracted."), juce::File{}};
    }
    if(destination.createDirectory().failed())
    {
        return {juce::Result::fail("The plugin package could not be extracted."), juce::File{}};
    }
    juce::ZipFile zipFile{file};
    auto const result = zipFile.uncompressTo(destination);
    if(!result.wasOk())
    {
        return {result, juce::File{}};
    }
#if JUCE_MAC
    static juce::WildcardFileFilter const installerFilter{"*.pkg", {}, {}};
#elif JUCE_WINDOWS
    static juce::WildcardFileFilter const installerFilter{"*.exe", {}, {}};
#else
    // The installation script is searched explicitly so that the uninstallation scripts are ignored
    static juce::WildcardFileFilter const installerFilter{"Install*.sh", {}, {}};
#endif
    auto const files = destination.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true);
    auto const installerIt = std::find_if(files.begin(), files.end(), [](juce::File const& extracted)
                                          {
                                              return installerFilter.isFileSuitable(extracted);
                                          });
    if(installerIt == files.end())
    {
        return {juce::Result::fail("The plugin package does not contain any installer."), juce::File{}};
    }
    return {juce::Result::ok(), *installerIt};
}

void PluginList::WebDownloader::download(std::function<void(juce::Result, std::vector<Plugin::WebReference>)> callback)
{
    if(mProcess.valid())
    {
        callback(juce::Result::fail(juce::translate("A download is already in progress.")), {});
        return;
    }
    mCallback = callback;
    mProcess = std::async([this]()
                          {
                              juce::Thread::setCurrentThreadName("PluginList::WebDownloader");
                              juce::Result result = juce::Result::ok();
                              std::vector<Plugin::WebReference> plugins;
                              try
                              {
                                  plugins = download(mShouldQuit);
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
    auto& packageDownload = mPackageDownloads.emplace_back();
    packageDownload.callback = std::move(callback);
    packageDownload.process = std::async([this, reference]()
                                         {
                                             juce::Thread::setCurrentThreadName("PluginList::WebDownloader");
                                             std::tuple<juce::Result, juce::File> results{juce::Result::fail(juce::translate("An unexpected error occurred during plugin package download.")), juce::File{}};
                                             try
                                             {
                                                 results = downloadPackage(reference, mShouldQuit);
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
    if(mProcess.valid())
    {
        auto const results = mProcess.get();
        if(mCallback != nullptr)
        {
            mCallback(std::get<0>(results), std::get<1>(results));
            mCallback = nullptr;
        }
    }
    for(auto it = mPackageDownloads.begin(); it != mPackageDownloads.end();)
    {
        if(it->process.valid() && it->process.wait_for(std::chrono::seconds{0}) == std::future_status::ready)
        {
            auto const results = it->process.get();
            if(it->callback != nullptr)
            {
                it->callback(std::get<0>(results), std::get<1>(results));
            }
            it = mPackageDownloads.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

ANALYSE_FILE_END
