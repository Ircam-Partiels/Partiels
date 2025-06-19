#pragma once

#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    namespace Tools
    {
        struct Clipboard
        {
            using ChannelData = Track::Result::ChannelData;
            using MultiChannelData = std::map<size_t, ChannelData>;

            std::map<juce::String, MultiChannelData> data;
            juce::Range<double> range;
        };

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

        void resizeItem(Accessor& accessor, juce::String const& itemIdentifier, int newHeight, int parentHeight);
        void resizeItems(Accessor& accessor, bool preserveRatio, int parentHeight);

        bool containsFrames(Accessor const& accessor, juce::Range<double> const& selection);
        bool matchesFrame(Accessor const& accessor, double const time);
        bool canBreak(Accessor const& accessor, double time);
        bool isClipboardEmpty(Accessor const& accessor, Clipboard const& clipboard);
        std::set<size_t> getEffectiveSelectedChannelsForTrack(Accessor const& accessor, Track::Accessor const& trackAcsr);

        std::unique_ptr<juce::Component> createTimeRangeEditor(Accessor& accessor);
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

    class FocusRestorer
    : public juce::UndoableAction
    {
    public:
        FocusRestorer(Accessor& accessor);

        ~FocusRestorer() override = default;

        // juce::UndoableAction
        bool perform() override;
        bool undo() override;

    protected:
        Accessor& mAccessor;
        std::map<juce::String, Track::FocusInfo> mTrackFocus;
        std::map<juce::String, Group::FocusInfo> mGroupFocus;
    };
} // namespace Document

ANALYSE_FILE_END
