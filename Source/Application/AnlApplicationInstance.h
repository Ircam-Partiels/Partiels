#pragma once

#include "AnlApplicationModel.h"
#include "AnlApplicationInterface.h"
#include "AnlApplicationWindow.h"
#include "AnlApplicationLookAndFeel.h"
#include "AnlApplicationProperties.h"
#include "AnlApplicationAudioReader.h"
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
        PluginList::Accessor& getPluginListAccessor();
        Document::Accessor& getDocumentAccessor();
        Document::FileBased& getDocumentFileBased();
        
        juce::ApplicationCommandManager& getApplicationCommandManager();
        juce::AudioFormatManager& getAudioFormatManager();
        juce::AudioDeviceManager& getAudioDeviceManager();

    private:
        
        juce::ApplicationCommandManager mApplicationCommandManager;
        juce::AudioFormatManager mAudioFormatManager;
        juce::AudioDeviceManager mAudioDeviceManager;
        
        Container mApplicationContainer;
        Accessor mApplicationAccessor {mApplicationContainer};
        PluginList::Container mPluginListContainer;
        PluginList::Accessor mPluginListAccessor {mPluginListContainer};
        Document::Container mDocumentContainer
        { {juce::File{}}
            , {false}
            , {1.0}
            , {false}
            , {0.0}
            , {Zoom::Container{{juce::Range<double>{0.0, 47.0}}, {0.001}, {juce::Range<double>{0.0, 47.0}}}}
            , {}
        };
        Document::Accessor mDocumentAccessor {mDocumentContainer};
        Document::FileWatcher mDocumentFileWatcher {mDocumentAccessor, mAudioFormatManager};
        
        Properties mProperties;
        AudioReader mAudioReader;
        LookAndFeel mLookAndFeel;
        Document::FileBased mDocumentFileBased {mDocumentAccessor, getFileExtension(), getFileWildCard(), "Open a document", "Save the document"};
        
        std::unique_ptr<Window> mWindow;
    };
}

ANALYSE_FILE_END
