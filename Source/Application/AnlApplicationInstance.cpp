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
    enum AttrType : size_t { v1, v2, v3, v4, v5 };
    using AttrFlag = Anl::Model::AttrFlag;
    using Container = Anl::Model::Container
    <Anl::Model::Attr<AttrType::v1, int, AttrFlag::all>
    ,Anl::Model::Attr<AttrType::v2, int, AttrFlag::notifying>
    ,Anl::Model::Attr<AttrType::v3, float, AttrFlag::saveable>
    ,Anl::Model::Attr<AttrType::v4, std::vector<int>, AttrFlag::ignored>
    ,Anl::Model::Attr<AttrType::v5, std::string, AttrFlag::notifying>
    >;
    
    using ModelAcsr = Anl::Model::Accessor<Container>;
    using ModelLtnr = ModelAcsr::Listener;
    ModelAcsr acsr1({{1}, {2}, {3.0f}, {{1, 2, 3}}, {"john"}});
    ModelAcsr acsr2;
    
    ModelLtnr ltnr;
    ltnr.onChanged = [&](ModelAcsr const& acsr, AttrType attribute)
    {
        std::cout << (&acsr == &acsr1 ? "acsr1" : "acsr2") << " changed "<< magic_enum::enum_name(attribute) << ": ";
        switch(attribute)
        {
            case AttrType::v1:
                std::cout << acsr.getValue<AttrType::v1>() << "\n";
                break;
            case AttrType::v2:
                std::cout << acsr.getValue<AttrType::v2>() << "\n";
                break;
            case AttrType::v3:
                std::cout << acsr.getValue<AttrType::v3>() << "\n";
                break;
            case AttrType::v4:
            {
                for(auto const& v : acsr.getValue<AttrType::v4>())
                {
                    std::cout << v << " ";
                }
                std::cout << "\n";
            }
                break;
            case AttrType::v5:
                std::cout << acsr.getValue<AttrType::v5>() << "\n";
                break;
        }
    };
    acsr1.addListener(ltnr, NotificationType::synchronous);
    acsr2.addListener(ltnr, NotificationType::synchronous);
    
    auto xml = acsr1.toXml("state1");
    std::cout << "acsr1: "<< xml->toString() << "\n";
    acsr1.setValue<AttrType::v1>(acsr1.getValue<AttrType::v2>() + 2);
    acsr1.setValue<AttrType::v3>(acsr1.getValue<AttrType::v3>() - 2.2f);
    xml = acsr1.toXml("state2");
    std::cout << "acsr1: "<< xml->toString() << "\n";
    acsr2.fromXml(*xml.get(), "state2");
    xml = acsr2.toXml("state3");
    std::cout << "acsr2: "<< xml->toString() << "\n";
    acsr2.setValue<AttrType::v1>(acsr2.getValue<AttrType::v1>() + 2);
    acsr2.setValue<AttrType::v3>(acsr2.getValue<AttrType::v3>() - 2.2f);
    acsr1.fromModel(acsr2.getModel());
    xml = acsr1.toXml("state4");
    std::cout << "acsr1: "<< xml->toString() << "\n";
    
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
        mAccessor.fromModel(copy, NotificationType::synchronous);
    }
    else if(mAudioFormatManager.getWildcardForAllFormats().contains(fileExtension))
    {
        Document::Model model;
        model.file = file;
        mDocumentFileBased.setFile({});
        mDocumentAccessor.fromModel(model, NotificationType::synchronous);
        auto copy = mAccessor.getModel();
        copy.currentDocumentFile = juce::File{};
        mAccessor.fromModel(copy, NotificationType::synchronous);
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
