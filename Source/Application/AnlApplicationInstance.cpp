#include "AnlApplicationInstance.h"

#include "../Tools/AnlModel.h"

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
    enum attrs : size_t { v1, v2, v3, v4 };
    using AttrFlag = Anl::Model::AttrFlag;
    using ModelContainer = Anl::Model::Container
    <Anl::Model::AttrType<attrs::v1, int, AttrFlag::all>
    ,Anl::Model::AttrType<attrs::v2, int, AttrFlag::ignored>
    ,Anl::Model::AttrType<attrs::v3, float, AttrFlag::all>
    ,Anl::Model::AttrType<attrs::v4, std::vector<int>, AttrFlag::ignored>
    >;
    
    using ModelAcsr = Anl::Model::Accessor<ModelContainer>;
    ModelAcsr acsr1(ModelContainer{{1}, {2}, {3.0f}, {{}}});
    ModelAcsr acsr2;
    
    auto xml = acsr1.toXml("state1");
    std::cout << "acsr1: "<< xml->toString() << "\n";
    acsr1.setValue<attrs::v1>(acsr1.getValue<attrs::v2>() + 2);
    acsr1.setValue<attrs::v3>(acsr1.getValue<attrs::v3>() - 2.2f);
    xml = acsr1.toXml("state2");
    std::cout << "acsr1: "<< xml->toString() << "\n";
    acsr2.fromXml(*xml.get(), "state2");
    xml = acsr2.toXml("state3");
    std::cout << "acsr2: "<< xml->toString() << "\n";
    acsr2.setValue<attrs::v1>(acsr2.getValue<attrs::v1>() + 2);
    acsr2.setValue<attrs::v3>(acsr2.getValue<attrs::v3>() - 2.2f);
//    acsr1.fromModel(acsr2.getModel());
//    xml = acsr1.toXml("state4");
//    std::cout << "acsr1: "<< xml->toString() << "\n";
    
    AnlDebug("Application", "Begin...");
    juce::ignoreUnused(commandLine);
    juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDocumentsDirectory).getChildFile("Ircam").setAsCurrentWorkingDirectory();
    juce::LookAndFeel::setDefaultLookAndFeel(&mLookAndFeel);
    
    juce::LocalisedStrings::setCurrentMappings(new juce::LocalisedStrings(juce::String::createStringFromData(BinaryData::Fr_txt, BinaryData::Fr_txtSize), false));
    
    mAudioFormatManager.registerBasicFormats();
    mWindow = std::make_unique<Window>();
    if(mWindow == nullptr)
    {
        AnlDebug("Application", "Failed.");
        return;
    }
    openFile(mModel.currentDocumentFile);
    AnlDebug("Application", "Ready.");
}

void Application::Instance::anotherInstanceStarted(juce::String const& commandLine)
{
    juce::ignoreUnused(commandLine);
}

void Application::Instance::systemRequestedQuit()
{
    AnlDebug("Application", "Begin...");
    if(mDocumentFileBased.saveIfNeededAndUserAgrees() != juce::FileBasedDocument::SaveResult::savedOk)
    {
        return;
    }
    
    if(juce::ModalComponentManager::getInstance()->cancelAllModalComponents())
    {
        AnlDebug("Application", "Delayed...");
        juce::Timer::callAfterDelay(500, [this]()
        {
            systemRequestedQuit();
        });
    }
    else
    {
        AnlDebug("Application", "Ready");
        quit();
    }
}

void Application::Instance::shutdown()
{
    mWindow.reset();
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    AnlDebug("Application", "Done");
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
        auto copy = mAccessor.getModel();
        copy.currentDocumentFile = file;
        mAccessor.fromModel(copy, juce::NotificationType::sendNotificationSync);
    }
    else if(mAudioFormatManager.getWildcardForAllFormats().contains(fileExtension))
    {
        Document::Model model;
        model.file = file;
        mDocumentFileBased.setFile({});
        mDocumentAccessor.fromModel(model, juce::NotificationType::sendNotificationSync);
        auto copy = mAccessor.getModel();
        copy.currentDocumentFile = juce::File{};
        mAccessor.fromModel(copy, juce::NotificationType::sendNotificationSync);
    }
    else
    {
        anlWeakAssert(false && "file format is not supported");
    }
}

Application::Accessor& Application::Instance::getAccessor()
{
    return mAccessor;
}

PluginList::Accessor& Application::Instance::getPluginListAccessor()
{
    return mPluginListAccessor;
}

Document::Accessor& Application::Instance::getDocumentAccessor()
{
    return mDocumentAccessor;
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
