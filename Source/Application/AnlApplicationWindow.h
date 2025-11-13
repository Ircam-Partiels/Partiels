#pragma once

#include "AnlApplicationCommandTarget.h"
#include "AnlApplicationInterface.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Window
    : public juce::DocumentWindow
    , private juce::AsyncUpdater
    , private juce::ChangeListener
    {
    public:
        Window();
        ~Window() override;

        Interface& getInterface();
        Interface const& getInterface() const;

        CommandTarget& getCommandTarget();
        CommandTarget const& getCommandTarget() const;

        void refreshInterface();

        // juce::DocumentWindow
        void closeButtonPressed() override;
        void resized() override;
        void moved() override;
        void maximiseButtonPressed() override;
        void lookAndFeelChanged() override;

    private:
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        // juce::ChangeListener
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;

        class CommandTargetInterface
        : public juce::Component
        , public CommandTarget
        {
        public:
            CommandTargetInterface() = default;
            ~CommandTargetInterface() override = default;

            // juce::Component
            void resized() override;
        };

        juce::ComponentBoundsConstrainer mBoundsConstrainer;
        CommandTargetInterface mCommandTargetInterface;
        std::unique_ptr<Interface> mInterface;
    };
} // namespace Application

ANALYSE_FILE_END
