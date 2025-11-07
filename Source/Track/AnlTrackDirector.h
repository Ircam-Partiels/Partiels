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

        bool hasChanged() const;
        void startAction();
        void endAction(ActionState state, juce::String const& name = {});

        void setGlobalValueRange(juce::Range<double> const& range, NotificationType const notification);

        void setAudioFormatReader(std::unique_ptr<juce::AudioFormatReader> audioFormatReader, NotificationType const notification);
        void setBackupDirectory(juce::File const& directory);
        void setSilentResultsFileManagement(bool state);

        std::function<void(NotificationType notification)> onIdentifierUpdated = nullptr;
        std::function<void(NotificationType notification)> onNameUpdated = nullptr;
        std::function<void(NotificationType notification)> onInputUpdated = nullptr;
        std::function<void(NotificationType notification)> onResultsUpdated = nullptr;
        std::function<void(NotificationType notification)> onChannelsLayoutUpdated = nullptr;

        void setAlertCatcher(AlertWindow::Catcher* catcher);
        void setPluginTable(PluginList::Table* table, std::function<void(bool)> showHideFn);
        void setLoaderSelector(Loader::ArgumentSelector* selector, std::function<void(bool)> showHideFn);

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
        Loader::ArgumentSelector* mLoaderSelector;
        std::function<void(bool)> mLoaderSelectorShowHideFn{nullptr};
        std::unique_ptr<juce::FileChooser> mFileChooser;
        SafeAccessorRetriever mSafeAccessorRetriever;
        juce::File mBackupDirectory;
        bool mSilentResultsFileManagement{false};
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
