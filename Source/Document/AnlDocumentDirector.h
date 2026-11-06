#pragma once

#include "../Group/AnlGroupDirector.h"
#include "../Plugin/AnlPluginListTable.h"
#include "../Track/AnlTrackDirector.h"
#include "AnlDocumentHierarchyManager.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Director
    : public Track::MultiDirector
    , private FileWatcher
    , private juce::AsyncUpdater
    {
    public:
        Director(Accessor& accessor, juce::AudioFormatManager& audioFormatManager, juce::UndoManager& undoManager);
        ~Director() override;

        Accessor& getAccessor();
        HierarchyManager& getHierarchyManager();
        juce::AudioFormatManager& getAudioFormatManager();
        Group::Director& getGroupDirector(juce::String const& identifier);
        Track::Director const& getTrackDirector(juce::String const& identifier) const override;
        Track::Director& getTrackDirector(juce::String const& identifier) override;
        juce::UndoManager& getUndoManager();

        std::function<Track::Accessor&()> getSafeTrackAccessorFn(juce::String const& identifier);
        std::function<Group::Accessor&()> getSafeGroupAccessorFn(juce::String const& identifier);
        std::function<Zoom::Accessor&()> getSafeTimeZoomAccessorFn();
        std::function<Transport::Accessor&()> getSafeTransportZoomAccessorFn();

        [[nodiscard]] std::map<juce::String, juce::String> sanitize(NotificationType const notification);

        void resetSavedState();
        bool hasChanged() const;
        bool isPerformingAction() const;
        void startAction();
        void endAction(ActionState state, juce::String const& name = {});

        //! @brief Changes the number of voices of linked tracks (the voices of an analysis)
        //! @details The number of voices is clamped to the range of the plugin. The tracks of the new voices are added
        //! above the track of the highest voice (as copies of it) from the bottom to the top and the tracks of the voices
        //! that no longer exist are removed.
        //! The voices that existed but whose tracks have been removed are not restored. The method should be called
        //! during an action and not by a component or a function that belongs to a linked track because the track
        //! can be removed.
        //! @param voiceLink The identifier shared by the linked tracks
        //! @param count The new number of voices
        //! @param notification The type of notification
        //! @return The result of the operation
        juce::Result setVoiceCount(juce::String const& voiceLink, size_t count, NotificationType const notification);

        std::tuple<juce::Result, juce::String> addTrack(juce::String const groupIdentifier, size_t position, NotificationType const notification);
        juce::Result removeTrack(juce::String const identifier, NotificationType const notification);
        juce::Result moveTrack(juce::String const groupIdentifier, size_t index, juce::String const trackIdentifier, NotificationType const notification);
        std::tuple<juce::Result, juce::String> copyTrack(juce::String const groupIdentifier, size_t index, juce::String const trackIdentifier, NotificationType const notification);

        std::tuple<juce::Result, juce::String> addGroup(size_t position, NotificationType const notification);
        juce::Result removeGroup(juce::String const identifier, NotificationType const notification);
        juce::Result moveGroup(juce::String const groupIdentifier, size_t index, NotificationType const notification);
        std::tuple<juce::Result, juce::String> copyGroup(juce::String const groupIdentifier, size_t index, NotificationType const notification);

        void setAlertCatcher(AlertWindow::Catcher* catcher);
        void setFileMapper(juce::File const& saved, juce::File const& current);
        void setPluginTable(PluginList::Table* table, std::function<void(bool)> showHideFn);
        void setBackupDirectory(juce::File const& directory);
        void setSilentResultsFileManagement(bool state);
        void setPreserveFullDurationWhenEditing(bool state);
        bool isPreserveFullDurationWhenEditingEnabled() const;

        class LinkedTracksUnitTest;

    private:
        // FileWatcher
        void fileHasBeenRemoved(juce::File const& file) override;
        void fileHasBeenRestored(juce::File const& file) override;
        void fileHasBeenModified(juce::File const& file) override;

        void restoreAudioFile(juce::File const& file);
        void initializeAudioReaders(NotificationType notification);

        void updateMarkers(NotificationType notification);
        juce::String createNextUuid() const;

        // Linked tracks (the voices of the same analysis)
        std::vector<std::reference_wrapper<Track::Director>> getLinkedTrackDirectors(juce::String const& voiceLink);
        void synchronizeLinkedTracks(Track::Director& source, Track::AttrType attribute, NotificationType notification);
        void dispatchLinkedAnalysisResults(Track::Director& source, juce::Result const& result, std::vector<Track::Processor::FeatureResult> const& featureResults);
        void runLinkedAnalyses();

        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        Accessor& mAccessor;
        HierarchyManager mHierarchyManager{mAccessor};
        juce::AudioFormatManager& mAudioFormatManager;
        juce::UndoManager& mUndoManager;
        Accessor mSavedState;

        std::vector<std::unique_ptr<Group::Director>> mGroups;
        std::vector<std::unique_ptr<Track::Director>> mTracks;
        double mDuration = 0.0;
        AlertWindow::Catcher* mAlertCatcher = nullptr;
        PluginList::Table* mPluginTable{nullptr};
        std::function<void(bool)> mPluginTableShowHideFn{nullptr};
        std::unique_ptr<juce::FileChooser> mFileChooser;
        juce::File mBackupDirectory;
        bool mSilentResultsFileManagement{false};
        bool mIsPreserveFullDurationWhenEditingEnabled{false};
        std::pair<juce::File, juce::File> mFileMapper;
        bool mIsPerformingAction{false};
        std::set<juce::String> mLinkedAnalysisRequests;
        size_t mLinkedActionDepth{0_z};
        bool mIsLinkedActionStarted{false};
        bool mIsSynchronizingLinkedTracks{false};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
        JUCE_DECLARE_WEAK_REFERENCEABLE(Director)
    };
} // namespace Document

ANALYSE_FILE_END
