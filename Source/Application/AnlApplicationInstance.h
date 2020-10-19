#pragma once

#include "AnlApplicationModel.h"
#include "AnlApplicationInterface.h"
#include "AnlApplicationWindow.h"
#include "AnlApplicationLookAndFeel.h"
#include "AnlApplicationProperties.h"

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
        
        Accessor& getAccessor();
        PluginList::Accessor& getPluginListAccessor();
        juce::ApplicationCommandManager& getApplicationCommandManager();
        juce::AudioFormatManager& getAudioFormatManager();
        
    private:
        
        void saveProperties();
        
        juce::ApplicationCommandManager mApplicationCommandManager;
        juce::AudioFormatManager mAudioFormatManager;
        
        Model mModel;
        Accessor mAccessor {mModel};
        
        PluginList::Model mPluginListModel;
        PluginList::Accessor mPluginListAccessor {mPluginListModel};
        
        Properties mProperties;
        LookAndFeel mLookAndFeel;
        
        std::unique_ptr<Interface> mInterface;
        std::unique_ptr<Window> mWindow;
    };
}

ANALYSE_FILE_END
