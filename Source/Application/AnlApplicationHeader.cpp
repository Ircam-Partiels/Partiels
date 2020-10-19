
#include "AnlApplicationHeader.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Header::Header()
{
//    addAndMakeVisible(mAudioTransportControls);
//    addAndMakeVisible(mHMSmsTimeField);
//
//    mMainTime.setFont(ilf::FontManager::getEditLabelFont().withPointHeight(20.f));
//    mHMSmsTimeField.setJustificationType(juce::Justification::centredRight);
//    mHMSmsTimeField.setEditable(true, true, true);
//    mHMSmsTimeField.onTimeChanged = []()
//    {
//
//    };
//    mHMSmsTimeField.setColour(juce::Label::textColourId, ilf::GraphicsHelpers::blueColour);
//    mHMSmsTimeField.setColour(juce::Label::textWhenEditingColourId, ilf::GraphicsHelpers::yellowColour);
//    mHMSmsTimeField.setColour(juce::TextEditor::textColourId, ilf::GraphicsHelpers::yellowColour);
//    mHMSmsTimeField.setColour(ilf::NumberField::ColourIds::textOverColourId, ilf::GraphicsHelpers::yellowColour);
//    mHMSmsTimeField.setTooltip("The start playhead time position");
}

void Application::Header::resized()
{
//    auto bounds = getLocalBounds().reduced(4);
//    mAudioTransportControls.setBounds(bounds.removeFromLeft(210));
}

void Application::Header::applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info)
{
    
//    auto updateButton = [](juce::Button& button)
//    {
//        button.setState(juce::Button::buttonDown);
//        juce::Timer::callAfterDelay(100, [sp = juce::Component::SafePointer<juce::Button>(&button)]()
//        {
//            if(auto safeButton = sp.getComponent())
//            {
//                safeButton->setState(juce::Button::buttonNormal);
//            }
//        });
//    };
//    using CommandIds = ilf::AudioSequencer::CommandTarget::Transport::CommandIds;
//    if(info.commandID == CommandIds::TogglePlay)
//    {
//        updateButton(mAudioTransportControls.getPlayPauseButton());
//    }
//    else if(info.commandID == CommandIds::ToggleLoop)
//    {
//        updateButton(mAudioTransportControls.getLoopButton());
//    }
//    else if(info.commandID == CommandIds::ToggleRecord)
//    {
//        updateButton(mAudioTransportControls.getRecordButton());
//    }
//    else if(info.commandID == CommandIds::MovePlayheadToBegin)
//    {
//        updateButton(mAudioTransportControls.getRewindButton());
//    }
//    else if(info.commandID == CommandIds::MovePlayheadToEnd)
//    {
//        updateButton(mAudioTransportControls.getFastForwardButton());
//    }
}

void Application::Header::applicationCommandListChanged()
{
//    auto& commandManager = Instance::get().getCommandManager();
//    using CommandIds = ilf::AudioSequencer::CommandTarget::Transport::CommandIds;
//    mAudioTransportControls.getPlayPauseButton().setTooltip(commandManager.getDescriptionOfCommand(CommandIds::TogglePlay));
//    mAudioTransportControls.getRecordButton().setTooltip(commandManager.getDescriptionOfCommand(CommandIds::ToggleRecord));
//    mAudioTransportControls.getLoopButton().setTooltip(commandManager.getDescriptionOfCommand(CommandIds::ToggleLoop));
//    mAudioTransportControls.getRewindButton().setTooltip(commandManager.getDescriptionOfCommand(CommandIds::MovePlayheadToBegin));
//    mAudioTransportControls.getFastForwardButton().setTooltip(commandManager.getDescriptionOfCommand(CommandIds::MovePlayheadToEnd));
}

ANALYSE_FILE_END
