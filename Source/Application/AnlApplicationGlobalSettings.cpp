#include "AnlApplicationGlobalSettings.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationMainMenuModel.h"

ANALYSE_FILE_BEGIN

namespace
{
    // clang-format off
    enum DefaultTemplateId : int
    {
          noneTemplateId = 1
        , factoryTemplateId
        , customTemplateId
        , selectTemplateId
    };

    enum QuickExportDirectoryId : int
    {
          desktopDirectoryId = 1
        , customDirectoryId
        , selectDirectoryId
    };
    // clang-format on

    juce::StringArray getThemeNames()
    {
        static_assert(magic_enum::enum_count<Application::ColourMode>() == static_cast<size_t>(4));
        return {juce::translate("Night"), juce::translate("Day"), juce::translate("Grass"), juce::translate("Automatic")};
    }
} // namespace

Application::GlobalSettingsContent::GlobalSettingsContent()
: mPropertyTheme(juce::translate("Theme"), juce::translate("The theme of the application (the automatic mode follows the light or dark appearance of the operating system)"), "", getThemeNames(), [](size_t index)
                 {
                     auto const mode = magic_enum::enum_cast<ColourMode>(static_cast<int>(index));
                     if(mode.has_value())
                     {
                         Instance::get().getApplicationAccessor().setAttr<AttrType::colourMode>(mode.value(), NotificationType::synchronous);
                     }
                 })
, mPropertySilentFileManagement(juce::translate("Silent File Management"), juce::translate("Automatically reload the results files of the tracks when they are modified or restored, and do not warn when they are removed"), [](bool state)
                                {
                                    Instance::get().getApplicationAccessor().setAttr<AttrType::silentFileManagement>(state, NotificationType::synchronous);
                                })
, mPropertyIgnoreTimeSelection(juce::translate("Ignore Time Selection During Quick Export"), juce::translate("Export all the frames regardless of the time selection when using the quick export commands"), [](bool state)
                               {
                                   Instance::get().getApplicationAccessor().setAttr<AttrType::ignoreTimeSelectionDuringQuickExport>(state, NotificationType::synchronous);
                               })
, mPropertyPreserveFullDuration(juce::translate("Preserve Full Duration When Editing"), juce::translate("Preserve the frame durations to their maximum value during the editing operations"), [](bool state)
                                {
                                    Instance::get().getApplicationAccessor().setAttr<AttrType::preserveFullDurationWhenEditing>(state, NotificationType::synchronous);
                                })
, mPropertyAutoUpdate(juce::translate("Automatic Check for New Version"), juce::translate("Check for a new version when the application starts"), [](bool state)
                      {
                          Instance::get().getApplicationAccessor().setAttr<AttrType::autoUpdate>(state, NotificationType::synchronous);
                      })
, mPropertyIncludePreReleases(juce::translate("Include Pre-Release Versions in Update Checks"), juce::translate("Also consider the pre-release versions when checking for a new version"), [](bool state)
                              {
                                  Instance::get().getApplicationAccessor().setAttr<AttrType::includePreReleases>(state, NotificationType::synchronous);
                              })
, mPropertyDefaultTemplate(juce::translate("Default Template"), juce::translate("The template used when loading an audio file"), "", {}, [this]([[maybe_unused]] size_t index)
                           {
                               auto& accessor = Instance::get().getApplicationAccessor();
                               switch(mPropertyDefaultTemplate.entry.getSelectedId())
                               {
                                   case DefaultTemplateId::noneTemplateId:
                                   {
                                       accessor.setAttr<AttrType::defaultTemplateFile>(juce::File(), NotificationType::synchronous);
                                       break;
                                   }
                                   case DefaultTemplateId::factoryTemplateId:
                                   {
                                       accessor.setAttr<AttrType::defaultTemplateFile>(Accessor::getFactoryTemplateFile(), NotificationType::synchronous);
                                       break;
                                   }
                                   case DefaultTemplateId::selectTemplateId:
                                   {
                                       // The selection is restored until the file is selected asynchronously
                                       updateDefaultTemplate();
                                       if(auto* window = Instance::get().getWindow())
                                       {
                                           window->getCommandTarget().selectDefaultTemplateFile();
                                       }
                                       break;
                                   }
                                   default:
                                   {
                                       updateDefaultTemplate();
                                       break;
                                   }
                               }
                           })
, mPropertyQuickExportDirectory(juce::translate("Quick Export Directory"), juce::translate("The directory used to export the frames with the quick export commands"), "", {}, [this]([[maybe_unused]] size_t index)
                                {
                                    switch(mPropertyQuickExportDirectory.entry.getSelectedId())
                                    {
                                        case QuickExportDirectoryId::desktopDirectoryId:
                                        {
                                            auto const desktop = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory);
                                            Instance::get().getApplicationAccessor().setAttr<AttrType::quickExportDirectory>(desktop, NotificationType::synchronous);
                                            break;
                                        }
                                        case QuickExportDirectoryId::selectDirectoryId:
                                        {
                                            // The selection is restored until the directory is selected asynchronously
                                            updateQuickExportDirectory();
                                            if(auto* window = Instance::get().getWindow())
                                            {
                                                window->getCommandTarget().selectQuickExportDirectory();
                                            }
                                            break;
                                        }
                                        default:
                                        {
                                            updateQuickExportDirectory();
                                            break;
                                        }
                                    }
                                })
, mPropertyTranslation(juce::translate("Language"), juce::translate("The language of the application"), "", {}, [this]([[maybe_unused]] size_t index)
                       {
                           auto const selectedIndex = mPropertyTranslation.entry.getSelectedId() - 1;
                           if(selectedIndex < 0 || static_cast<size_t>(selectedIndex) >= mTranslationFiles.size())
                           {
                               updateTranslation();
                               return;
                           }
                           // The interface (including this panel) is recreated when the translation changes,
                           // so the change is performed asynchronously and the panel is shown again afterward
                           auto const file = mTranslationFiles.at(static_cast<size_t>(selectedIndex));
                           juce::MessageManager::callAsync([=]()
                                                           {
                                                               Instance::get().getApplicationAccessor().setAttr<AttrType::currentTranslationFile>(file, NotificationType::synchronous);
                                                               if(auto* window = Instance::get().getWindow())
                                                               {
                                                                   window->getInterface().showGlobalSettingsPanel();
                                                               }
                                                           });
                       })
{
    addAndMakeVisible(mPropertyTheme);
    addAndMakeVisible(mPropertySilentFileManagement);
    addAndMakeVisible(mPropertyIgnoreTimeSelection);
    addAndMakeVisible(mPropertyPreserveFullDuration);
    addAndMakeVisible(mPropertyAutoUpdate);
    addAndMakeVisible(mPropertyIncludePreReleases);
    addAndMakeVisible(mPropertyDefaultTemplate);
    addAndMakeVisible(mPropertyQuickExportDirectory);
    addAndMakeVisible(mPropertyTranslation);

    setSize(300, 200);

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attr)
    {
        switch(attr)
        {
            case AttrType::colourMode:
            {
                mPropertyTheme.entry.setSelectedId(static_cast<int>(acsr.getAttr<AttrType::colourMode>()) + 1, juce::NotificationType::dontSendNotification);
                break;
            }
            case AttrType::autoUpdate:
            {
                mPropertyAutoUpdate.entry.setToggleState(acsr.getAttr<AttrType::autoUpdate>(), juce::NotificationType::dontSendNotification);
                break;
            }
            case AttrType::silentFileManagement:
            {
                mPropertySilentFileManagement.entry.setToggleState(acsr.getAttr<AttrType::silentFileManagement>(), juce::NotificationType::dontSendNotification);
                break;
            }
            case AttrType::ignoreTimeSelectionDuringQuickExport:
            {
                mPropertyIgnoreTimeSelection.entry.setToggleState(acsr.getAttr<AttrType::ignoreTimeSelectionDuringQuickExport>(), juce::NotificationType::dontSendNotification);
                break;
            }
            case AttrType::preserveFullDurationWhenEditing:
            {
                mPropertyPreserveFullDuration.entry.setToggleState(acsr.getAttr<AttrType::preserveFullDurationWhenEditing>(), juce::NotificationType::dontSendNotification);
                break;
            }
            case AttrType::includePreReleases:
            {
                mPropertyIncludePreReleases.entry.setToggleState(acsr.getAttr<AttrType::includePreReleases>(), juce::NotificationType::dontSendNotification);
                break;
            }
            case AttrType::defaultTemplateFile:
            {
                updateDefaultTemplate();
                break;
            }
            case AttrType::quickExportDirectory:
            {
                updateQuickExportDirectory();
                break;
            }
            case AttrType::currentTranslationFile:
            {
                updateTranslation();
                break;
            }
            case AttrType::desktopGlobalScaleFactor:
            case AttrType::windowState:
            case AttrType::recentlyOpenedFilesList:
            case AttrType::currentDocumentFile:
            case AttrType::showInfoBubble:
            case AttrType::exportOptions:
            case AttrType::adaptationToSampleRate:
            case AttrType::autoLoadConvertedFile:
            case AttrType::routingMatrix:
            case AttrType::lastVersion:
            case AttrType::timeZoomAnchorOnPlayhead:
            case AttrType::globalGraphicPreset:
            case AttrType::showTooltipBubble:
                break;
        }
    };

    Instance::get().getApplicationAccessor().addListener(mListener, NotificationType::synchronous);
}

Application::GlobalSettingsContent::~GlobalSettingsContent()
{
    Instance::get().getApplicationAccessor().removeListener(mListener);
}

void Application::GlobalSettingsContent::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto const setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyTheme);
    setBounds(mPropertySilentFileManagement);
    setBounds(mPropertyIgnoreTimeSelection);
    setBounds(mPropertyPreserveFullDuration);
    setBounds(mPropertyAutoUpdate);
    setBounds(mPropertyIncludePreReleases);
    setBounds(mPropertyDefaultTemplate);
    setBounds(mPropertyQuickExportDirectory);
    setBounds(mPropertyTranslation);
    setSize(getWidth(), bounds.getY());
}

void Application::GlobalSettingsContent::visibilityChanged()
{
    // The template file, the quick export directory and the translation files
    // might have been moved or removed while the panel was hidden
    if(isShowing())
    {
        updateDefaultTemplate();
        updateQuickExportDirectory();
        updateTranslation();
    }
}

void Application::GlobalSettingsContent::updateDefaultTemplate()
{
    auto const templateFile = Instance::get().getApplicationAccessor().getAttr<AttrType::defaultTemplateFile>();
    auto const factoryFile = Accessor::getFactoryTemplateFile();
    auto& entry = mPropertyDefaultTemplate.entry;
    entry.clear(juce::NotificationType::dontSendNotification);
    entry.addItem(juce::translate("None"), DefaultTemplateId::noneTemplateId);
    entry.addItem(juce::translate("Factory"), DefaultTemplateId::factoryTemplateId);
    auto const isCustom = templateFile.existsAsFile() && templateFile != factoryFile;
    if(isCustom)
    {
        entry.addItem(templateFile.getFileNameWithoutExtension(), DefaultTemplateId::customTemplateId);
    }
    entry.addSeparator();
    entry.addItem(juce::translate("Select..."), DefaultTemplateId::selectTemplateId);
    auto const selectedId = isCustom ? DefaultTemplateId::customTemplateId : (templateFile.existsAsFile() ? DefaultTemplateId::factoryTemplateId : DefaultTemplateId::noneTemplateId);
    entry.setSelectedId(selectedId, juce::NotificationType::dontSendNotification);
    entry.setTooltip(templateFile.existsAsFile() ? templateFile.getFullPathName() : juce::translate("The template used when loading an audio file"));
}

void Application::GlobalSettingsContent::updateQuickExportDirectory()
{
    auto const directory = Instance::get().getApplicationAccessor().getAttr<AttrType::quickExportDirectory>();
    auto const desktop = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory);
    auto& entry = mPropertyQuickExportDirectory.entry;
    entry.clear(juce::NotificationType::dontSendNotification);
    entry.addItem(juce::translate("Desktop"), QuickExportDirectoryId::desktopDirectoryId);
    auto const isCustom = directory.isDirectory() && directory != desktop;
    if(isCustom)
    {
        entry.addItem(directory.getFileName(), QuickExportDirectoryId::customDirectoryId);
    }
    entry.addSeparator();
    entry.addItem(juce::translate("Select..."), QuickExportDirectoryId::selectDirectoryId);
    if(isCustom)
    {
        entry.setSelectedId(QuickExportDirectoryId::customDirectoryId, juce::NotificationType::dontSendNotification);
    }
    else if(directory == desktop)
    {
        entry.setSelectedId(QuickExportDirectoryId::desktopDirectoryId, juce::NotificationType::dontSendNotification);
    }
    entry.setTooltip(directory.isDirectory() ? directory.getFullPathName() : juce::translate("The directory used to export the frames with the quick export commands"));
}

void Application::GlobalSettingsContent::updateTranslation()
{
    auto const currentTranslationFile = Instance::get().getApplicationAccessor().getAttr<AttrType::currentTranslationFile>();
    auto& entry = mPropertyTranslation.entry;
    entry.clear(juce::NotificationType::dontSendNotification);
    mTranslationFiles.clear();

    auto const addItem = [&](juce::String const& name, juce::File const& file)
    {
        mTranslationFiles.push_back(file);
        auto const itemId = static_cast<int>(mTranslationFiles.size());
        entry.addItem(name, itemId);
        if(file == currentTranslationFile)
        {
            entry.setSelectedId(itemId, juce::NotificationType::dontSendNotification);
        }
    };

    auto const addTranslationsFromDirectory = [&](juce::File const& folder)
    {
        auto const files = folder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*.txt", juce::File::FollowSymlinks::no);
        if(!files.isEmpty())
        {
            entry.addSeparator();
        }
        for(auto const& file : files)
        {
            auto const languageName = juce::LocalisedStrings(file, false).getLanguageName();
            auto const hasDuplicate = std::count_if(files.begin(), files.end(), [&](auto const& f)
                                                    {
                                                        return languageName == juce::LocalisedStrings(f, false).getLanguageName();
                                                    }) > 1l;
            addItem(languageName + (hasDuplicate ? " (" + file.getFileNameWithoutExtension() + ")" : ""), file);
        }
    };

    auto const defaultTranslationFile = MainMenuModel::getSystemDefaultTranslationFile();
    auto const defaultTranslationLanguage = juce::LocalisedStrings(defaultTranslationFile, false).getLanguageName();
    addItem(juce::translate("Default (TRLANGUAGE)").replace("TRLANGUAGE", defaultTranslationLanguage.isEmpty() ? +"English" : defaultTranslationLanguage), juce::File());
    addTranslationsFromDirectory(MainMenuModel::getEmbeddedTranslationsDirectory());
    addTranslationsFromDirectory(MainMenuModel::getUserTranslationsDirectory());
}

Application::GlobalSettingsPanel::GlobalSettingsPanel()
: HideablePanelTyped<GlobalSettingsContent>(juce::translate("Global Settings"))
{
}

ANALYSE_FILE_END
