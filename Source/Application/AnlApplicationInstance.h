#pragma once

#include "AnlApplicationModel.h"
#include "AnlApplicationController.h"
#include "AnlApplicationInterface.h"
#include "AnlApplicationWindow.h"
#include "AnlApplicationLookAndFeel.h"

#include "../Plugin/AnlPluginScanner.h"

#include "../Model/AnlModelAnalyzer.h"
#include "../View/AnlViewAnalyzer.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Instance
    : public JUCEApplication
    , public juce::ChangeListener
    {
    public:
        
        Instance()
        {
        }
        
        juce::String const getApplicationName() override
        {
            return ProjectInfo::projectName;
        }
        
        juce::String const getApplicationVersion() override
        {
            return ProjectInfo::versionString;
        }
        
        bool moreThanOneInstanceAllowed() override
        {
            return true;
        }
        
        void initialise(juce::String const& commandLine) override
        {
            juce::ignoreUnused(commandLine);
            juce::LookAndFeel::setDefaultLookAndFeel(&mLookAndFeel);
            
            juce::LocalisedStrings::setCurrentMappings(new juce::LocalisedStrings(juce::String::createStringFromData(BinaryData::Fr_txt, BinaryData::Fr_txtSize), false));
            
            mInterface = std::make_unique<Interface>();
            anlWeakAssert(mInterface != nullptr);
            if(mInterface != nullptr)
            {
                mWindow = std::make_unique<Window>(*mInterface.get());
            }
            
            mPluginScanner.scan();
            
            juce::AudioFormatManager audioFormatManager;
            audioFormatManager.registerBasicFormats();
            auto audioFormat = audioFormatManager.findFormatForFileExtension(".wav");
            auto audioFormatReader = std::unique_ptr<juce::MemoryMappedAudioFormatReader>(audioFormat->createMemoryMappedReader({"/Users/guillot/Dropbox/Cours/Sounds/Snail48k.wav"}));
            if(audioFormatReader != nullptr)
            {
                audioFormatReader->mapEntireFile();
            }
            auto copy = mAnalyzerAccessor.getModel();
            copy.key = juce::String("jimi");
            mAnalyzerAccessor.fromModel(copy, juce::NotificationType::sendNotification);
            if(audioFormatReader != nullptr)
            {
                mAnalyzerView.perform(*(audioFormatReader.get()));
            }
            
            copy.key = juce::String("vamp-example-plugins:spectralcentroid");
            mAnalyzerAccessor.fromModel(copy, juce::NotificationType::sendNotification);
            if(audioFormatReader != nullptr)
            {
                mAnalyzerView.perform(*(audioFormatReader.get()));
            }
            
            copy.key = juce::String("vamp-example-plugins:percussiononsets");
            mAnalyzerAccessor.fromModel(copy, juce::NotificationType::sendNotification);
            if(audioFormatReader != nullptr)
            {
                mAnalyzerView.perform(*(audioFormatReader.get()));                
            }
            
        }
        
        void shutdown() override
        {
            mWindow.reset();
            mInterface.reset();
            juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
        }
        
        void systemRequestedQuit() override
        {
            quit();
        }
        
        void anotherInstanceStarted (const String& commandLine) override
        {
            juce::ignoreUnused(commandLine);
        }
        
        juce::ApplicationCommandTarget* getNextCommandTarget() override
        {
            return nullptr;
        }
        
        void getAllCommands(juce::Array<juce::CommandID>& commands) override
        {
            juce::ignoreUnused(commands);
        }
        
        void changeListenerCallback(ChangeBroadcaster* source) override
        {
            juce::ignoreUnused(source);
        }
        
        Controller& getController()
        {
            return mController;
        }
        
        juce::ApplicationCommandManager& getCommandManager()
        {
            return mCommandManager;
        }

        static Instance& get()
        {
            return *static_cast<Instance*>(JUCEApplication::getInstance());
        }

    private:
        
        juce::ApplicationCommandManager mCommandManager;
        
        Model mModel;
        Controller mController {mModel};
        
        LookAndFeel mLookAndFeel;
        std::unique_ptr<Interface> mInterface;
        std::unique_ptr<Window> mWindow;
        Plugin::Scanner mPluginScanner;
        
        Analyzer::Model mAnalyzerModel;
        Analyzer::Accessor mAnalyzerAccessor {mAnalyzerModel};
        Analyzer::View mAnalyzerView {mAnalyzerAccessor};
    };
}

ANALYSE_FILE_END
