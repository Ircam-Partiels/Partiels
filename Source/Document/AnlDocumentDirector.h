#pragma once

#include "../Group/AnlGroupDirector.h"
#include "../Plugin/AnlPluginListTable.h"
#include "../Track/AnlTrackDirector.h"
#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Director
    : public Track::MultiDirector
    , private FileWatcher
    {
    public:
        Director(Accessor& accessor, juce::AudioFormatManager& audioFormatManager, juce::UndoManager& undoManager);
        ~Director() override;

        Accessor& getAccessor();
        juce::AudioFormatManager& getAudioFormatManager();
        Group::Director& getGroupDirector(juce::String const& identifier);
        Track::Director& getTrackDirector(juce::String const& identifier) override;

        void sanitize(NotificationType const notification);

        void startAction();
        void endAction(ActionState state, juce::String const& name = {});

        std::optional<juce::String> addTrack(juce::String const groupIdentifer, size_t position, NotificationType const notification);
        bool removeTrack(juce::String const identifier, NotificationType const notification);
        std::optional<juce::String> addGroup(size_t position, NotificationType const notification);
        bool removeGroup(juce::String const identifier, NotificationType const notification);

        bool moveTrack(juce::String const groupIdentifier, size_t index, juce::String const trackIdentifier, NotificationType const notification);
        bool copyTrack(juce::String const groupIdentifier, size_t index, juce::String const trackIdentifier, NotificationType const notification);

    private:
        // FileWatcher
        void fileHasBeenRemoved() override;
        void fileHasBeenModified() override;
        
        void initializeAudioReaders(NotificationType notification);

        Accessor& mAccessor;
        juce::AudioFormatManager& mAudioFormatManager;
        juce::UndoManager& mUndoManager;
        Accessor mSavedState;

        std::vector<std::unique_ptr<Group::Director>> mGroups;
        std::vector<std::unique_ptr<Track::Director>> mTracks;
        double mSampleRate = 44100.0;
        double mDuration = 0.0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
} // namespace Document

ANALYSE_FILE_END
