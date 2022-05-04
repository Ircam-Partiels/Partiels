#include "AnlApplicationProperties.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Properties::Properties()
{
    mApplicationListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        saveToFile(PropertyType::Application);
    };

    mPluginListListener.onAttrChanged = [this](PluginList::Accessor const& acsr, PluginList::AttrType attribute)
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

    Instance::get().getApplicationAccessor().addListener(mApplicationListener, NotificationType::synchronous);
    Instance::get().getPluginListAccessor().addListener(mPluginListListener, NotificationType::synchronous);
    Instance::get().getAudioDeviceManager().addChangeListener(this);

    saveToFile(PropertyType::Application);
    saveToFile(PropertyType::PluginList);
    saveToFile(PropertyType::AudioSetup);
}

Application::Properties::~Properties()
{
    Instance::get().getAudioDeviceManager().removeChangeListener(this);
    Instance::get().getPluginListAccessor().removeListener(mPluginListListener);
    Instance::get().getApplicationAccessor().removeListener(mApplicationListener);
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

    switch(type)
    {
        case PropertyType::Application:
        {
            writeTo(Instance::get().getApplicationAccessor().toXml("AppSettings"), "application.settings");
        }
        break;
        case PropertyType::PluginList:
        {
            writeTo(Instance::get().getPluginListAccessor().toXml("PluginList"), "plugin.settings");
        }
        break;
        case PropertyType::AudioSetup:
        {
            auto xml = Instance::get().getAudioDeviceManager().createStateXml();
            if(xml != nullptr)
            {
                writeTo(std::move(xml), "audio.settings");
            }
        }
        break;
    }
}

void Application::Properties::loadFromFile(PropertyType type)
{
    anlStrongAssert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    switch(type)
    {
        case PropertyType::Application:
        {
            auto xml = juce::parseXML(getFile("application.settings"));
            if(xml != nullptr)
            {
                auto& acsr = Instance::get().getApplicationAccessor();
                acsr.fromXml(*xml, "AppSettings", NotificationType::synchronous);
            }
        }
        break;
        case PropertyType::PluginList:
        {
            auto xml = juce::parseXML(getFile("plugin.settings"));
            if(xml != nullptr)
            {
                auto& acsr = Instance::get().getPluginListAccessor();
                acsr.fromXml(*xml, "PluginList", NotificationType::synchronous);
                PluginList::Accessor copy;
                copy.copyFrom(acsr, NotificationType::synchronous);
                auto const getPluginPackageDirectory = []()
                {
                    auto const exeFile = juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentExecutableFile);
#if JUCE_MAC
                    auto const pluginPackage = exeFile.getParentDirectory().getSiblingFile("PlugIns");
                    auto const files = pluginPackage.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*.dylib");
                    PluginList::removeLibrariesFromQuarantine({files.begin(), files.end()});
                    return pluginPackage;
#else
                    return exeFile.getSiblingFile("PlugIns");
#endif
                };
                auto searchPath = acsr.getAttr<PluginList::AttrType::searchPath>();
                searchPath.insert(searchPath.begin(), getPluginPackageDirectory());
                copy.setAttr<PluginList::AttrType::searchPath>(searchPath, NotificationType::synchronous);
                PluginList::setEnvironment(copy);
            }
        }
        break;
        case PropertyType::AudioSetup:
        {
            auto xml = juce::parseXML(getFile("audio.settings"));
            auto& manager = Instance::get().getAudioDeviceManager();
            auto const error = manager.initialise(0, sMaxIONumber, xml.get(), true);
            if(error.isNotEmpty())
            {
                askToRestoreDefaultAudioSettings(error);
            }
        }
        break;
    }
}

void Application::Properties::askToRestoreDefaultAudioSettings(juce::String const& error)
{
    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::QuestionIcon)
                             .withTitle(juce::translate("Error loading audio device settings!"))
                             .withMessage(juce::translate("An error accured while loading audio device settings: ERROR. Do you want to restore the default settings?").replace("ERROR", error))
                             .withButton(juce::translate("Restore Default Settings"))
                             .withButton(juce::translate("Ignore"));

    juce::AlertWindow::showAsync(options, [=](int result)
                                 {
                                     if(result == 1)
                                     {
                                         auto& audioDeviceManager = Instance::get().getAudioDeviceManager();
                                         audioDeviceManager.initialiseWithDefaultDevices(0, sMaxIONumber);
                                     }
                                 });
}

ANALYSE_FILE_END
