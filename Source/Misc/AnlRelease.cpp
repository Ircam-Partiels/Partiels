#include "AnlRelease.h"

ANALYSE_FILE_BEGIN

Release::Version Release::Version::fromString(juce::String const& text)
{
    auto const trimmed = text.trim();
    // The build metadata (after +) is ignored and the tags can be prefixed by v
    auto const fullVersion = trimmed.upToFirstOccurrenceOf("+", false, false).trimCharactersAtStart("vV");
    auto const coreText = fullVersion.upToFirstOccurrenceOf("-", false, false);
    auto const preRelease = fullVersion.fromFirstOccurrenceOf("-", false, false);
    if(fullVersion.contains("-") && preRelease.isEmpty())
    {
        return {};
    }
    auto const numbers = juce::StringArray::fromTokens(coreText, ".", {});
    if(numbers.size() != 3 || std::any_of(numbers.begin(), numbers.end(), [](juce::String const& number)
                                          {
                                              return number.isEmpty() || !number.containsOnly("0123456789");
                                          }))
    {
        return {};
    }
    return {numbers[0].getIntValue(), numbers[1].getIntValue(), numbers[2].getIntValue(), preRelease};
}

juce::String Release::Version::toString() const
{
    auto const core = juce::String(major) + "." + juce::String(minor) + "." + juce::String(patch);
    return preRelease.isEmpty() ? core : core + "-" + preRelease;
}

bool Release::Version::isValid() const
{
    return major != 0 || minor != 0 || patch != 0;
}

bool Release::Version::isPreRelease() const
{
    return preRelease.isNotEmpty();
}

Release::Version Release::Version::withoutPreRelease() const
{
    return {major, minor, patch, {}};
}

int Release::Version::compare(Version const& other) const
{
    auto const lhs = std::make_tuple(major, minor, patch);
    auto const rhs = std::make_tuple(other.major, other.minor, other.patch);
    if(lhs != rhs)
    {
        return lhs < rhs ? -1 : 1;
    }
    // A release is newer than its pre-releases
    if(preRelease.isEmpty() || other.preRelease.isEmpty())
    {
        return preRelease.isEmpty() == other.preRelease.isEmpty() ? 0 : (preRelease.isEmpty() ? 1 : -1);
    }
    return preRelease.compareNatural(other.preRelease);
}

bool Release::Version::operator<(Version const& other) const
{
    return compare(other) < 0;
}

bool Release::Version::operator>(Version const& other) const
{
    return compare(other) > 0;
}

bool Release::Version::operator<=(Version const& other) const
{
    return compare(other) <= 0;
}

bool Release::Version::operator>=(Version const& other) const
{
    return compare(other) >= 0;
}

bool Release::Version::operator==(Version const& other) const
{
    return compare(other) == 0;
}

bool Release::Version::operator!=(Version const& other) const
{
    return compare(other) != 0;
}

std::vector<Release::Info> Release::parseList(juce::String const& content, bool includePreReleases)
{
    nlohmann::json json;
    try
    {
        json = nlohmann::json::parse(content.toStdString());
    }
    catch(nlohmann::json::exception const&)
    {
        return {};
    }
    if(!json.is_array())
    {
        return {};
    }

    auto const getString = [](nlohmann::json const& object, char const* key) -> juce::String
    {
        auto const it = object.find(key);
        return it != object.cend() && it->is_string() ? juce::String(it->get<std::string>()) : juce::String();
    };
    auto const getBool = [](nlohmann::json const& object, char const* key)
    {
        auto const it = object.find(key);
        return it != object.cend() && it->is_boolean() && it->get<bool>();
    };

    std::vector<Info> releases;
    for(auto const& entry : json)
    {
        if(!entry.is_object() || getBool(entry, "draft"))
        {
            continue;
        }
        Info info;
        info.tag = getString(entry, "tag_name");
        info.version = Version::fromString(info.tag);
        info.pageUrl = getString(entry, "html_url");
        info.isPreRelease = getBool(entry, "prerelease") || info.version.isPreRelease();
        if(!info.version.isValid() || (info.isPreRelease && !includePreReleases))
        {
            continue;
        }
        if(auto const assetsIt = entry.find("assets"); assetsIt != entry.cend() && assetsIt->is_array())
        {
            for(auto const& asset : *assetsIt)
            {
                if(asset.is_object())
                {
                    auto const name = getString(asset, "name");
                    auto const url = getString(asset, "browser_download_url");
                    if(name.isNotEmpty() && url.isNotEmpty())
                    {
                        info.assets[name] = url;
                    }
                }
            }
        }
        releases.push_back(std::move(info));
    }
    std::stable_sort(releases.begin(), releases.end(), [](Info const& lhs, Info const& rhs)
                     {
                         return lhs.version > rhs.version;
                     });
    return releases;
}

class ReleaseTest
: public juce::UnitTest
{
public:
    ReleaseTest()
    : juce::UnitTest("Release", "Misc")
    {
    }

    ~ReleaseTest() override = default;

    void runTest() override
    {
        using Version = Release::Version;

        beginTest("Version Parsing");
        {
            expect(Version::fromString("3.0.0").toString() == "3.0.0");
            expect(Version::fromString("v3.0.0").toString() == "3.0.0");
            expect(Version::fromString("3.0.0-beta14").toString() == "3.0.0-beta14");
            expect(Version::fromString("3.0.0-beta14").isPreRelease());
            expect(Version::fromString("3.0.0-beta14").withoutPreRelease().toString() == "3.0.0");
            expect(Version::fromString("2.0.0-dev2+build").toString() == "2.0.0-dev2");
            expect(!Version::fromString("").isValid());
            expect(!Version::fromString("3.0").isValid());
            expect(!Version::fromString("3.0.0-").isValid());
            expect(!Version::fromString("3.x.0").isValid());
            expect(!Version::fromString("latest").isValid());
        }

        beginTest("Version Comparison");
        {
            expect(Version::fromString("2.6.0") < Version::fromString("3.0.0"));
            expect(Version::fromString("2.10.0") > Version::fromString("2.9.1"));
            expect(Version::fromString("3.0.0-beta14") < Version::fromString("3.0.0"));
            expect(Version::fromString("3.0.0-beta14") > Version::fromString("2.6.0"));
            expect(Version::fromString("3.0.0-beta9") < Version::fromString("3.0.0-beta14"));
            expect(Version::fromString("3.0.0-beta14") < Version::fromString("3.0.0-rc1"));
            expect(Version::fromString("v3.0.0") == Version::fromString("3.0.0"));
            expect(Version::fromString("3.0.0-beta14") != Version::fromString("3.0.0"));
        }

        beginTest("Release List");
        {
            auto const content = R"([
                {"tag_name": "2.0.0-dev2", "prerelease": true, "draft": false, "html_url": "page-dev2", "assets": [{"name": "List.json", "browser_download_url": "url-dev2"}]},
                {"tag_name": "2.0.0-dev3", "prerelease": false, "draft": true, "assets": []},
                {"tag_name": "1.0.0", "prerelease": false, "draft": false, "html_url": "page-1.0.0", "assets": [{"name": "List.json", "browser_download_url": "url-1.0.0"}]},
                {"tag_name": "1.1.0", "prerelease": false, "draft": false, "assets": []},
                {"tag_name": "nightly", "prerelease": false, "draft": false, "assets": []},
                "invalid"
            ])";

            auto const releases = Release::parseList(content, false);
            expectEquals(static_cast<int>(releases.size()), 2);
            if(releases.size() == 2)
            {
                expectEquals(releases[0].tag, juce::String("1.1.0"));
                expectEquals(releases[1].tag, juce::String("1.0.0"));
                expectEquals(releases[1].pageUrl, juce::String("page-1.0.0"));
                expectEquals(releases[1].assets.count("List.json"), 1_z);
                expectEquals(releases[1].assets.at("List.json"), juce::String("url-1.0.0"));
            }

            auto const allReleases = Release::parseList(content, true);
            expectEquals(static_cast<int>(allReleases.size()), 3);
            if(allReleases.size() == 3)
            {
                expectEquals(allReleases[0].tag, juce::String("2.0.0-dev2"));
                expect(allReleases[0].isPreRelease);
                expectEquals(allReleases[1].tag, juce::String("1.1.0"));
            }

            expect(Release::parseList("{}", true).empty());
            expect(Release::parseList("not json", true).empty());
        }
    }
};

static ReleaseTest releaseTest;

ANALYSE_FILE_END
