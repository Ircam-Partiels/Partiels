#include "AnlApplicationInstance.h"
#include <TranslationsData.h>

ANALYSE_FILE_BEGIN

juce::String const Application::Instance::getApplicationName()
{
    return ProjectInfo::projectName;
}

juce::String const Application::Instance::getApplicationVersion()
{
    return ProjectInfo::versionString;
}

bool Application::Instance::moreThanOneInstanceAllowed()
{
    return true;
}

void Application::Instance::initialise(juce::String const& commandLine)
{
    anlDebug("Application", "Begin...");
    juce::ignoreUnused(commandLine);
    juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDocumentsDirectory).getChildFile("Ircam").setAsCurrentWorkingDirectory();
    juce::LookAndFeel::setDefaultLookAndFeel(&mLookAndFeel);
    
    mAudioFormatManager.registerBasicFormats();
    mWindow = std::make_unique<Window>();
    if(mWindow == nullptr)
    {
        anlDebug("Application", "Failed.");
        return;
    }
    openFile(mApplicationAccessor.getAttr<AttrType::currentDocumentFile>());
    anlDebug("Application", "Ready.");
}

void Application::Instance::anotherInstanceStarted(juce::String const& commandLine)
{
    juce::ignoreUnused(commandLine);
}

void Application::Instance::systemRequestedQuit()
{
    anlDebug("Application", "Begin...");
    if(mDocumentFileBased.saveIfNeededAndUserAgrees() != juce::FileBasedDocument::SaveResult::savedOk)
    {
        return;
    }
    
    if(juce::ModalComponentManager::getInstance()->cancelAllModalComponents())
    {
        anlDebug("Application", "Delayed...");
        juce::Timer::callAfterDelay(500, [this]()
        {
            systemRequestedQuit();
        });
    }
    else
    {
        anlDebug("Application", "Ready");
        quit();
    }
}

void Application::Instance::shutdown()
{
    mWindow.reset();
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    anlDebug("Application", "Done");
}

Application::Instance& Application::Instance::get()
{
    return *static_cast<Instance*>(JUCEApplication::getInstance());
}

juce::String Application::Instance::getFileExtension()
{
    return ".brioche";
}

juce::String Application::Instance::getFileWildCard()
{
    return "*" + getFileExtension();
}

void Application::Instance::openFile(juce::File const& file)
{
    auto const fileExtension = file.getFileExtension();
    if(file == juce::File{})
    {
        mDocumentAccessor.copyFrom({Document::FileBased::getDefaultContainer()}, NotificationType::synchronous);
        mDocumentFileBased.setFile(file);
        mApplicationAccessor.setAttr<AttrType::currentDocumentFile>(file, NotificationType::synchronous);
    }
    else if(getFileExtension() == fileExtension)
    {
        mDocumentFileBased.loadFrom(file, true);
        mApplicationAccessor.setAttr<AttrType::currentDocumentFile>(file, NotificationType::synchronous);
    }
    else if(mAudioFormatManager.getWildcardForAllFormats().contains(fileExtension))
    {
        auto accessor = Document::Accessor(Document::FileBased::getDefaultContainer());
        accessor.setAttr<Document::AttrType::file>(file, NotificationType::synchronous);
        mDocumentAccessor.copyFrom(accessor, NotificationType::synchronous);
        mDocumentFileBased.setFile({});
        mApplicationAccessor.setAttr<AttrType::currentDocumentFile>(juce::File{}, NotificationType::synchronous);
    }
    else
    {
        MessageWindow::show(MessageWindow::MessageType::warning, "File format not supported!", "The format of the file FILENAME is not supported by thee application.", {{"FILENAME", file.getFullPathName()}});
    }
}

Application::Accessor& Application::Instance::getApplicationAccessor()
{
    return mApplicationAccessor;
}

PluginList::Accessor& Application::Instance::getPluginListAccessor()
{
    return mPluginListAccessor;
}

Document::Accessor& Application::Instance::getDocumentAccessor()
{
    return mDocumentAccessor;
}

Document::Director& Application::Instance::getDocumentDirector()
{
    return mDocumentDirector;
}

Document::FileBased& Application::Instance::getDocumentFileBased()
{
    return mDocumentFileBased;
}

juce::ApplicationCommandManager& Application::Instance::getApplicationCommandManager()
{
    return mApplicationCommandManager;
}

juce::AudioFormatManager& Application::Instance::getAudioFormatManager()
{
    return mAudioFormatManager;
}

juce::AudioDeviceManager& Application::Instance::getAudioDeviceManager()
{
    return mAudioDeviceManager;
}

ANALYSE_FILE_END
