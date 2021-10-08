#pragma once

#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    namespace Tools
    {
        bool hasTrackAcsr(Accessor const& accessor, juce::String const& identifier);
        bool hasGroupAcsr(Accessor const& accessor, juce::String const& identifier);
        bool isTrackInGroup(Accessor const& accessor, juce::String const& identifier);

        Track::Accessor const& getTrackAcsr(Accessor const& accessor, juce::String const& identifier);
        Group::Accessor const& getGroupAcsr(Accessor const& accessor, juce::String const& identifier);
        Group::Accessor const& getGroupAcsrForTrack(Accessor const& accessor, juce::String const& identifier);

        Track::Accessor& getTrackAcsr(Accessor& accessor, juce::String const& identifier);
        Group::Accessor& getGroupAcsr(Accessor& accessor, juce::String const& identifier);
        Group::Accessor& getGroupAcsrForTrack(Accessor& accessor, juce::String const& identifier);

        size_t getTrackPosition(Accessor const& accessor, juce::String const& identifier);
        size_t getGroupPosition(Accessor const& accessor, juce::String const& identifier);

        std::optional<juce::String> getFocusedTrack(Accessor const& accessor);
        std::optional<juce::String> getFocusedGroup(Accessor const& accessor);

        std::unique_ptr<juce::Component> createTimeRangeEditor(Accessor& acsr);
    } // namespace Tools

    class LayoutNotifier
    {
    public:
        LayoutNotifier(juce::String const name, Accessor& accessor, std::function<void(void)> fn = nullptr);
        ~LayoutNotifier();

    private:
        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};

    public:
        std::function<void(void)> onLayoutUpdated = nullptr;

    private:
        juce::String const mName;
        std::vector<std::unique_ptr<Track::Accessor::SmartListener>> mTrackListeners;
        std::vector<std::unique_ptr<Group::Accessor::SmartListener>> mGroupListeners;
    };
} // namespace Document

ANALYSE_FILE_END
