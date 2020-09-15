#pragma once

#include "../Misc/AnlMisc.h"
#include "AnlApplicationModel.h"
#include "AnlApplicationController.h"

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
        }
        
        void shutdown() override
        {
        }
        
        void systemRequestedQuit() override
        {
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
        
        Model mModel;
        Controller mController {mModel};
        juce::ApplicationCommandManager mCommandManager;
    };
}

ANALYSE_FILE_END
