#pragma once

#include "../Misc/AnlMisc.h"
#include "AnlApplicationModel.h"
#include "AnlApplicationController.h"
#include "AnlApplicationInterface.h"
#include "AnlApplicationWindow.h"
#include "AnlApplicationLookAndFeel.h"

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
            mInterface = std::make_unique<Interface>();
            ilfWeakAssert(mInterface != nullptr);
            if(mInterface != nullptr)
            {
                mWindow = std::make_unique<Window>(*mInterface.get());
            }
        }
        
        void shutdown() override
        {
            mWindow.reset();
            mInterface.reset();
            juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
            ilf::FontManager::freeDefaultTypeFaces();
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
    };
}

ANALYSE_FILE_END
