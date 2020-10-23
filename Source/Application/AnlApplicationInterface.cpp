#include "AnlApplicationInterface.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationCommandIDs.h"

ANALYSE_FILE_BEGIN

Application::Interface::Interface()
: mDocumentTransport(Instance::get().getDocumentAccessor())
, mDocumentFileInfoPanel(Instance::get().getAudioFormatManager(), Instance::get().getDocumentAccessor())
, mPluginListTable(Instance::get().getPluginListAccessor())
{
    addAndMakeVisible(mDocumentTransport);
    addAndMakeVisible(mDocumentTransportSeparator);
    addAndMakeVisible(mDocumentFileInfoPanel);
    addAndMakeVisible(mHeaderSeparator);
    Instance::get().getApplicationCommandManager().registerAllCommandsForTarget(this);
    
    mPluginListTable.onPluginSelected = [&](juce::String key)
    {
        auto copy = mAnalyzerAccessor.getModel();
        copy.key = key;
        mAnalyzerAccessor.fromModel(copy, juce::NotificationType::sendNotificationSync);
        if(mAudioFormatReader != nullptr)
        {
            mAnalyzerProcessor.perform(*(mAudioFormatReader.get()));
        }
    };
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
            auto* audioFormat = audioFormatManager.findFormatForFileExtension(file.getFileExtension());
            if(audioFormat == nullptr)
            {
                juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, juce::translate("Can't load the audio!"), juce::translate("the file extension format ADFMMT is not supported.").replace("ADFMMT", file.getFileExtension()));
                return true;
            }
            
            mAudioFormatReader = std::unique_ptr<juce::AudioFormatReader>(audioFormat->createReaderFor(file.createInputStream().release(), true));
            if(mAudioFormatReader == nullptr)
            {
                juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, juce::translate("Can't load the audio!"), juce::translate("the file audio forrmat reader cannot be allocated.").replace("ADFMMT", file.getFileExtension()));
                return true;
            }
            return true;
        }
        case CommandIDs::New:
        {
            juce::DialogWindow::showModalDialog("Plugin List", &mPluginListTable, this, findColour(juce::ResizableWindow::backgroundColourId, true), true);
            return true;
        }
        default:
            break;
    }
    return false;
}

ANALYSE_FILE_END
