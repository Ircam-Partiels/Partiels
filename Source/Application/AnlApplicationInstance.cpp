#include "AnlApplicationInstance.h"

#include "../Misc/AnlModel.h"

ANALYSE_FILE_BEGIN

Application::Instance::LocalisedStringsMapper::LocalisedStringsMapper()
{
    auto ls = std::make_unique<juce::LocalisedStrings>(juce::String::createStringFromData(BinaryData::Fr_txt, BinaryData::Fr_txtSize), false);
    if(ls != nullptr)
    {
        ls->addStrings({juce::String::createStringFromData(BinaryData::Fr_txt, BinaryData::Fr_txtSize), false});
        ls->addStrings({juce::String::createStringFromData(BinaryData::Plugin_txt, BinaryData::Plugin_txtSize), false});
        ls->addStrings({juce::String::createStringFromData(BinaryData::Analyzer_txt, BinaryData::Analyzer_txtSize), false});
    }
    juce::LocalisedStrings::setCurrentMappings(ls.release());
}

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
    if(getFileExtension() == fileExtension)
    {
        mDocumentFileBased.loadFrom(file, true);
        mApplicationAccessor.setAttr<AttrType::currentDocumentFile>(file, NotificationType::synchronous);
    }
    else if(mAudioFormatManager.getWildcardForAllFormats().contains(fileExtension))
    {
        mDocumentFileBased.setFile({});
        mDocumentAccessor.setAttr<Document::AttrType::file>(file, NotificationType::synchronous);
        mApplicationAccessor.setAttr<AttrType::currentDocumentFile>(juce::File{}, NotificationType::synchronous);
    }
    else
    {
        anlWeakAssert(false && "file format is not supported");
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
