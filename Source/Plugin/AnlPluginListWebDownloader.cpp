#include "AnlPluginListWebDownloader.h"

ANALYSE_FILE_BEGIN

PluginList::WebDownloader::~WebDownloader()
{
    if(mProcess.valid())
    {
        mShouldQuit = true;
        mProcess.get();
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
#ifdef JUCE_DEBUG
    for(auto const& library : libraries)
    {
        anlDebug("PluginList::WebDownloader", "Library: " + library.first);
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
                                plugin.downloadUrl = library.url;
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
#ifdef JUCE_DEBUG
    for(auto const& plugin : plugins)
    {
        anlDebug("PluginList::WebDownloader", "Plugin: " + plugin.name);
    }
#endif

    return plugins;
}

std::vector<Plugin::WebReference> PluginList::WebDownloader::download(std::atomic<bool> const& shouldQuit)
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
                anlDebug("PluginList::WebDownloader", "URL: " + subUrl);
                auto const descriptions = parse(stream->readEntireStreamAsString(), shouldQuit);
                allDescriptions.insert(allDescriptions.end(), descriptions.cbegin(), descriptions.cend());
            }
            else
            {
                anlDebug("PluginList::WebDownloader", "Failed to download plugin RDF from URL: " + subUrl);
            }
        }
        if(shouldQuit)
        {
            return {};
        }
    }
    return allDescriptions;
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
}

ANALYSE_FILE_END
