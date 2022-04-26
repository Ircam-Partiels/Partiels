#pragma once

#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    namespace Tools
    {
        bool hasItem(Accessor const& accessor, juce::String const& identifier);
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
        size_t getItemPosition(Accessor const& accessor, juce::String const& identifier);

        std::unique_ptr<juce::Component> createTimeRangeEditor(Accessor& acsr);
    } // namespace Tools

    class LayoutNotifier
    {
    public:
        LayoutNotifier(juce::String const name, Accessor& accessor, std::function<void(void)> fn = nullptr, std::set<Group::AttrType> groupAttrs = {Group::AttrType::identifier}, std::set<Track::AttrType> trackAttrs = {Track::AttrType::identifier});
        ~LayoutNotifier();

    private:
        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};

    public:
        std::function<void(void)> onLayoutUpdated = nullptr;

    private:
        juce::String const mName;
        std::vector<std::unique_ptr<Track::Accessor::SmartListener>> mTrackListeners;
        std::set<Track::AttrType> mTrackAttributes;
        std::vector<std::unique_ptr<Group::Accessor::SmartListener>> mGroupListeners;
        std::set<Group::AttrType> mGroupAttributes;
    };
} // namespace Document

ANALYSE_FILE_END
