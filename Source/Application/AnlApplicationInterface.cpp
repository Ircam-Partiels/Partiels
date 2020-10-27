#include "AnlApplicationInterface.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationCommandIDs.h"

ANALYSE_FILE_BEGIN

Application::Interface::Interface()
: mDocumentTransport(Instance::get().getDocumentAccessor())
, mDocumentFileInfoPanel(Instance::get().getDocumentAccessor(), Instance::get().getAudioFormatManager())
, mDocumentAnalyzerPanel(Instance::get().getDocumentAccessor(), Instance::get().getPluginListAccessor())
{
    addAndMakeVisible(mDocumentTransport);
    addAndMakeVisible(mDocumentTransportSeparator);
    addAndMakeVisible(mDocumentFileInfoPanel);
    addAndMakeVisible(mHeaderSeparator);
    addAndMakeVisible(mDocumentAnalyzerPanel);
    
    mDocumentListener.onChanged = [&](Document::Accessor& acsr, Document::Model::Attribute attribute)
    {
        if(attribute == Document::Model::Attribute::file)
        {
            auto const file = acsr.getModel().file;
            auto const& audioFormatManager = Instance::get().getAudioFormatManager();
            auto const isDocumentEnable = file.existsAsFile() && audioFormatManager.getWildcardForAllFormats().contains(file.getFileExtension());
                
            mDocumentTransport.setEnabled(isDocumentEnable);
            mDocumentFileInfoPanel.setEnabled(isDocumentEnable);
            mDocumentAnalyzerPanel.setEnabled(isDocumentEnable);
        }
    };
    
    Instance::get().getDocumentAccessor().addListener(mDocumentListener, juce::NotificationType::sendNotificationSync);
    Instance::get().getApplicationCommandManager().registerAllCommandsForTarget(this);
}

Application::Interface::~Interface()
{
    Instance::get().getDocumentAccessor().removeListener(mDocumentListener);
}

void Application::Interface::resized()
{
    auto constexpr separatorSize = 2;
    auto bounds = getLocalBounds();
    auto header = bounds.removeFromTop(102);
    mDocumentTransport.setBounds(header.removeFromLeft(240));
    mDocumentTransportSeparator.setBounds(header.removeFromLeft(separatorSize));
    mDocumentFileInfoPanel.setBounds(header);
    mHeaderSeparator.setBounds(bounds.removeFromTop(separatorSize));
    mDocumentAnalyzerPanel.setBounds(bounds);
}

juce::ApplicationCommandTarget* Application::Interface::getNextCommandTarget()
{
    return nullptr;
}

void Application::Interface::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    commands.add(CommandIDs::Open);
    commands.add(CommandIDs::New);
}

void Application::Interface::getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result)
{
    switch (commandID)
    {
        case CommandIDs::Open:
        {
            result.setInfo(juce::translate("Open"), juce::translate("Open an audio file"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('o', juce::ModifierKeys::commandModifier, 0));
        }
            break;
        case CommandIDs::New:
        {
            result.setInfo(juce::translate("New"), juce::translate("New analyis file"), "Application", 0);
            result.defaultKeypresses.add(juce::KeyPress('n', juce::ModifierKeys::commandModifier, 0));
        }
            break;
            
        default:
            break;
    }
}

bool Application::Interface::perform(juce::ApplicationCommandTarget::InvocationInfo const& info)
{
    switch (info.commandID)
    {
        case CommandIDs::Open:
        {
            using AlertIconType = juce::AlertWindow::AlertIconType;
            auto& acsr = Instance::get().getDocumentAccessor();
            auto& audioFormatManager = Instance::get().getAudioFormatManager();
            JUCE_COMPILER_WARNING("Translation required for : Open Document");
            juce::FileChooser fc(juce::translate("Open Document"), {}, audioFormatManager.getWildcardForAllFormats());
            if(!fc.browseForFileToOpen())
            {
                auto copy = acsr.getModel();
                copy.file = juce::File{};
                acsr.fromModel(copy, juce::NotificationType::sendNotificationSync);
                return true;
            }
                
            auto const file = fc.getResult();
            
            auto copy = acsr.getModel();
            copy.file = file;
            acsr.fromModel(copy, juce::NotificationType::sendNotificationSync);
            return true;
        }
        case CommandIDs::New:
        {
            return true;
        }
        default:
            break;
    }
    return false;
}

ANALYSE_FILE_END
