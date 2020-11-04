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
    enum typess { v1, v2, v3, v4 };
    
    using data_t = std::tuple
    <Anl::Model::TypedData<int, true, decltype(magic_enum::enum_name<typess::v1>())>
    ,Anl::Model::TypedData<int, false, decltype("v2"_tstr)>
    ,Anl::Model::TypedData<float, true, decltype("v3"_tstr)>
    ,Anl::Model::TypedData<std::vector<int>, false, decltype("v4"_tstr)>
    >;
    
    Anl::Model::Accessor<data_t> accessor(data_t{{1}, {2}, {3.0f}, {{}}});
    
    auto xml = accessor.toXml("john");
    std::cout << xml->toString() << "\n";
    auto model = Anl::Model::Accessor<data_t>::fromXml(*xml.get(), "john", data_t(accessor));
    std::get<0>(model).value += 2;
    std::get<2>(model).value -= 2.2f;
    Anl::Model::Accessor<data_t> accessor2(std::move(model));
    xml = accessor2.toXml("jim");
    std::cout << xml->toString() << "\n";
    
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
