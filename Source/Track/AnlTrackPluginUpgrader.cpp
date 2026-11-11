#include "AnlTrackPluginUpgrader.h"
#include "../Plugin/AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

namespace
{
    // The parameters of a former version that selected the index of the result and that defined the number of results
    // (optional) that have been replaced with the voices of an output (the voice N corresponds to the index N and the
    // identifier of the first voice is the identifier of the former output)
    struct VoiceParameters
    {
        std::string index;
        std::string count;
    };

    // The rule that upgrades the tracks that use the plugins of a library with a version lower than the new version
    struct Rule
    {
        std::string formerLibrary;                                          // The identifier of the former library
        std::string newLibrary;                                             // The identifier of the new library (can be the same)
        unsigned int newVersion;                                            // The version of the plugins of the new library
        std::vector<std::pair<std::string, std::string>> renamedParameters; // The former and new identifiers of the renamed parameters
        std::vector<VoiceParameters> voiceParameters;                       // The parameters replaced with the voices of the outputs
    };

    std::vector<Rule> const& getRules()
    {
        // SuperVP 8 and PM2 6 renamed their libraries (supervp.dylib to ircamsvp.vamp and PM2.dylib to ircampm2.vamp),
        // replaced the Window Size parameter (in milliseconds) with the Window Duration parameter (the new Window Size
        // parameter in samples being an alias of the Window Duration parameter) and replaced the index parameters with
        // the voices of the outputs (Peak Index and Formant Index for SuperVP, Partial ID and Maximum Number of
        // Partials for PM2 since version 5)
        static std::vector<Rule> const rules{
            {"supervp", "ircamsvp", 8u, {{"windowsize", "windowduration"}}, {{"peakindex", ""}, {"formantindex", ""}}},
            {"pm2", "ircampm2", 6u, {{"windowsize", "windowduration"}}, {{"partialid", "maximumnumberofpartials"}}}};
        return rules;
    }

    // The upgrade of a track to the new version of a plugin
    struct Upgrade
    {
        juce::XmlElement* xml{nullptr};              // The XML of the track
        unsigned int newVersion{0u};                 // The minimum version of the new plugin
        Plugin::Key key;                             // The key of the new version (the feature is the first voice if the track uses a voice)
        Plugin::State state;                         // The state of the new version
        Plugin::VoiceCounts voiceCounts;             // The numbers of voices of the new version
        double sampleRate{44100.0};                  // The sample rate of the track
        std::optional<size_t> voiceIndex;            // The index of the voice that replaces the former index parameter
        std::optional<float> formerVoiceCount;       // The value of the former parameter that defined the number of results
        std::map<juce::String, juce::String> inputs; // The input tracks
        bool useInputResultsExtraThresholds{false};  // Whether the extra thresholds of the input tracks are used
        bool performsAnalysis{false};                // Whether the track performs the analysis (the results are not loaded from a file)
    };

    Rule const* getRule(juce::XmlElement const& trackXml)
    {
        if(trackXml.getChildByName("key") == nullptr)
        {
            return nullptr;
        }
        auto const key = XmlParser::fromXml(trackXml, "key", Plugin::Key{});
        auto const libraryName = juce::String(key.identifier).upToFirstOccurrenceOf(":", false, false).toStdString();
        auto const& rules = getRules();
        auto const it = std::find_if(rules.cbegin(), rules.cend(), [&](auto const& rule)
                                     {
                                         return libraryName == rule.formerLibrary;
                                     });
        return it != rules.cend() ? &(*it) : nullptr;
    }

    // The XML of the track can be in the format of a former version of the application (when the tracks of a document
    // are upgraded before being restored), only the key, the state, the description and the numbers of voices that
    // didn't change since are upgraded and the other elements are converted when the track is restored
    std::optional<Upgrade> prepareUpgrade(juce::XmlElement& trackXml)
    {
        auto const* rule = getRule(trackXml);
        if(rule == nullptr)
        {
            return {};
        }
        auto const* descriptionXml = trackXml.getChildByName("description");
        auto const version = descriptionXml != nullptr ? XmlParser::fromXml(*descriptionXml, "version", 0u) : 0u;
        if(version >= rule->newVersion)
        {
            return {};
        }

        Upgrade upgrade;
        upgrade.xml = &trackXml;
        upgrade.newVersion = rule->newVersion;
        upgrade.key = XmlParser::fromXml(trackXml, "key", Plugin::Key{});
        upgrade.key.identifier = rule->newLibrary + upgrade.key.identifier.substr(rule->formerLibrary.size());
        upgrade.state = XmlParser::fromXml(trackXml, "state", Plugin::State{});
        upgrade.voiceCounts = XmlParser::fromXml(trackXml, "voiceCounts", Plugin::VoiceCounts{});
        upgrade.inputs = XmlParser::fromXml(trackXml, "inputs", std::map<juce::String, juce::String>{});
        if(trackXml.getChildByName("inputs") == nullptr && trackXml.hasAttribute("input"))
        {
            // Before version 2.4.2, the track had a single input
            auto const description = XmlParser::fromXml(trackXml, "description", Plugin::Description{});
            if(!description.inputs.empty())
            {
                upgrade.inputs[description.inputs.front().identifier] = trackXml.getStringAttribute("input");
            }
        }
        upgrade.useInputResultsExtraThresholds = trackXml.getBoolAttribute("useInputResultsExtraThresholds", false);
        // Before version 0.0.10, the results file was stored in another format so the track is considered as not
        // performing the analysis to prevent the link
        auto const formatVersion = trackXml.getIntAttribute("MiscModelVersion", 0);
        upgrade.performsAnalysis = formatVersion > 0x9 && (trackXml.getChildByName("file") == nullptr || XmlParser::fromXml(trackXml, "file", Track::FileInfo{}).isEmpty());
        auto const sampleRate = trackXml.getDoubleAttribute("sampleRate", 0.0);
        if(sampleRate > 0.0)
        {
            upgrade.sampleRate = sampleRate;
        }

        // The renamed parameters keep their values (the renaming is performed in the order of the rule so a new
        // parameter can have the identifier of a former parameter)
        auto& parameters = upgrade.state.parameters;
        for(auto const& [formerIdentifier, newIdentifier] : rule->renamedParameters)
        {
            auto const it = parameters.find(formerIdentifier);
            if(it != parameters.end())
            {
                auto const value = it->second;
                parameters.erase(it);
                parameters[newIdentifier] = value;
            }
        }

        // The former index parameter is replaced by the voice of the same index (the number of voices is at least the
        // index) and the former feature is the first voice of the output
        auto const toCount = [](float value)
        {
            return static_cast<size_t>(std::max(std::round(value), 1.0f));
        };
        for(auto const& voiceParameter : rule->voiceParameters)
        {
            auto const indexIt = parameters.find(voiceParameter.index);
            if(indexIt == parameters.end())
            {
                continue;
            }
            auto const index = toCount(indexIt->second);
            auto count = index;
            parameters.erase(indexIt);
            if(!voiceParameter.count.empty())
            {
                auto const countIt = parameters.find(voiceParameter.count);
                if(countIt != parameters.end())
                {
                    upgrade.formerVoiceCount = countIt->second;
                    count = std::max(count, toCount(countIt->second));
                    parameters.erase(countIt);
                }
            }
            upgrade.voiceCounts[upgrade.key.feature] = count;
            upgrade.voiceIndex = index - 1_z;
        }
        return upgrade;
    }

    bool applyUpgrade(Upgrade const& upgrade, Track::PluginUpgrader::DescriptionLoader const& loader, juce::String const& voiceLink)
    {
        MiscWeakAssert(upgrade.xml != nullptr && loader != nullptr);
        if(upgrade.xml == nullptr || loader == nullptr)
        {
            return false;
        }

        auto key = upgrade.key;
        if(upgrade.voiceIndex.has_value())
        {
            auto const voices = loader(key, upgrade.sampleRate, upgrade.state, upgrade.voiceCounts).voices;
            if(upgrade.voiceIndex.value() >= voices.size() || voices.front() != key.feature)
            {
                return false;
            }
            key.feature = voices.at(upgrade.voiceIndex.value());
        }

        // The description of the new version is also used to check that the new version of the plugin is installed,
        // it is necessary to display the parameters even if the analysis is not performed
        auto const description = loader(key, upgrade.sampleRate, upgrade.state, upgrade.voiceCounts);
        if(description == Plugin::Description{} || description.version < upgrade.newVersion)
        {
            return false;
        }

        auto& xml = *upgrade.xml;
        for(auto const* name : {"key", "state", "description"})
        {
            if(auto* child = xml.getChildByName(name))
            {
                xml.removeChildElement(child, true);
            }
        }
        XmlParser::toXml(xml, "key", key);
        XmlParser::toXml(xml, "state", upgrade.state);
        XmlParser::toXml(xml, "description", description);
        XmlParser::toXml(xml, "voiceCounts", upgrade.voiceCounts);
        if(voiceLink.isNotEmpty())
        {
            XmlParser::toXml(xml, "voiceLink", voiceLink);
        }
        return true;
    }

    // Whether the tracks performed the same analysis with the former versions of the plugins and can be linked
    bool canBeLinked(Upgrade const& lhs, Upgrade const& rhs)
    {
        auto const isLinkable = [](Upgrade const& upgrade)
        {
            return upgrade.voiceIndex.has_value() && upgrade.performsAnalysis;
        };
        return isLinkable(lhs) && isLinkable(rhs) &&
               lhs.key == rhs.key &&
               lhs.state.blockSize == rhs.state.blockSize &&
               lhs.state.stepSize == rhs.state.stepSize &&
               lhs.state.windowType == rhs.state.windowType &&
               lhs.state.parameters == rhs.state.parameters &&
               lhs.formerVoiceCount == rhs.formerVoiceCount &&
               std::abs(lhs.sampleRate - rhs.sampleRate) <= std::numeric_limits<double>::epsilon() &&
               lhs.inputs == rhs.inputs &&
               lhs.useInputResultsExtraThresholds == rhs.useInputResultsExtraThresholds;
    }
} // namespace

Plugin::Description Track::PluginUpgrader::loadInstalledDescription(Plugin::Key const& key, double sampleRate, Plugin::State const& state, Plugin::VoiceCounts const& voiceCounts)
{
    try
    {
        return PluginList::Scanner::loadDescription(key, sampleRate, state, voiceCounts);
    }
    catch(...)
    {
    }
    return {};
}

bool Track::PluginUpgrader::upgradeTrack(juce::XmlElement& trackXml, DescriptionLoader const& loader)
{
    auto const upgrade = prepareUpgrade(trackXml);
    return upgrade.has_value() && applyUpgrade(upgrade.value(), loader, {});
}

void Track::PluginUpgrader::upgradeTracks(juce::XmlElement& documentXml, DescriptionLoader const& loader)
{
    std::vector<Upgrade> upgrades;
    for(auto* trackXml : documentXml.getChildWithTagNameIterator("tracks"))
    {
        auto upgrade = prepareUpgrade(*trackXml);
        if(upgrade.has_value())
        {
            upgrades.push_back(std::move(upgrade.value()));
        }
    }

    // The tracks of the voices of the same analysis are linked and share the highest number of voices
    std::vector<std::vector<size_t>> groups;
    for(auto index = 0_z; index < upgrades.size(); ++index)
    {
        auto const it = std::find_if(groups.begin(), groups.end(), [&](auto const& group)
                                     {
                                         return canBeLinked(upgrades.at(group.front()), upgrades.at(index));
                                     });
        if(it != groups.end())
        {
            it->push_back(index);
        }
        else if(upgrades.at(index).voiceIndex.has_value())
        {
            groups.push_back({index});
        }
    }
    std::vector<juce::String> voiceLinks(upgrades.size());
    for(auto const& group : groups)
    {
        if(group.size() < 2_z)
        {
            continue;
        }
        auto const voiceLink = juce::Uuid().toString();
        auto const& feature = upgrades.at(group.front()).key.feature;
        auto voiceCount = 1_z;
        for(auto const index : group)
        {
            voiceCount = std::max(voiceCount, upgrades.at(index).voiceCounts.at(feature));
        }
        for(auto const index : group)
        {
            upgrades.at(index).voiceCounts[feature] = voiceCount;
            voiceLinks.at(index) = voiceLink;
        }
    }

    for(auto index = 0_z; index < upgrades.size(); ++index)
    {
        applyUpgrade(upgrades.at(index), loader, voiceLinks.at(index));
    }
}

class TrackPluginUpgraderUnitTest
: public juce::UnitTest
{
public:
    TrackPluginUpgraderUnitTest()
    : juce::UnitTest("PluginUpgrader", "Track")
    {
    }

    ~TrackPluginUpgraderUnitTest() override = default;

    void runTest() override
    {
        using namespace Track;

        // The loader emulates the new versions of the plugins (the outputs with voices use 15 voices by default and
        // 100 voices at most)
        auto const loader = [](Plugin::Key const& key, [[maybe_unused]] double sampleRate, [[maybe_unused]] Plugin::State const& state, Plugin::VoiceCounts const& voiceCounts)
        {
            struct Output
            {
                std::string identifier;
                unsigned int version;
                bool hasVoices;
            };
            static std::map<std::string, Output> const plugins{
                {"ircamsvp:supervpspectrogramfft", {"spectrogram", 8u, false}},
                {"ircamsvp:supervppeak", {"peakfreq", 8u, true}},
                {"ircamsvp:supervpformantlpc", {"formant", 8u, true}},
                {"ircampm2:pm2harmonicpartialtracking", {"frequency", 6u, true}}};
            auto const it = plugins.find(key.identifier);
            if(it == plugins.cend())
            {
                return Plugin::Description{};
            }
            auto const& output = it->second;
            std::vector<std::string> voices;
            if(output.hasVoices)
            {
                auto const countIt = voiceCounts.find(output.identifier);
                auto const count = std::clamp(countIt != voiceCounts.cend() ? countIt->second : 15_z, 1_z, 100_z);
                for(auto index = 0_z; index < count; ++index)
                {
                    voices.push_back(index == 0_z ? output.identifier : output.identifier + std::to_string(index + 1_z));
                }
            }
            auto const hasFeature = output.hasVoices ? std::find(voices.cbegin(), voices.cend(), key.feature) != voices.cend() : key.feature == output.identifier;
            if(!hasFeature)
            {
                return Plugin::Description{};
            }
            Plugin::Description description;
            description.name = key.identifier;
            description.version = output.version;
            description.output.identifier = key.feature;
            description.voices = voices;
            if(key.identifier == "ircampm2:pm2harmonicpartialtracking")
            {
                description.inputs.resize(1_z);
                description.inputs.front().identifier = "frequency";
            }
            return description;
        };

        auto const notInstalledLoader = []([[maybe_unused]] Plugin::Key const& key, [[maybe_unused]] double sampleRate, [[maybe_unused]] Plugin::State const& state, [[maybe_unused]] Plugin::VoiceCounts const& voiceCounts)
        {
            return Plugin::Description{};
        };

        auto const createTrackXml = [](juce::String const& identifier, Plugin::Key const& key, unsigned int version, std::map<std::string, float> const& parameters, juce::String const& inputTrack = {})
        {
            Accessor accessor;
            Plugin::Description description;
            description.version = version;
            description.inputs.resize(1_z);
            description.inputs.front().identifier = "frequency";
            accessor.setAttr<AttrType::identifier>(identifier, NotificationType::synchronous);
            accessor.setAttr<AttrType::description>(description, NotificationType::synchronous);
            accessor.setAttr<AttrType::key>(key, NotificationType::synchronous);
            accessor.setAttr<AttrType::inputs>(std::map<juce::String, juce::String>{{"frequency", inputTrack}}, NotificationType::synchronous);
            accessor.setAttr<AttrType::sampleRate>(44100.0, NotificationType::synchronous);
            Plugin::State state;
            state.blockSize = 64_z;
            state.stepSize = 64_z;
            state.parameters = parameters;
            accessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
            return accessor.toXml("tracks");
        };

        auto const getKey = [](juce::XmlElement const& xml)
        {
            return XmlParser::fromXml(xml, "key", Plugin::Key{});
        };

        auto const getParameters = [](juce::XmlElement const& xml)
        {
            return XmlParser::fromXml(xml, "state", Plugin::State{}).parameters;
        };

        auto const getVoiceCounts = [](juce::XmlElement const& xml)
        {
            return XmlParser::fromXml(xml, "voiceCounts", Plugin::VoiceCounts{});
        };

        auto const getVersion = [](juce::XmlElement const& xml)
        {
            return XmlParser::fromXml(xml, "description", Plugin::Description{}).version;
        };

        beginTest("unrelated tracks");
        {
            for(auto const& [key, version] : std::vector<std::pair<Plugin::Key, unsigned int>>{{{"vamp-example-plugins:spectralcentroid", "linearcentroid"}, 2u}, {{"supervp:supervpspectrogramfft", "spectrogram"}, 8u}, {{"pm2:pm2harmonicpartialtracking", "frequency"}, 6u}, {{"ircamsvp:supervpspectrogramfft", "spectrogram"}, 8u}})
            {
                auto const original = createTrackXml("track", key, version, {{"windowsize", 1024.0f}});
                auto xml = std::make_unique<juce::XmlElement>(*original);
                expect(!PluginUpgrader::upgradeTrack(*xml, loader));
                expect(xml->isEquivalentTo(original.get(), false));
            }
        }

        beginTest("plugins not installed");
        {
            auto const original = createTrackXml("track", {"supervp:supervppeak", "peakfreq"}, 7u, {{"windowsize", 46.0f}, {"peakindex", 3.0f}});
            auto xml = std::make_unique<juce::XmlElement>(*original);
            expect(!PluginUpgrader::upgradeTrack(*xml, notInstalledLoader));
            expect(xml->isEquivalentTo(original.get(), false));

            // The installed version is older than the new version
            auto const formerVersionLoader = [&](Plugin::Key const& key, double sampleRate, Plugin::State const& state, Plugin::VoiceCounts const& voiceCounts)
            {
                auto description = loader(key, sampleRate, state, voiceCounts);
                description.version = 7u;
                return description;
            };
            expect(!PluginUpgrader::upgradeTrack(*xml, formerVersionLoader));
            expect(xml->isEquivalentTo(original.get(), false));
        }

        beginTest("window size");
        {
            auto xml = createTrackXml("track", {"supervp:supervpspectrogramfft", "spectrogram"}, 7u, {{"windowsize", 46.0f}, {"fftoversampling", 1.0f}});
            expect(PluginUpgrader::upgradeTrack(*xml, loader));
            expect(getKey(*xml) == Plugin::Key{"ircamsvp:supervpspectrogramfft", "spectrogram"});
            expect(getParameters(*xml) == std::map<std::string, float>{{"windowduration", 46.0f}, {"fftoversampling", 1.0f}});
            expect(getVoiceCounts(*xml).empty());
            expect(getVersion(*xml) == 8u);
            expect(xml->getStringAttribute("voiceLink").isEmpty());

            // The track is not upgraded twice
            auto const copy = std::make_unique<juce::XmlElement>(*xml);
            expect(!PluginUpgrader::upgradeTrack(*xml, loader));
            expect(xml->isEquivalentTo(copy.get(), false));
        }

        beginTest("index parameters");
        {
            auto peakXml = createTrackXml("peak", {"supervp:supervppeak", "peakfreq"}, 7u, {{"windowsize", 46.0f}, {"peakindex", 3.0f}});
            expect(PluginUpgrader::upgradeTrack(*peakXml, loader));
            expect(getKey(*peakXml) == Plugin::Key{"ircamsvp:supervppeak", "peakfreq3"});
            expect(getParameters(*peakXml) == std::map<std::string, float>{{"windowduration", 46.0f}});
            expect(getVoiceCounts(*peakXml) == Plugin::VoiceCounts{{"peakfreq", 3_z}});

            auto formantXml = createTrackXml("formant", {"supervp:supervpformantlpc", "formant"}, 7u, {{"formantindex", 1.0f}});
            expect(PluginUpgrader::upgradeTrack(*formantXml, loader));
            expect(getKey(*formantXml) == Plugin::Key{"ircamsvp:supervpformantlpc", "formant"});
            expect(getVoiceCounts(*formantXml) == Plugin::VoiceCounts{{"formant", 1_z}});

            auto partialXml = createTrackXml("partial", {"pm2:pm2harmonicpartialtracking", "frequency"}, 5u, {{"windowsize", 46.0f}, {"partialid", 2.0f}, {"maximumnumberofpartials", 5.0f}});
            expect(PluginUpgrader::upgradeTrack(*partialXml, loader));
            expect(getKey(*partialXml) == Plugin::Key{"ircampm2:pm2harmonicpartialtracking", "frequency2"});
            expect(getParameters(*partialXml) == std::map<std::string, float>{{"windowduration", 46.0f}});
            expect(getVoiceCounts(*partialXml) == Plugin::VoiceCounts{{"frequency", 5_z}});
            expect(getVersion(*partialXml) == 6u);

            // The index is greater than the maximum number of voices of the new version
            auto const original = createTrackXml("peak", {"supervp:supervppeak", "peakfreq"}, 7u, {{"peakindex", 150.0f}});
            auto invalidXml = std::make_unique<juce::XmlElement>(*original);
            expect(!PluginUpgrader::upgradeTrack(*invalidXml, loader));
            expect(invalidXml->isEquivalentTo(original.get(), false));
        }

        beginTest("linked tracks");
        {
            std::map<std::string, float> const pm2Parameters{{"windowsize", 46.0f}, {"maximumnumberofpartials", 4.0f}, {"amplitudethreshold", 90.0f}};
            auto const withParameter = [](std::map<std::string, float> parameters, std::string const& identifier, float value)
            {
                parameters[identifier] = value;
                return parameters;
            };

            juce::XmlElement documentXml("document");
            documentXml.addChildElement(createTrackXml("f0", {"supervp:supervpf0featurescoring", "fundamental"}, 7u, {{"windowsize", 46.0f}}).release());
            documentXml.addChildElement(createTrackXml("partial1", {"pm2:pm2harmonicpartialtracking", "frequency"}, 5u, withParameter(pm2Parameters, "partialid", 1.0f), "f0").release());
            documentXml.addChildElement(createTrackXml("partial2", {"pm2:pm2harmonicpartialtracking", "frequency"}, 5u, withParameter(pm2Parameters, "partialid", 2.0f), "f0").release());
            documentXml.addChildElement(createTrackXml("partial3", {"pm2:pm2harmonicpartialtracking", "frequency"}, 5u, withParameter(pm2Parameters, "partialid", 3.0f), "f0").release());
            documentXml.addChildElement(createTrackXml("partial3bis", {"pm2:pm2harmonicpartialtracking", "frequency"}, 5u, withParameter(pm2Parameters, "partialid", 3.0f), "f0").release());
            // Different state, different input, results file
            documentXml.addChildElement(createTrackXml("threshold", {"pm2:pm2harmonicpartialtracking", "frequency"}, 5u, withParameter(withParameter(pm2Parameters, "partialid", 2.0f), "amplitudethreshold", 60.0f), "f0").release());
            documentXml.addChildElement(createTrackXml("input", {"pm2:pm2harmonicpartialtracking", "frequency"}, 5u, withParameter(pm2Parameters, "partialid", 2.0f), "other").release());
            auto fileXml = createTrackXml("file", {"pm2:pm2harmonicpartialtracking", "frequency"}, 5u, withParameter(pm2Parameters, "partialid", 2.0f), "f0");
            fileXml->removeChildElement(fileXml->getChildByName("file"), true);
            XmlParser::toXml(*fileXml, "file", FileInfo{juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory).getChildFile("results.json")});
            documentXml.addChildElement(fileXml.release());
            // The highest index defines the number of voices
            documentXml.addChildElement(createTrackXml("peak2", {"supervp:supervppeak", "peakfreq"}, 7u, {{"peakindex", 2.0f}}).release());
            documentXml.addChildElement(createTrackXml("peak5", {"supervp:supervppeak", "peakfreq"}, 7u, {{"peakindex", 5.0f}}).release());
            documentXml.addChildElement(createTrackXml("formant", {"supervp:supervpformantlpc", "formant"}, 7u, {{"formantindex", 2.0f}}).release());
            documentXml.addChildElement(createTrackXml("centroid", {"vamp-example-plugins:spectralcentroid", "linearcentroid"}, 2u, {}).release());

            PluginUpgrader::upgradeTracks(documentXml, loader);

            std::map<juce::String, juce::XmlElement const*> tracks;
            std::vector<juce::String> order;
            for(auto const* trackXml : documentXml.getChildWithTagNameIterator("tracks"))
            {
                auto const identifier = trackXml->getStringAttribute("identifier");
                tracks[identifier] = trackXml;
                order.push_back(identifier);
            }
            expect(order == std::vector<juce::String>{"f0", "partial1", "partial2", "partial3", "partial3bis", "threshold", "input", "file", "peak2", "peak5", "formant", "centroid"});

            auto const getVoiceLink = [&](juce::String const& identifier)
            {
                return tracks.at(identifier)->getStringAttribute("voiceLink");
            };

            auto const partialLink = getVoiceLink("partial1");
            expect(partialLink.isNotEmpty());
            for(auto const& [identifier, feature] : std::vector<std::pair<juce::String, std::string>>{{"partial1", "frequency"}, {"partial2", "frequency2"}, {"partial3", "frequency3"}, {"partial3bis", "frequency3"}})
            {
                expectEquals(getVoiceLink(identifier), partialLink);
                expect(getKey(*tracks.at(identifier)) == Plugin::Key{"ircampm2:pm2harmonicpartialtracking", feature});
                expect(getVoiceCounts(*tracks.at(identifier)) == Plugin::VoiceCounts{{"frequency", 4_z}});
                expect(getParameters(*tracks.at(identifier)) == std::map<std::string, float>{{"windowduration", 46.0f}, {"amplitudethreshold", 90.0f}});
            }

            for(auto const& identifier : {"f0", "threshold", "input", "file", "formant", "centroid"})
            {
                expect(getVoiceLink(identifier).isEmpty());
            }
            expect(getKey(*tracks.at("threshold")) == Plugin::Key{"ircampm2:pm2harmonicpartialtracking", "frequency2"});
            expect(getKey(*tracks.at("input")) == Plugin::Key{"ircampm2:pm2harmonicpartialtracking", "frequency2"});
            expect(getKey(*tracks.at("file")) == Plugin::Key{"ircampm2:pm2harmonicpartialtracking", "frequency2"});
            expect(getKey(*tracks.at("formant")) == Plugin::Key{"ircamsvp:supervpformantlpc", "formant2"});
            expect(getKey(*tracks.at("centroid")) == Plugin::Key{"vamp-example-plugins:spectralcentroid", "linearcentroid"});
            // The F0 plugin is not installed
            expect(getKey(*tracks.at("f0")) == Plugin::Key{"supervp:supervpf0featurescoring", "fundamental"});

            auto const peakLink = getVoiceLink("peak2");
            expect(peakLink.isNotEmpty() && peakLink != partialLink);
            expectEquals(getVoiceLink("peak5"), peakLink);
            expect(getKey(*tracks.at("peak2")) == Plugin::Key{"ircamsvp:supervppeak", "peakfreq2"});
            expect(getKey(*tracks.at("peak5")) == Plugin::Key{"ircamsvp:supervppeak", "peakfreq5"});
            expect(getVoiceCounts(*tracks.at("peak2")) == Plugin::VoiceCounts{{"peakfreq", 5_z}});
            expect(getVoiceCounts(*tracks.at("peak5")) == Plugin::VoiceCounts{{"peakfreq", 5_z}});
        }

        beginTest("linked tracks of former formats");
        {
            // Before version 2.4.2, the track had a single input defined by an attribute
            auto const createFormerTrackXml = [&](juce::String const& identifier, float partialId, int formatVersion)
            {
                auto xml = createTrackXml(identifier, {"pm2:pm2harmonicpartialtracking", "frequency"}, 5u, {{"windowsize", 46.0f}, {"partialid", partialId}, {"maximumnumberofpartials", 2.0f}});
                xml->setAttribute("MiscModelVersion", formatVersion);
                while(auto* child = xml->getChildByName("inputs"))
                {
                    xml->removeChildElement(child, true);
                }
                xml->setAttribute("input", "f0");
                return xml;
            };

            juce::XmlElement documentXml("document");
            documentXml.addChildElement(createFormerTrackXml("partial1", 1.0f, 0x20400).release());
            documentXml.addChildElement(createFormerTrackXml("partial2", 2.0f, 0x20400).release());
            // Before version 0.0.10, the results file was stored in another format so the tracks are not linked
            documentXml.addChildElement(createFormerTrackXml("old1", 1.0f, 0x9).release());
            documentXml.addChildElement(createFormerTrackXml("old2", 2.0f, 0x9).release());
            PluginUpgrader::upgradeTracks(documentXml, loader);

            // The tracks are restored to convert the other elements to the current format
            std::map<juce::String, std::unique_ptr<Accessor>> accessors;
            for(auto const* trackXml : documentXml.getChildWithTagNameIterator("tracks"))
            {
                auto const identifier = trackXml->getStringAttribute("identifier");
                accessors[identifier] = std::make_unique<Accessor>();
                accessors[identifier]->fromXml(*trackXml, "tracks", NotificationType::synchronous);
            }

            auto const& partial1 = *accessors.at("partial1");
            auto const& partial2 = *accessors.at("partial2");
            expect(partial1.getAttr<AttrType::voiceLink>().isNotEmpty());
            expectEquals(partial2.getAttr<AttrType::voiceLink>(), partial1.getAttr<AttrType::voiceLink>());
            expect(partial1.getAttr<AttrType::key>() == Plugin::Key{"ircampm2:pm2harmonicpartialtracking", "frequency"});
            expect(partial2.getAttr<AttrType::key>() == Plugin::Key{"ircampm2:pm2harmonicpartialtracking", "frequency2"});
            for(auto const* accessor : {&partial1, &partial2})
            {
                expect(accessor->getAttr<AttrType::inputs>() == std::map<juce::String, juce::String>{{"frequency", "f0"}});
                expect(accessor->getAttr<AttrType::voiceCounts>() == Plugin::VoiceCounts{{"frequency", 2_z}});
                expect(accessor->getAttr<AttrType::state>().parameters == std::map<std::string, float>{{"windowduration", 46.0f}});
                expect(accessor->getAttr<AttrType::description>().version == 6u);
            }

            auto const& old1 = *accessors.at("old1");
            auto const& old2 = *accessors.at("old2");
            expect(old1.getAttr<AttrType::voiceLink>().isEmpty());
            expect(old2.getAttr<AttrType::voiceLink>().isEmpty());
            expect(old1.getAttr<AttrType::key>() == Plugin::Key{"ircampm2:pm2harmonicpartialtracking", "frequency"});
            expect(old2.getAttr<AttrType::key>() == Plugin::Key{"ircampm2:pm2harmonicpartialtracking", "frequency2"});
        }
    }
};

static TrackPluginUpgraderUnitTest trackPluginUpgraderUnitTest;

ANALYSE_FILE_END
