#pragma once

#include "AnlApplicationAbout.h"
#include "AnlApplicationAudioReader.h"
#include "AnlApplicationAudioSettings.h"
#include "AnlApplicationBatcher.h"
#include "AnlApplicationExporter.h"
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
    {
    public:
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

        static Instance& get();
        static juce::String getFileExtension();
        static juce::String getFileWildCard();

        void openFiles(std::vector<juce::File> const& files);

        Accessor& getApplicationAccessor();
        AudioSettings* getAudioSettings();
        About* getAbout();
        Window* getWindow();
        Exporter* getExporter();
        Batcher* getBatcher();

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

        juce::File getBackupFile() const;

        juce::ApplicationCommandManager mApplicationCommandManager;
        juce::AudioFormatManager mAudioFormatManager;
        juce::AudioDeviceManager mAudioDeviceManager;
        juce::UndoManager mUndoManager;

        Accessor mApplicationAccessor;
        Accessor::Listener mApplicationListener{typeid(*this).name()};

        std::unique_ptr<PluginList::Accessor> mPluginListAccessor;
        std::unique_ptr<PluginList::Scanner> mPluginListScanner;
        std::unique_ptr<Document::Accessor> mDocumentAccessor;
        std::unique_ptr<Document::Director> mDocumentDirector;

        std::unique_ptr<Properties> mProperties;
        std::unique_ptr<AudioReader> mAudioReader;
        std::unique_ptr<LookAndFeel> mLookAndFeel;
        std::unique_ptr<Document::FileBased> mDocumentFileBased;

        std::unique_ptr<Window> mWindow;
        std::unique_ptr<MainMenuModel> mMainMenuModel;
        std::unique_ptr<About> mAbout;
        std::unique_ptr<AudioSettings> mAudioSettings;
        std::unique_ptr<Exporter> mExporter;
        std::unique_ptr<Batcher> mBatcher;
    };
} // namespace Application

ANALYSE_FILE_END
