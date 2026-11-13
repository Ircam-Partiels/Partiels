#pragma once

#include "AnlBase.h"

ANALYSE_FILE_BEGIN

namespace Release
{
    //! @brief The version of a release with an optional pre-release suffix (for example 3.0.0 or 3.0.0-beta14)
    //! @details A release is newer than its pre-releases and the pre-release suffixes are compared
    //! with a natural order (so 3.0.0-beta9 is older than 3.0.0-beta14).
    struct Version
    {
        int major{0};
        int minor{0};
        int patch{0};
        juce::String preRelease;

        //! @brief Parse a version from a text such as 3.0.0, v3.0.0 or 3.0.0-beta14
        //! @param text The text to parse
        //! @return The version or an invalid version if the text is not a valid version
        static Version fromString(juce::String const& text);

        //! @brief Get the text representation of the version
        //! @return The text representation of the version (for example 3.0.0-beta14)
        juce::String toString() const;

        //! @brief Check if the version is valid
        //! @return True if the version is not 0.0.0
        bool isValid() const;

        //! @brief Check if the version is a pre-release
        //! @return True if the version has a pre-release suffix
        bool isPreRelease() const;

        //! @brief Get the version without the pre-release suffix
        //! @return The version without the pre-release suffix (for example 3.0.0 for 3.0.0-beta14)
        Version withoutPreRelease() const;

        //! @brief Compare the version with another version
        //! @param other The other version
        //! @return A negative value if the version is older, zero if the versions are equal, a positive value if the version is newer
        int compare(Version const& other) const;

        bool operator<(Version const& other) const;
        bool operator>(Version const& other) const;
        bool operator<=(Version const& other) const;
        bool operator>=(Version const& other) const;
        bool operator==(Version const& other) const;
        bool operator!=(Version const& other) const;
    };

    //! @brief The information of a release published on GitHub
    struct Info
    {
        Version version;
        juce::String tag;
        juce::String pageUrl;
        bool isPreRelease{false};
        std::map<juce::String, juce::String> assets; //!< The download URLs of the assets by name
    };

    //! @brief Parse a release list of the GitHub API (https://api.github.com/repos/OWNER/REPO/releases)
    //! @details The drafts, the releases whose tag is not a valid version and, if not included, the pre-releases are ignored.
    //! @param content The JSON content of the release list
    //! @param includePreReleases If the pre-releases are included
    //! @return The releases sorted from the newest to the oldest version
    std::vector<Info> parseList(juce::String const& content, bool includePreReleases);
} // namespace Release

ANALYSE_FILE_END
