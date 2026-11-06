#pragma once

#include "../Plugin/AnlPluginListTable.h"
#include "AnlTrackGraphics.h"
#include "AnlTrackHierarchyManager.h"
#include "AnlTrackLoader.h"
#include "AnlTrackModel.h"
#include "AnlTrackProcessor.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    struct SafeAccessorRetriever
    {
        std::function<Accessor&()> getAccessorFn;
        std::function<Zoom::Accessor&()> getTimeZoomAccessorFn;
        std::function<Transport::Accessor&()> getTransportAccessorFn;
    };

    class Director
    : private FileWatcher
    , private juce::Timer
    {
    public:
        Director(Accessor& accessor, juce::UndoManager& undoManager, HierarchyManager& hierarchyManager, std::unique_ptr<juce::AudioFormatReader> audioFormatReader);
        ~Director() override;

        Accessor& getAccessor();
        juce::UndoManager& getUndoManager();
        HierarchyManager& getHierarchyManager();

        std::function<Accessor&()> getSafeAccessorFn();
        std::function<Zoom::Accessor&()> getSafeTimeZoomAccessorFn();
        std::function<Transport::Accessor&()> getSafeTransportZoomAccessorFn();
        void setSafeAccessorRetriever(SafeAccessorRetriever const& sav);

        void resetSavedState();
        bool hasChanged() const;
        bool isPerformingAction() const;
        void startAction();
        void endAction(ActionState state, juce::String const& name = {});

        void setGlobalValueRange(juce::Range<double> const& range, NotificationType const notification);

        void setAudioFormatReader(std::unique_ptr<juce::AudioFormatReader> audioFormatReader, NotificationType const notification);
        void setBackupDirectory(juce::File const& directory);
        void setSilentResultsFileManagement(bool state);
        void setPreserveFullDurationWhenEditing(bool state);
        bool isPreserveFullDurationWhenEditingEnabled() const;

        std::function<void(NotificationType notification)> onIdentifierUpdated = nullptr;
        std::function<void(NotificationType notification)> onNameUpdated = nullptr;
        std::function<void(NotificationType notification)> onInputUpdated = nullptr;
        std::function<void(NotificationType notification)> onResultsUpdated = nullptr;
        std::function<void(NotificationType notification)> onExtraThresholdsUpdated = nullptr;
        std::function<void(NotificationType notification)> onChannelsLayoutUpdated = nullptr;
        std::function<void(NotificationType notification)> onFocusUpdated = nullptr;

        //! @brief Gets if the track is linked to other tracks (the voices of the same analysis)
        bool isLinked() const;

        //! @brief Runs the analysis shared by the linked tracks
        //! @details The track performs the analysis of all the features (the voices) of the linked tracks, the
        //! results of the other features are notified with onLinkedAnalysisEnded.
        //! @param features The features of the linked tracks
        //! @return True if the analysis has been launched
        bool runLinkedAnalysis(std::vector<std::string> const& features);

        //! @brief Gets the advancement of the analysis performed by the track (if running)
        std::optional<float> getAnalysisAdvancement() const;

        //! @brief Sets the function that gets the advancement of the analysis performed by another linked track
        void setLinkedAnalysisAdvancementFn(std::function<std::optional<float>()> fn);

        //! @brief Requests the analysis shared by the linked tracks (if the track is linked)
        void requestLinkedAnalysis();

        //! @brief Sets the results of the analysis performed by another linked track
        void setLinkedAnalysisResults(juce::Result const& result, Processor::FeatureResult const& featureResult);

        //! @brief Requests the change of the number of voices of the linked tracks
        //! @details The change is performed by the document (with onVoiceCountRequested) because it adds or removes
        //! linked tracks (potentially this track).
        //! @param count The new number of voices
        void requestVoiceCount(size_t count);

        //! @brief Called when the analysis of the linked tracks is requested
        std::function<void(NotificationType notification)> onLinkedAnalysisRequested = nullptr;
        //! @brief Called when the track ends the analysis shared by the linked tracks
        std::function<void(juce::Result const& result, std::vector<Processor::FeatureResult> const& featureResults)> onLinkedAnalysisEnded = nullptr;
        //! @brief Called when the track stops the analysis of the linked tracks because it is no more linked to them
        std::function<void(juce::String const& voiceLink)> onLinkedAnalysisInterrupted = nullptr;
        //! @brief Called when the change of the number of voices of the linked tracks is requested
        std::function<void(size_t count)> onVoiceCountRequested = nullptr;
        //! @brief Called when an attribute shared by the linked tracks is updated
        std::function<void(AttrType attribute, NotificationType notification)> onLinkedAttrUpdated = nullptr;
        //! @brief Called when an action starts on a linked track (the action must be managed by the document)
        std::function<void()> onLinkedActionStarted = nullptr;
        //! @brief Called when an action ends on a linked track (the action must be managed by the document)
        std::function<void(ActionState state, juce::String const& name)> onLinkedActionEnded = nullptr;

        void setAlertCatcher(AlertWindow::Catcher* catcher);
        void setPluginTable(PluginList::Table* table, std::function<void(bool)> showHideFn);

        void warmAboutPlugin(juce::String const& reason);
        void askToReloadPlugin(juce::String const& reason);
        void askToRestoreState(juce::String const& reason);
        void askToReloadFile(juce::String const& reason);
        void askToRemoveFile();
        void askToResolveWarnings();

        bool isFileModified() const;

    private:
        void sanitizeZooms(NotificationType const notification);
        void sanitizeExtraOutputs(NotificationType const notification);
        void runAnalysis(NotificationType const notification);
        bool launchAnalysis(std::vector<std::string> const& features, NotificationType const notification);
        void applyAnalysisResults(juce::Result const& result, Results const& results, Plugin::Description const& description, bool showWarning);
        void notifyLinkedAttrUpdated(AttrType attribute, NotificationType const notification);
        void runLoading();
        void runRendering();
        void askForFile();
        void deleteBackup() const;
        void saveBackup() const;
        juce::File getEffectiveFile() const;

        // FileWatcher
        void fileHasBeenRemoved(juce::File const& file) override;
        void fileHasBeenRestored(juce::File const& file) override;
        void fileHasBeenModified(juce::File const& file) override;

        // juce::Timer
        void timerCallback() override;

        Accessor& mAccessor;
        juce::UndoManager& mUndoManager;
        HierarchyManager& mHierarchyManager;
        Accessor mSavedState;
        bool mIsPerformingAction{false};
        bool mIsPerformingLinkedAction{false};
        bool mIsLinkedAnalysisPending{false};
        bool mIsRunningLinkedAnalysis{false};
        juce::String mVoiceLink;
        std::function<std::optional<float>()> mLinkedAnalysisAdvancementFn;
        std::unique_ptr<juce::AudioFormatReader> mAudioFormatReader;
        Processor mProcessor;
        Loader mLoader;
        Graphics mGraphics;
        std::optional<ColourMap> mLastColourMap;
        std::optional<std::reference_wrapper<Zoom::Accessor>> mSharedZoomAccessor;
        Zoom::Accessor::Listener mSharedZoomListener{typeid(*this).name()};
        std::mutex mSharedZoomMutex;
        AlertWindow::Catcher* mAlertCatcher = nullptr;
        PluginList::Table* mPluginTable{nullptr};
        std::function<void(bool)> mPluginTableShowHideFn{nullptr};
        std::unique_ptr<juce::FileChooser> mFileChooser;
        SafeAccessorRetriever mSafeAccessorRetriever;
        juce::File mBackupDirectory;
        bool mSilentResultsFileManagement{false};
        bool mIsPreserveFullDurationWhenEditingEnabled{false};
        HierarchyManager::Listener mHierarchyListener;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
        JUCE_DECLARE_WEAK_REFERENCEABLE(Director)
    };

    class MultiDirector
    {
    public:
        virtual ~MultiDirector() = default;
        virtual Director const& getTrackDirector(juce::String const& identifier) const = 0;
        virtual Director& getTrackDirector(juce::String const& identifier) = 0;
    };
} // namespace Track

ANALYSE_FILE_END
