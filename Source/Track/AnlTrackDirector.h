#pragma once

#include "../Plugin/AnlPluginListTable.h"
#include "AnlTrackGraphics.h"
#include "AnlTrackLoader.h"
#include "AnlTrackModel.h"
#include "AnlTrackProcessor.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Director
    : private FileWatcher
    , private juce::Timer
    {
    public:
        Director(Accessor& accessor, juce::UndoManager& undoManager, std::unique_ptr<juce::AudioFormatReader> audioFormatReader);
        ~Director() override;

        Accessor& getAccessor();

        juce::Result consolidate(juce::File const& file);

        bool hasChanged() const;
        void startAction();
        void endAction(ActionState state, juce::String const& name = {});
        void setAudioFormatReader(std::unique_ptr<juce::AudioFormatReader> audioFormatReader, NotificationType const notification);

        std::function<void(NotificationType notification)> onIdentifierUpdated = nullptr;
        std::function<void(NotificationType notification)> onResultsUpdated = nullptr;
        std::function<void(NotificationType notification)> onChannelsLayoutUpdated = nullptr;

        void setAlertCatcher(AlertWindow::Catcher* catcher);
        void setPluginTable(PluginList::Table* table);

        void warmAboutPlugin(juce::String const& reason);
        void askToReloadPlugin(juce::String const& reason);
        void askToRestoreState(juce::String const& reason);
        void askToReloadFile(juce::String const& reason);
        void askToRemoveFile();
        void askToResolveWarnings();

    private:
        void sanitizeZooms(NotificationType const notification);
        void runAnalysis(NotificationType const notification);
        void runLoading();
        void runRendering();
        void askForFile();
        void removeFile();

        // FileWatcher
        void fileHasBeenRemoved(juce::File const& file) override;
        void fileHasBeenRestored(juce::File const& file) override;
        void fileHasBeenModified(juce::File const& file) override;

        // juce::Timer
        void timerCallback() override;

        // clang-format off
        enum class ValueRangeMode
        {
              undefined
            , plugin
            , results
            , custom
        };
        // clang-format on

        Accessor& mAccessor;
        juce::UndoManager& mUndoManager;
        Accessor mSavedState;
        bool mIsPerformingAction{false};
        std::unique_ptr<juce::AudioFormatReader> mAudioFormatReader;
        Processor mProcessor;
        Loader mLoader;
        Graphics mGraphics;
        std::optional<std::reference_wrapper<Zoom::Accessor>> mSharedZoomAccessor;
        Zoom::Accessor::Listener mSharedZoomListener{typeid(*this).name()};
        std::mutex mSharedZoomMutex;
        ValueRangeMode mValueRangeMode = ValueRangeMode::undefined;
        AlertWindow::Catcher* mAlertCatcher = nullptr;
        PluginList::Table* mPluginTable = nullptr;
        std::unique_ptr<juce::FileChooser> mFileChooser;
        std::unique_ptr<Loader::ArgumentSelector> mLoaderArgumentSelector;

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
