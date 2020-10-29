#pragma once

#include "AnlApplicationModel.h"
#include "AnlApplicationInterface.h"
#include "AnlApplicationWindow.h"
#include "AnlApplicationLookAndFeel.h"
#include "AnlApplicationProperties.h"
#include "AnlApplicationAudioReader.h"

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
        
        Accessor& getAccessor();
        PluginList::Accessor& getPluginListAccessor();
        Document::Accessor& getDocumentAccessor();
        
        juce::ApplicationCommandManager& getApplicationCommandManager();
        juce::AudioFormatManager& getAudioFormatManager();
        juce::AudioDeviceManager& getAudioDeviceManager();

    private:
        
        juce::ApplicationCommandManager mApplicationCommandManager;
        juce::AudioFormatManager mAudioFormatManager;
        juce::AudioDeviceManager mAudioDeviceManager;
        
        Model mModel;
        Document::Model mDocumentModel;
        PluginList::Model mPluginListModel;
        
        Accessor mAccessor {mModel};
        PluginList::Accessor mPluginListAccessor {mPluginListModel};
        Document::Accessor mDocumentAccessor {mDocumentModel};
        
        Properties mProperties;
        AudioReader mAudioReader;
        LookAndFeel mLookAndFeel;
        
        std::unique_ptr<Window> mWindow;
    };
}

ANALYSE_FILE_END
