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

        void startAction();
        void endAction(ActionState state, juce::String const& name = {});

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

    private:
        // FileWatcher
        void fileHasBeenRemoved(juce::File const& file) override;
        void fileHasBeenRestored(juce::File const& file) override;
        void fileHasBeenModified(juce::File const& file) override;

        void restoreAudioFile(juce::File const& file);
        void initializeAudioReaders(NotificationType notification);

        void updateMarkers(NotificationType notification);
        juce::String createNextUuid() const;

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

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
        JUCE_DECLARE_WEAK_REFERENCEABLE(Director)
    };
} // namespace Document

ANALYSE_FILE_END
