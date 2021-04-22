#pragma once

#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    namespace Tools
    {
        bool hasTrackAcsr(Accessor const& accessor, juce::String const& identifier);
        bool hasGroupAcsr(Accessor const& accessor, juce::String const& identifier);

        Track::Accessor const& getTrackAcsr(Accessor const& accessor, juce::String const& identifier);
        Group::Accessor const& getGroupAcsr(Accessor const& accessor, juce::String const& identifier);

        Track::Accessor& getTrackAcsr(Accessor& accessor, juce::String const& identifier);
        Group::Accessor& getGroupAcsr(Accessor& accessor, juce::String const& identifier);

        size_t getTrackPosition(Accessor& accessor, juce::String const& identifier);
        size_t getGroupPosition(Accessor& accessor, juce::String const& identifier);

        std::optional<juce::String> getFocusedTrack(Accessor const& accessor);
        std::optional<juce::String> getFocusedGroup(Accessor const& accessor);
    } // namespace Tools
} // namespace Document

ANALYSE_FILE_END
