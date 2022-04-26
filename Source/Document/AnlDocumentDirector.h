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
        Track::Director const& getTrackDirector(juce::String const& identifier) const override;
        Track::Director& getTrackDirector(juce::String const& identifier) override;
        juce::UndoManager& getUndoManager();

        void sanitize(NotificationType const notification);

        void startAction();
        void endAction(ActionState state, juce::String const& name = {});

        std::optional<juce::String> addTrack(juce::String const groupIdentifer, size_t position, NotificationType const notification);
        bool removeTrack(juce::String const identifier, NotificationType const notification);
        std::optional<juce::String> addGroup(size_t position, NotificationType const notification);
        bool removeGroup(juce::String const identifier, NotificationType const notification);

        bool moveTrack(juce::String const groupIdentifier, size_t index, juce::String const trackIdentifier, NotificationType const notification);
        bool copyTrack(juce::String const groupIdentifier, size_t index, juce::String const trackIdentifier, NotificationType const notification);

        void setAlertCatcher(AlertWindow::Catcher* catcher);
        using PluginTableContainer = Track::Director::PluginTableContainer;
        void setPluginTable(PluginTableContainer* table);
        using LoaderSelectorContainer = Track::Director::LoaderSelectorContainer;
        void setLoaderSelector(LoaderSelectorContainer* selector);

    private:
        // FileWatcher
        void fileHasBeenRemoved(juce::File const& file) override;
        void fileHasBeenRestored(juce::File const& file) override;
        void fileHasBeenModified(juce::File const& file) override;

        void restoreAudioFile(juce::File const& file);
        void initializeAudioReaders(NotificationType notification);

        void updateMarkers(NotificationType notification);

        Accessor& mAccessor;
        juce::AudioFormatManager& mAudioFormatManager;
        juce::UndoManager& mUndoManager;
        Accessor mSavedState;

        std::vector<std::unique_ptr<Group::Director>> mGroups;
        std::vector<std::unique_ptr<Track::Director>> mTracks;
        std::optional<double> mSampleRate;
        double mDuration = 0.0;
        AlertWindow::Catcher* mAlertCatcher = nullptr;
        PluginTableContainer* mPluginTableContainer = nullptr;
        LoaderSelectorContainer* mLoaderSelectorContainer = nullptr;
        std::unique_ptr<juce::FileChooser> mFileChooser;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
        JUCE_DECLARE_WEAK_REFERENCEABLE(Director)
    };
} // namespace Document

ANALYSE_FILE_END
