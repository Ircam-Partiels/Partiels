#pragma once

#include "AnlApplicationAudioReader.h"
#include "AnlApplicationCommandLine.h"
#include "AnlApplicationInterface.h"
#include "AnlApplicationLookAndFeel.h"
#include "AnlApplicationMainMenuModel.h"
#include "AnlApplicationModel.h"
#include "AnlApplicationProperties.h"
#include "AnlApplicationTranslationManager.h"
#include "AnlApplicationWindow.h"

#include "../Document/AnlDocumentDirector.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Instance
    : public juce::JUCEApplication
    , private juce::ChangeListener
    , private juce::AsyncUpdater
    , private juce::DarkModeSettingListener
    {
    public:
        using FileLoaderSelectorContainer = Document::Director::LoaderSelectorContainer;

        Instance() = default;
        ~Instance() override = default;

        // juce::JUCEApplication
        juce::String const getApplicationName() override;
        juce::String const getApplicationVersion() override;
        bool moreThanOneInstanceAllowed() override;
        void initialise(juce::String const& commandLine) override;
        void anotherInstanceStarted(juce::String const& commandLine) override;
        void systemRequestedQuit() override;
        void shutdown() override;

        // ApplicationCommandTarget
        juce::ApplicationCommandTarget* getNextCommandTarget() override;

        static Instance& get();
        static juce::String getExtensionForDocumentFile();
        static juce::String getWildCardForDocumentFile();
        static juce::String getWildCardForImportFormats();
        static juce::String getWildCardForAudioFormats();
        static std::pair<int, int> getSizeFor(juce::String const& identifier);
        static LookAndFeel::ColourChart getColourChart();

        void newDocument();
        void openDocumentFile(juce::File const& file);
        void openAudioFiles(std::vector<juce::File> const& files);
        void openFiles(std::vector<juce::File> const& files);
        void importFile(std::tuple<juce::String, size_t> const position, juce::File const& file);

        AuthorizationProcessor& getAuthorizationProcessor();
        FloatingWindowContainer& getAuthorizationWindow();

        Accessor& getApplicationAccessor();
        Window* getWindow();
        FileLoaderSelectorContainer* getFileLoaderSelectorContainer();

        PluginList::Accessor& getPluginListAccessor();
        PluginList::Scanner& getPluginListScanner();
        Document::Accessor& getDocumentAccessor();
        Document::Director& getDocumentDirector();
        Document::FileBased& getDocumentFileBased();

        juce::ApplicationCommandManager& getApplicationCommandManager();
        juce::AudioFormatManager& getAudioFormatManager();
        juce::AudioDeviceManager& getAudioDeviceManager();
        juce::UndoManager& getUndoManager();

    private:
        // juce::ChangeListener
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;

        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        // juce::DarkModeSettingListener
        void darkModeSettingChanged() override;

        void updateLookAndFeel();
        juce::File getBackupFile() const;
        void openStartupFiles();
        void checkPluginsQuarantine();

        AuthorizationProcessor mAuthorizationProcessor;

        std::unique_ptr<juce::ApplicationCommandManager> mApplicationCommandManager;
        std::unique_ptr<juce::AudioFormatManager> mAudioFormatManager;
        std::unique_ptr<juce::AudioDeviceManager> mAudioDeviceManager;
        std::unique_ptr<juce::UndoManager> mUndoManager;

        std::unique_ptr<Accessor> mApplicationAccessor;
        std::unique_ptr<Accessor::Listener> mApplicationListener;

        std::unique_ptr<PluginList::Accessor> mPluginListAccessor;
        std::unique_ptr<PluginList::Scanner> mPluginListScanner;
        std::unique_ptr<Document::Accessor> mDocumentAccessor;
        std::unique_ptr<Document::Director> mDocumentDirector;

        std::unique_ptr<AudioReader> mAudioReader;
        std::unique_ptr<LookAndFeel> mLookAndFeel;
        std::unique_ptr<Properties> mProperties;
        std::unique_ptr<Document::FileBased> mDocumentFileBased;

        std::unique_ptr<AuthorizationPanel> mAuthorizationPanel;
        std::unique_ptr<FloatingWindowContainer> mAuthorizationWindow;

        struct FileLoader
        {
            Track::Loader::ArgumentSelector selector;
            Track::Loader::ArgumentSelector::WindowContainer window{selector};
            FileLoaderSelectorContainer container{selector, window};
        };
        std::unique_ptr<FileLoader> mFileLoader;

        std::unique_ptr<Window> mWindow;
        std::unique_ptr<MainMenuModel> mMainMenuModel;
        std::unique_ptr<CommandLine> mCommandLine;

        std::atomic<bool> mIsPluginListReady{true};
        juce::File mPreviousFile{};
        std::vector<juce::File> mCommandFiles;
        bool mStartupFilesAreLoaded{false};

        JUCE_DECLARE_WEAK_REFERENCEABLE(Instance)
    };
} // namespace Application

ANALYSE_FILE_END
