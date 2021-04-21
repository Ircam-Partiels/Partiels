#pragma once

#include "AnlApplicationModel.h"
#include "AnlApplicationInterface.h"
#include "AnlApplicationWindow.h"
#include "AnlApplicationLookAndFeel.h"
#include "AnlApplicationTranslationManager.h"
#include "AnlApplicationProperties.h"
#include "AnlApplicationAbout.h"
#include "AnlApplicationAudioSettings.h"
#include "AnlApplicationAudioReader.h"
#include "AnlApplicationMainMenuModel.h"

#include "../Document/AnlDocumentDirector.h"
#include "../Document/AnlDocumentFileWatcher.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Instance
    : public juce::JUCEApplication
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
        
        void openFile(juce::File const& file);
        
        Accessor& getApplicationAccessor();
        AudioSettings& getAudioSettings();
        About& getAbout();
        PluginList::Accessor& getPluginListAccessor();
        Document::Accessor& getDocumentAccessor();
        Document::Director& getDocumentDirector();
        Document::FileBased& getDocumentFileBased();
        
        juce::ApplicationCommandManager& getApplicationCommandManager();
        juce::AudioFormatManager& getAudioFormatManager();
        juce::AudioDeviceManager& getAudioDeviceManager();
        juce::UndoManager& getUndoManager();

    private:
        TranslationManager mTranslationManager;
        juce::ApplicationCommandManager mApplicationCommandManager;
        juce::AudioFormatManager mAudioFormatManager;
        juce::AudioDeviceManager mAudioDeviceManager;
        juce::UndoManager mUndoManager;
        Accessor mApplicationAccessor;
        PluginList::Accessor mPluginListAccessor;
        PluginList::Scanner mPluginListScanner;
        Document::Accessor mDocumentAccessor;
        Document::Director mDocumentDirector {mDocumentAccessor, mPluginListAccessor, mPluginListScanner, mAudioFormatManager};
        Document::FileWatcher mDocumentFileWatcher {mDocumentAccessor, mAudioFormatManager};
        
        Properties mProperties;
        AudioReader mAudioReader;
        LookAndFeel mLookAndFeel;
        Document::FileBased mDocumentFileBased {mDocumentAccessor, mDocumentDirector, getFileExtension(), getFileWildCard(), "Open a document", "Save the document"};
        
        std::unique_ptr<Window> mWindow;
        std::unique_ptr<MainMenuModel> mMainMenuModel;
        std::unique_ptr<About> mAbout;
        std::unique_ptr<AudioSettings> mAudioSettings;
    };
}

ANALYSE_FILE_END
