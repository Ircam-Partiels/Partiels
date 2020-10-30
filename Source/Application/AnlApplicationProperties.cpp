#include "AnlApplicationProperties.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Properties::Properties()
{
    mApplicationListener.onChanged = [this](Accessor const& acsr, Model::Attribute attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        saveToFile(PropertyType::Application);
    };
    
    mPluginListListener.onChanged = [this](PluginList::Accessor const& acsr, PluginList::Model::Attribute attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        saveToFile(PropertyType::PluginList);
    };
    
    if(getFile("").getParentDirectory().createDirectory().failed())
    {
        anlStrongAssert(false && "cannot create parent directory");
    }
    
    loadFromFile(PropertyType::Application);
    loadFromFile(PropertyType::PluginList);
    loadFromFile(PropertyType::AudioSetup);
    
    Instance::get().getAccessor().addListener(mApplicationListener, juce::NotificationType::dontSendNotification);
    Instance::get().getPluginListAccessor().addListener(mPluginListListener, juce::NotificationType::dontSendNotification);
    Instance::get().getAudioDeviceManager().addChangeListener(this);
    
    saveToFile(PropertyType::Application);
    saveToFile(PropertyType::PluginList);
    saveToFile(PropertyType::AudioSetup);
}

Application::Properties::~Properties()
{
    Instance::get().getAudioDeviceManager().removeChangeListener(this);
    Instance::get().getPluginListAccessor().removeListener(mPluginListListener);
    Instance::get().getAccessor().removeListener(mApplicationListener);
}

void Application::Properties::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    anlStrongAssert(source == &Instance::get().getAudioDeviceManager());
    if(source == &Instance::get().getAudioDeviceManager())
    {
        saveToFile(PropertyType::AudioSetup);
    }
}

juce::File Application::Properties::getFile(juce::StringRef const& fileName)
{
    juce::PropertiesFile::Options options;
    options.applicationName = ProjectInfo::projectName;
    options.folderName = "Ircam";
    options.filenameSuffix = fileName;
    options.osxLibrarySubFolder = "Application Support";
    return options.getDefaultFile();
}

void Application::Properties::saveToFile(PropertyType type)
{
    anlStrongAssert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    auto writeTo = [](std::unique_ptr<juce::XmlElement> xml, juce::StringRef const& fileName)
    {
        anlStrongAssert(xml != nullptr);
        if(xml != nullptr)
        {
            if(!xml->writeTo(getFile(fileName)))
            {
                anlWeakAssert(false && "cannot write to file");
            }
        }
    };
    
    switch (type)
    {
        case PropertyType::Application:
        {
            writeTo(Instance::get().getAccessor().getModel().toXml(), "application.settings");
        }
            break;
        case PropertyType::PluginList:
        {
            writeTo(Instance::get().getPluginListAccessor().getModel().toXml(), "pluginlist.settings");
        }
            break;
        case PropertyType::AudioSetup:
        {
            auto xml = Instance::get().getAudioDeviceManager().createStateXml();
            if(xml != nullptr)
            {
                writeTo(std::move(xml), "audiosetup.settings");
            }
        }
            break;
    }
}

void Application::Properties::loadFromFile(PropertyType type)
{
    anlStrongAssert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    switch (type)
    {
        case PropertyType::Application:
        {
            auto xml = juce::parseXML(getFile("application.settings"));
            if(xml != nullptr)
            {
                auto& acsr = Instance::get().getAccessor();
                acsr.fromXml(*xml, acsr.getModel(), juce::NotificationType::sendNotification);
            }
        }
            break;
        case PropertyType::PluginList:
        {
            auto xml = juce::parseXML(getFile("pluginlist.settings"));
            if(xml != nullptr)
            {
                auto& acsr = Instance::get().getPluginListAccessor();
                acsr.fromXml(*xml, acsr.getModel(), juce::NotificationType::sendNotification);
            }
        }
            break;
        case PropertyType::AudioSetup:
        {
            auto xml = juce::parseXML(getFile("audiosetup.settings"));
            auto& manager = Instance::get().getAudioDeviceManager();
            manager.initialise(sMaxIONumber, sMaxIONumber, xml.get(), true);
        }
            break;
    }
}

ANALYSE_FILE_END
