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
    loadFromFile(PropertyType::InternetPlugins);

    Instance::get().getApplicationAccessor().addListener(mApplicationListener, NotificationType::synchronous);
    Instance::get().getPluginListAccessor().addListener(mPluginListListener, NotificationType::synchronous);
    Instance::get().getAudioDeviceManager().addChangeListener(this);

    saveToFile(PropertyType::Application);
    saveToFile(PropertyType::PluginList);
    saveToFile(PropertyType::AudioSetup);

    // Start downloading internet plugin list
    Instance::get().getInternetPluginManager().downloadPluginList([this](bool success)
                                                                  {
                                                                      if(success)
                                                                      {
                                                                          saveToFile(PropertyType::InternetPlugins);
                                                                      }
                                                                  });
}

Application::Properties::~Properties()
{
    Instance::get().getAudioDeviceManager().removeChangeListener(this);
    Instance::get().getPluginListAccessor().removeListener(mPluginListListener);
    Instance::get().getApplicationAccessor().removeListener(mApplicationListener);
}

void Application::Properties::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    anlWeakAssert(source == std::addressof(Instance::get().getAudioDeviceManager()));
    if(source == std::addressof(Instance::get().getAudioDeviceManager()))
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
        case PropertyType::InternetPlugins:
        {
            auto xml = Instance::get().getInternetPluginManager().toXml();
            if(xml != nullptr)
            {
                writeTo(std::move(xml), "internetplugins.settings");
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
            std::vector<juce::File> searchPath;
            PluginList::Accessor copy;
            auto xml = juce::parseXML(getFile("plugin.settings"));
            if(xml != nullptr)
            {
                auto& acsr = Instance::get().getPluginListAccessor();
                acsr.fromXml(*xml, "PluginList", NotificationType::synchronous);
                copy.copyFrom(acsr, NotificationType::synchronous);
                searchPath = acsr.getAttr<PluginList::AttrType::searchPath>();
            }
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
            searchPath.insert(searchPath.begin(), getPluginPackageDirectory());
            copy.setAttr<PluginList::AttrType::searchPath>(searchPath, NotificationType::synchronous);
            PluginList::setEnvironment(copy, getFile("plugin.blacklist"));
        }
        break;
        case PropertyType::AudioSetup:
        {
            auto xml = juce::parseXML(getFile("audio.settings"));
            auto const error = Instance::get().getAudioDeviceManager().initialise(0, sMaxIONumber, xml.get(), true);
            if(error.isNotEmpty())
            {
                askToRestoreDefaultAudioSettings(error);
            }
        }
        break;
        case PropertyType::InternetPlugins:
        {
            auto xml = juce::parseXML(getFile("internetplugins.settings"));
            if(xml != nullptr)
            {
                Instance::get().getInternetPluginManager().loadFromXml(*xml);
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
