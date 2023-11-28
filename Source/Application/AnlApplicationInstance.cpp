#include "AnlApplicationInstance.h"
#include "AnlApplicationTools.h"
#include <TranslationsData.h>

ANALYSE_FILE_BEGIN

#define AppQuitIfInvalidPointer(ptr)                    \
    if(ptr == nullptr)                                  \
    {                                                   \
        anlDebug("Application", "Allocation Failed!."); \
        setApplicationReturnValue(-1);                  \
        quit();                                         \
        return;                                         \
    }

juce::String const Application::Instance::getApplicationName()
{
    return ProjectInfo::projectName;
}

juce::String const Application::Instance::getApplicationVersion()
{
    return juce::String(PARTIELS_BUILD_TAG).isEmpty() ? juce::String(ProjectInfo::versionString) : juce::String(PARTIELS_BUILD_TAG);
}

bool Application::Instance::moreThanOneInstanceAllowed()
{
    return true;
}

void Application::Instance::initialise(juce::String const& commandLine)
{
    anlDebug("Application", "Begin...");
    anlDebug("Application", "Command line '" + commandLine + "'");

    mCommandLine = CommandLine::createAndRun(commandLine);
    if(mCommandLine != nullptr)
    {
        return;
    }

    anlDebug("Application", "Running with GUI");
    TranslationManager::loadFromBinaries();

    juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDocumentsDirectory).getChildFile("Ircam").setAsCurrentWorkingDirectory();

    mLookAndFeel = std::make_unique<LookAndFeel>();
    AppQuitIfInvalidPointer(mLookAndFeel);
    juce::LookAndFeel::setDefaultLookAndFeel(mLookAndFeel.get());

    mApplicationCommandManager = std::make_unique<juce::ApplicationCommandManager>();
    AppQuitIfInvalidPointer(mApplicationCommandManager);

    mAudioFormatManager = std::make_unique<juce::AudioFormatManager>();
    AppQuitIfInvalidPointer(mAudioFormatManager);
    mAudioFormatManager->registerBasicFormats();

    mAudioDeviceManager = std::make_unique<juce::AudioDeviceManager>();
    AppQuitIfInvalidPointer(mAudioDeviceManager);

    mUndoManager = std::make_unique<juce::UndoManager>();
    AppQuitIfInvalidPointer(mUndoManager);

    mApplicationAccessor = std::make_unique<Accessor>();
    AppQuitIfInvalidPointer(mApplicationAccessor);

    mApplicationListener = std::make_unique<Accessor::Listener>(typeid(*this).name());
    AppQuitIfInvalidPointer(mApplicationListener);

    mPluginListAccessor = std::make_unique<PluginList::Accessor>();
    AppQuitIfInvalidPointer(mPluginListAccessor);

    mPluginListScanner = std::make_unique<PluginList::Scanner>();
    AppQuitIfInvalidPointer(mPluginListScanner);

    mDocumentAccessor = std::make_unique<Document::Accessor>();
    AppQuitIfInvalidPointer(mDocumentAccessor);

    mDocumentDirector = std::make_unique<Document::Director>(*mDocumentAccessor.get(), *mAudioFormatManager.get(), *mUndoManager.get());
    AppQuitIfInvalidPointer(mDocumentDirector);
    mDocumentDirector->setBackupDirectory(getBackupFile().getSiblingFile("Tracks"));

    mAudioReader = std::make_unique<AudioReader>();
    AppQuitIfInvalidPointer(mAudioReader);

    mProperties = std::make_unique<Properties>();
    AppQuitIfInvalidPointer(mProperties);

    mDocumentFileBased = std::make_unique<Document::FileBased>(*mDocumentDirector.get(), getExtensionForDocumentFile(), getWildCardForDocumentFile(), "Open a document", "Save the document");
    AppQuitIfInvalidPointer(mDocumentFileBased);

    mWindow = std::make_unique<Window>();
    AppQuitIfInvalidPointer(mWindow);

    mMainMenuModel = std::make_unique<MainMenuModel>(*mWindow.get());
    AppQuitIfInvalidPointer(mMainMenuModel);

    mDownloader = std::make_unique<Downloader>();
    AppQuitIfInvalidPointer(mDownloader);

    checkPluginsQuarantine();

    mApplicationListener->onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::currentDocumentFile:
            case AttrType::windowState:
            case AttrType::exportOptions:
            case AttrType::adaptationToSampleRate:
            case AttrType::autoLoadConvertedFile:
            case AttrType::routingMatrix:
            case AttrType::lastVersion:
            case AttrType::timeZoomAnchorOnPlayhead:
                break;
            case AttrType::autoUpdate:
            case AttrType::recentlyOpenedFilesList:
            case AttrType::defaultTemplateFile:
            case AttrType::showInfoBubble:
            {
                mMainMenuModel->menuItemsChanged();
#ifdef JUCE_MAC
                mMainMenuModel->updateAppleMenuItems();
#endif
            }
            break;
            case AttrType::desktopGlobalScaleFactor:
            {
                auto const windowState = acsr.getAttr<AttrType::windowState>();
                auto const scale = acsr.getAttr<AttrType::desktopGlobalScaleFactor>();
                juce::Desktop::getInstance().setGlobalScaleFactor(scale);
                if(mWindow != nullptr && !windowState.isEmpty())
                {
                    mWindow->restoreWindowStateFromString(windowState);
                }
                mMainMenuModel->menuItemsChanged();
            }
            break;
            case AttrType::colourMode:
            {
                updateLookAndFeel();
            }
            break;
        }
    };
    mApplicationAccessor->addListener(*mApplicationListener.get(), NotificationType::synchronous);

    juce::Desktop::getInstance().addDarkModeSettingListener(this);

    anlDebug("Application", "Ready!");

    juce::ArgumentList const args("Partiels", commandLine);
    mCommandFiles.clear();
    for(int i = 0; i < args.size(); ++i)
    {
        auto const file = args[i].resolveAsFile();
        if(file.existsAsFile())
        {
            mCommandFiles.push_back(file);
        }
    }
    mPreviousFile = mApplicationAccessor->getAttr<AttrType::currentDocumentFile>();
    triggerAsyncUpdate();
}

void Application::Instance::anotherInstanceStarted(juce::String const& commandLine)
{
    if(mWindow == nullptr)
    {
        anlDebug("Application", "Failed: not initialized.");
        return;
    }

    juce::ArgumentList const args("Partiels", commandLine);
    std::vector<juce::File> files;
    for(int i = 0; i < args.size(); ++i)
    {
        auto const file = args[i].resolveAsFile();
        if(file.existsAsFile())
        {
            files.push_back(file);
        }
    }
    if(!mStartupFilesAreLoaded)
    {
        mCommandFiles = std::move(files);
        triggerAsyncUpdate();
    }
    else
    {
        anlDebug("Application", "Opening new file(s)...");
        openFiles(files);
    }
}

void Application::Instance::systemRequestedQuit()
{
    auto delaySystemRequestedQuit = [this]()
    {
        anlDebug("Application", "Delay System Requested Quit...");
        juce::WeakReference<Instance> weakReference(this);
        juce::Timer::callAfterDelay(500, [=, this]()
                                    {
                                        if(weakReference.get() == nullptr)
                                        {
                                            return;
                                        }
                                        systemRequestedQuit();
                                    });
    };

    anlDebug("Application", "Begin...");
    if(mCommandLine != nullptr && mCommandLine->isRunning())
    {
        delaySystemRequestedQuit();
        return;
    }

    if(mApplicationAccessor != nullptr)
    {
        mApplicationAccessor->setAttr<AttrType::currentDocumentFile>(mDocumentFileBased->getFile(), NotificationType::synchronous);
    }

    if(auto* modalComponentManager = juce::ModalComponentManager::getInstance())
    {
        if(modalComponentManager->cancelAllModalComponents())
        {
            delaySystemRequestedQuit();
            return;
        }
    }

    auto saveAndQuit = [this]()
    {
        if(mDocumentFileBased != nullptr)
        {
            mDocumentFileBased->saveIfNeededAndUserAgreesAsync([](juce::FileBasedDocument::SaveResult result)
                                                               {
                                                                   if(result == juce::FileBasedDocument::SaveResult::savedOk)
                                                                   {
                                                                       anlDebug("Application", "Ready to Quit");
                                                                       quit();
                                                                   }
                                                               });
        }
        else
        {
            anlDebug("Application", "Ready to Quit");
            quit();
        }
    };

    if(mDocumentAccessor != nullptr)
    {
        auto const trackAcsrs = mDocumentAccessor->getAcsrs<Document::AcsrType::tracks>();
        auto const isRunning = std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [](auto const& trackAcsr)
                                           {
                                               auto const& processing = trackAcsr.get().template getAttr<Track::AttrType::processing>();
                                               return std::get<0>(processing) || std::get<2>(processing);
                                           });
        if(isRunning)
        {
            auto const options = juce::MessageBoxOptions()
                                     .withIconType(juce::AlertWindow::QuestionIcon)
                                     .withTitle(juce::translate("Some analyses are still running!"))
                                     .withMessage(juce::translate("Do you still want to quit the app? Note that a running analysis may block the application from closing until it completes."))
                                     .withButton(juce::translate("Quit"))
                                     .withButton(juce::translate("Cancel"));

            juce::AlertWindow::showAsync(options, [=](int result)
                                         {
                                             if(result == 1)
                                             {
                                                 saveAndQuit();
                                             }
                                         });
            return;
        }
    }
    saveAndQuit();
}

void Application::Instance::shutdown()
{
    juce::Desktop::getInstance().removeDarkModeSettingListener(this);

    anlDebug("Application", "Begin...");
    if(mCommandLine != nullptr)
    {
        mCommandLine.reset();
        return;
    }

    if(mApplicationAccessor != nullptr)
    {
        mApplicationAccessor->removeListener(*mApplicationListener.get());
    }
    if(mDocumentFileBased != nullptr)
    {
        mDocumentFileBased->removeChangeListener(this);
    }
    auto backupFile = getBackupFile();
    backupFile.deleteFile();
    backupFile.getSiblingFile("Tracks").deleteRecursively();
    Document::FileBased::getConsolidateDirectory(backupFile).deleteRecursively();

    mDownloader.reset();
    mMainMenuModel.reset();
    mWindow.reset();
    mDocumentFileBased.reset();
    mAudioReader.reset();
    mProperties.reset();
    mDocumentDirector.reset();
    mDocumentAccessor.reset();
    mPluginListScanner.reset();
    mPluginListAccessor.reset();
    mApplicationListener.reset();
    mApplicationAccessor.reset();
    mUndoManager.reset();
    mAudioDeviceManager.reset();
    mAudioFormatManager.reset();
    mApplicationCommandManager.reset();

    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    mLookAndFeel.reset();

    anlDebug("Application", "Done");
}

juce::ApplicationCommandTarget* Application::Instance::getNextCommandTarget()
{
    return mWindow != nullptr ? dynamic_cast<juce::ApplicationCommandTarget*>(mWindow->getContentComponent()) : nullptr;
}

Application::Instance& Application::Instance::get()
{
    return *static_cast<Instance*>(JUCEApplication::getInstance());
}

juce::String Application::Instance::getExtensionForDocumentFile()
{
    return App::getFileExtensionFor("doc");
}

juce::String Application::Instance::getWildCardForDocumentFile()
{
    return App::getFileWildCardFor("doc");
}

juce::String Application::Instance::getWildCardForImportFormats()
{
    return Track::Loader::getWildCardForAllFormats();
}

juce::String Application::Instance::getWildCardForAudioFormats()
{
    return get().getAudioFormatManager().getWildcardForAllFormats();
}

std::pair<int, int> Application::Instance::getSizeFor(juce::String const& identifier)
{
    if(identifier.isEmpty())
    {
        return {0, 0};
    }
    if(auto* window = get().getWindow())
    {
        auto const bounds = juce::Desktop::getInstance().getDisplays().logicalToPhysical(window->getInterface().getPlotBounds(identifier));
        anlWeakAssert(!bounds.isEmpty());
        return {bounds.getWidth(), bounds.getHeight()};
    }
    return {0, 0};
}

LookAndFeel::ColourChart Application::Instance::getColourChart()
{
    auto& instance = get();
    MiscWeakAssert(instance.mApplicationAccessor != nullptr);
    if(instance.mApplicationAccessor == nullptr)
    {
        return {LookAndFeel::ColourChart::Mode::night};
    }

    switch(instance.mApplicationAccessor->getAttr<AttrType::colourMode>())
    {
        case ColourMode::night:
            return {LookAndFeel::ColourChart::Mode::night};
        case ColourMode::day:
            return {LookAndFeel::ColourChart::Mode::day};
        case ColourMode::grass:
            return {LookAndFeel::ColourChart::Mode::grass};
        case ColourMode::automatic:
        {
            if(juce::Desktop::getInstance().isDarkModeActive())
            {
                return {LookAndFeel::ColourChart::Mode::night};
            }
            return {LookAndFeel::ColourChart::Mode::day};
        }
    }
    return {LookAndFeel::ColourChart::Mode::night};
}

void Application::Instance::newDocument()
{
    if(mDocumentAccessor->isEquivalentTo(mDocumentFileBased->getDefaultAccessor()))
    {
        return;
    }
    juce::WeakReference<Instance> weakReference(this);
    mDocumentFileBased->saveIfNeededAndUserAgreesAsync([=, this](juce::FileBasedDocument::SaveResult saveResult)
                                                       {
                                                           if(weakReference.get() == nullptr)
                                                           {
                                                               return;
                                                           }
                                                           if(saveResult != juce::FileBasedDocument::SaveResult::savedOk)
                                                           {
                                                               return;
                                                           }
                                                           mUndoManager->clearUndoHistory();
                                                           mDocumentAccessor->copyFrom(mDocumentFileBased->getDefaultAccessor(), NotificationType::synchronous);
                                                           mDocumentFileBased->setFile({});
                                                           if(auto* window = mWindow.get())
                                                           {
                                                               window->getInterface().hidePluginListTablePanel();
                                                           }
                                                       });
}

void Application::Instance::openDocumentFile(juce::File const& file)
{
    auto const documentFileExtension = getExtensionForDocumentFile();
    anlWeakAssert(file.existsAsFile() && file.hasFileExtension(documentFileExtension));
    if(!file.existsAsFile() || !file.hasFileExtension(documentFileExtension) || file == mDocumentFileBased->getFile())
    {
        return;
    }
    mDocumentFileBased->saveIfNeededAndUserAgreesAsync([=, this](juce::FileBasedDocument::SaveResult saveResult)
                                                       {
                                                           if(saveResult != juce::FileBasedDocument::SaveResult::savedOk)
                                                           {
                                                               return;
                                                           }
                                                           mUndoManager->clearUndoHistory();
                                                           mDocumentFileBased->loadFrom(file, true);
                                                       });
}

void Application::Instance::openAudioFiles(std::vector<juce::File> const& files)
{
    auto const audioFileWilcards = getWildCardForAudioFormats();
    std::vector<AudioFileLayout> readerLayout;
    for(auto const& file : files)
    {
        auto const fileExtension = file.getFileExtension();
        anlWeakAssert(file.existsAsFile() && fileExtension.isNotEmpty() && audioFileWilcards.contains(fileExtension));
        if(file.existsAsFile() && fileExtension.isNotEmpty() && audioFileWilcards.contains(fileExtension))
        {
            auto reader = std::unique_ptr<juce::AudioFormatReader>(mAudioFormatManager->createReaderFor(file));
            if(reader != nullptr)
            {
                for(unsigned int channel = 0; channel < reader->numChannels; ++channel)
                {
                    readerLayout.push_back({file, static_cast<int>(channel)});
                }
            }
        }
    }
    if(!readerLayout.empty())
    {
        mDocumentFileBased->saveIfNeededAndUserAgreesAsync([=, this](juce::FileBasedDocument::SaveResult saveResult)
                                                           {
                                                               if(saveResult != juce::FileBasedDocument::SaveResult::savedOk)
                                                               {
                                                                   return;
                                                               }
                                                               mUndoManager->clearUndoHistory();
                                                               mDocumentAccessor->copyFrom(mDocumentFileBased->getDefaultAccessor(), NotificationType::synchronous);
                                                               mDocumentAccessor->setAttr<Document::AttrType::reader>(readerLayout, NotificationType::synchronous);
                                                               auto const templateFile = mApplicationAccessor->getAttr<AttrType::defaultTemplateFile>();
                                                               if(templateFile != juce::File{})
                                                               {
                                                                   mDocumentFileBased->loadTemplate(templateFile, mApplicationAccessor->getAttr<AttrType::adaptationToSampleRate>());
                                                               }
                                                               mDocumentFileBased->setFile({});
                                                           });
    }
}

void Application::Instance::openFiles(std::vector<juce::File> const& files)
{
    auto const documentFileExtension = getExtensionForDocumentFile();
    auto const documentFile = std::find_if(files.cbegin(), files.cend(), [&](auto const& file)
                                           {
                                               return file.existsAsFile() && file.hasFileExtension(documentFileExtension);
                                           });
    if(documentFile != files.cend())
    {
        openDocumentFile(*documentFile);
        return;
    }

    std::vector<juce::File> audioFiles;
    juce::StringArray array;
    auto const audioFormatsWildCard = getWildCardForAudioFormats();
    for(auto const& file : files)
    {
        auto const fileExtension = file.getFileExtension();
        if(file.existsAsFile() && fileExtension.isNotEmpty() && audioFormatsWildCard.contains(fileExtension))
        {
            audioFiles.push_back(file);
        }
        else
        {
            array.add(file.getFullPathName());
        }
    }
    if(!audioFiles.empty())
    {
        openAudioFiles(audioFiles);
        return;
    }

    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::WarningIcon)
                             .withTitle(juce::translate("File formats not supported!"))
                             .withMessage(juce::translate("The formats of the files are not supported by the application:\n FILEPATHS").replace("FILEPATHS", array.joinIntoString("\n")))
                             .withButton(juce::translate("Ok"));
    juce::AlertWindow::showAsync(options, nullptr);
}

void Application::Instance::importFile(std::tuple<juce::String, size_t> const position, juce::File const& file)
{
    auto const importFileWildcard = getWildCardForImportFormats();
    auto const fileExtension = file.getFileExtension();
    anlWeakAssert(file.existsAsFile() && fileExtension.isNotEmpty() && importFileWildcard.contains(fileExtension));
    auto* window = getWindow();
    if(window == nullptr || mDocumentAccessor == nullptr || !file.existsAsFile() || fileExtension.isEmpty() || !importFileWildcard.contains(fileExtension))
    {
        return;
    }
    auto const sampleRate = mDocumentAccessor->getAttr<Document::AttrType::samplerate>();
    if(window->getInterface().getTrackLoaderArgumentSelector().setFile(file, sampleRate, [position](Track::FileInfo fileInfo)
                                                                       {
                                                                           Tools::addFileTrack(position, fileInfo);
                                                                           if(auto* w = Instance::get().getWindow())
                                                                           {
                                                                               w->getInterface().hideCurrentPanel();
                                                                           }
                                                                       }))
    {
        window->getInterface().showTrackLoaderPanel();
    }
}

Application::Accessor& Application::Instance::getApplicationAccessor()
{
    return *mApplicationAccessor.get();
}

Application::Window* Application::Instance::getWindow()
{
    return mWindow.get();
}

PluginList::Accessor& Application::Instance::getPluginListAccessor()
{
    return *mPluginListAccessor.get();
}

PluginList::Scanner& Application::Instance::getPluginListScanner()
{
    return *mPluginListScanner.get();
}

Document::Accessor& Application::Instance::getDocumentAccessor()
{
    return *mDocumentAccessor.get();
}

Document::Director& Application::Instance::getDocumentDirector()
{
    return *mDocumentDirector.get();
}

Document::FileBased& Application::Instance::getDocumentFileBased()
{
    return *mDocumentFileBased.get();
}

juce::ApplicationCommandManager& Application::Instance::getApplicationCommandManager()
{
    return *mApplicationCommandManager.get();
}

juce::AudioFormatManager& Application::Instance::getAudioFormatManager()
{
    return *mAudioFormatManager.get();
}

juce::AudioDeviceManager& Application::Instance::getAudioDeviceManager()
{
    return *mAudioDeviceManager.get();
}

juce::UndoManager& Application::Instance::getUndoManager()
{
    return *mUndoManager.get();
}

void Application::Instance::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if(source == mDocumentFileBased.get())
    {
        auto const result = mDocumentFileBased->saveBackup(getBackupFile());
        if(result.failed())
        {
            MiscDebug("Application::Instance", result.getErrorMessage());
            MiscWeakAssert(false);
        }
    }
    else
    {
        MiscWeakAssert(false);
    }
}

void Application::Instance::handleAsyncUpdate()
{
    openStartupFiles();
}

juce::File Application::Instance::getBackupFile() const
{
    return juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory).getChildFile("backup").withFileExtension(getExtensionForDocumentFile());
}

void Application::Instance::openStartupFiles()
{
    if(!mIsPluginListReady)
    {
        triggerAsyncUpdate();
        return;
    }

    auto const commandFiles = std::move(mCommandFiles);
    auto const previousFile = std::move(mPreviousFile);
    if(!commandFiles.empty())
    {
        mDocumentFileBased->addChangeListener(this);
        anlDebug("Application", "Opening new file(s)...");
        openFiles(commandFiles);
        mStartupFilesAreLoaded = true;

        if(mApplicationAccessor->getAttr<AttrType::autoUpdate>())
        {
            checkForNewVersion(false, false);
        }
        return;
    }

    auto const backupFile = getBackupFile();
    if(backupFile.existsAsFile())
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::QuestionIcon)
                                 .withTitle(juce::translate("Do you want to restore your document?"))
                                 .withMessage(juce::translate("The application unexpectedly quit. Do you want to restore the last saved state of the document before this?"))
                                 .withButton(juce::translate("Restore"))
                                 .withButton(juce::translate("Ignore"));

        juce::WeakReference<Instance> weakReference(this);
        juce::AlertWindow::showAsync(options, [=, this](int result)
                                     {
                                         if(weakReference.get() == nullptr)
                                         {
                                             return;
                                         }
                                         if(result == 0)
                                         {
                                             mDocumentFileBased->addChangeListener(this);
                                             if(previousFile.existsAsFile())
                                             {
                                                 anlDebug("Application", "Reopening last document...");
                                                 openDocumentFile(previousFile);
                                             }
                                         }
                                         else
                                         {
                                             anlDebug("Application", "Restoring backup document...");
                                             auto const loadResult = mDocumentFileBased->loadBackup(backupFile);
                                             anlWeakAssert(!loadResult.failed());
                                             if(!loadResult.failed())
                                             {
                                                 mApplicationAccessor->setAttr<AttrType::currentDocumentFile>(backupFile, NotificationType::synchronous);
                                             }
                                             mDocumentFileBased->addChangeListener(this);
                                         }
                                         if(mApplicationAccessor->getAttr<AttrType::autoUpdate>())
                                         {
                                             checkForNewVersion(false, false);
                                         }
                                     });
    }
    else
    {
        mDocumentFileBased->addChangeListener(this);
        if(previousFile.existsAsFile())
        {
            anlDebug("Application", "Reopening last document...");
            openDocumentFile(previousFile);
        }
        if(mApplicationAccessor->getAttr<AttrType::autoUpdate>())
        {
            checkForNewVersion(false, false);
        }
    }
    mStartupFilesAreLoaded = true;
}

void Application::Instance::checkPluginsQuarantine()
{
#if JUCE_MAC
    auto& pluginListAcsr = getPluginListAccessor();
    if(pluginListAcsr.getAttr<PluginList::AttrType::quarantineMode>() != PluginList::QuarantineMode::force)
    {
        return;
    }
    auto files = PluginList::findLibrariesInQuarantine(pluginListAcsr);
    if(files.empty())
    {
        return;
    }
    juce::String pluginNames;
    for(auto const& file : files)
    {
        pluginNames += file.getFullPathName() + "\n";
    }
    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::WarningIcon)
                             .withTitle(juce::translate("Some plugins may not be loaded due to macOS quarantine!"))
                             .withMessage(juce::translate("Partiels can attemp to remove the plugins from quarantine. Would you like to proceed or ignore the plugins in quarantine?\n PLUGINLLIST").replace("PLUGINLLIST", pluginNames))
                             .withButton(juce::translate("Proceed"))
                             .withButton(juce::translate("Ignore"));
    mIsPluginListReady = false;
    juce::WeakReference<Instance> weakReference(this);
    juce::AlertWindow::showAsync(options, [this, weakReference, files](int result)
                                 {
                                     if(weakReference.get() == nullptr)
                                     {
                                         return;
                                     }
                                     if(result != 0 && PluginList::removeLibrariesFromQuarantine(files))
                                     {
                                         Instance::get().getPluginListAccessor().sendSignal(PluginList::SignalType::rescan, {}, NotificationType::synchronous);
                                     }
                                     mIsPluginListReady = true;
                                 });
#endif
}

Misc::Version Application::Instance::parseVersion(juce::String const& content)
{
    nlohmann::json json;
    try
    {
        json = nlohmann::json::parse(content.toStdString());
    }
    catch(...)
    {
        return {};
    }
    if(!json.is_object())
    {
        return {};
    }
    auto const& releases = json.find("releases");
    if(releases == json.cend() || !releases->is_array())
    {
        return {};
    }

    Misc::Version version;
    for(auto index = 0_z; index < releases->size(); ++index)
    {
        auto const& release = releases->at(index);

        auto const& is_tagged = release.find("is_tagged");
        MiscWeakAssert(is_tagged != release.cend() && is_tagged->is_boolean());
        auto const latest = is_tagged != release.cend() && is_tagged->is_boolean() && is_tagged->get<bool>();

        auto const& version_number = release.find("version_number");
        MiscWeakAssert(version_number != release.cend());
        auto const version_string = (version_number != release.cend() && version_number->is_string()) ? version_number->get<std::string>() : std::optional<std::string>();

        if(latest && version_string.has_value())
        {
            version = std::max(version, Misc::Version::fromString(juce::String(version_string.value())));
        }
    }
    return version;
}

void Application::Instance::checkForNewVersion(bool useActiveVersionOnly, bool warnIfUpToDate)
{
    MiscWeakAssert(mApplicationAccessor != nullptr);
    if(mApplicationAccessor == nullptr)
    {
        return;
    }

    MiscWeakAssert(mDownloader != nullptr);
    if(mDownloader == nullptr)
    {
        return;
    }

    static auto const isDevelopmentVersion = juce::String(PARTIELS_BUILD_TAG) != juce::String(ProjectInfo::versionString);
    auto constexpr urlAddress = "https://forum.ircam.fr/api/projects/401/?format=json";
    mDownloader->launch({urlAddress}, {}, [=, this](juce::String content)
                        {
                            auto const currentVersion = Version::fromString(ProjectInfo::versionString);
                            auto const checkVersion = Version::fromString(mApplicationAccessor->getAttr<AttrType::lastVersion>());
                            auto const usedVersion = useActiveVersionOnly ? currentVersion : std::max(checkVersion, currentVersion);
                            auto const upstreamVersion = parseVersion(content);
                            if(upstreamVersion > usedVersion || (isDevelopmentVersion && upstreamVersion.tie() >= usedVersion.tie()))
                            {
                                auto options = juce::MessageBoxOptions()
                                                   .withIconType(juce::AlertWindow::AlertIconType::InfoIcon)
                                                   .withTitle(juce::translate("A new version is available!").replace("PRODUCT", ProjectInfo::projectName))
                                                   .withMessage(juce::translate("PRODUCT VERSION has been published on the COMPAGNY website.").replace("PRODUCT", ProjectInfo::projectName).replace("VERSION", upstreamVersion.toString()).replace("COMPAGNY", ProjectInfo::companyName))
                                                   .withButton(juce::translate("Proceed to download page"));
                                if(warnIfUpToDate)
                                {
                                    options = options.withButton(juce::translate("Close the window"));
                                }
                                else
                                {
                                    options = options.withButton(juce::translate("Ignore this version")).withButton(juce::translate("Remind me later"));
                                }

                                juce::AlertWindow::showAsync(options, [=, this](int result)
                                                             {
                                                                 if(result != 0)
                                                                 {
                                                                     mApplicationAccessor->setAttr<AttrType::lastVersion>(upstreamVersion.toString(), NotificationType::synchronous);
                                                                 }
                                                                 if(result == 1)
                                                                 {
                                                                     juce::Timer::callAfterDelay(20, []()
                                                                                                 {
                                                                                                     juce::URL("https://forum.ircam.fr/projects/detail/partiels/").launchInDefaultBrowser();
                                                                                                 });
                                                                 }
                                                             });
                            }
                            else if(warnIfUpToDate)
                            {
                                auto const options = juce::MessageBoxOptions()
                                                         .withIconType(juce::AlertWindow::AlertIconType::InfoIcon)
                                                         .withTitle(juce::translate("The version is up to date!"))
                                                         .withMessage(juce::translate("PRODUCT VERSION is the latest version available on the COMPAGNY website.").replace("PRODUCT", ProjectInfo::projectName).replace("VERSION", currentVersion.toString()).replace("COMPAGNY", ProjectInfo::companyName))
                                                         .withButton(juce::translate("Ok"));

                                juce::AlertWindow::showAsync(options, nullptr);
                            }
                        });
}

void Application::Instance::darkModeSettingChanged()
{
    updateLookAndFeel();
}

void Application::Instance::updateLookAndFeel()
{
    MiscWeakAssert(mLookAndFeel != nullptr);
    if(mLookAndFeel == nullptr)
    {
        return;
    }

    mLookAndFeel->setColourChart(getColourChart());
    juce::LookAndFeel::setDefaultLookAndFeel(mLookAndFeel.get());
    if(mMainMenuModel != nullptr)
    {
        mMainMenuModel->menuItemsChanged();
    }
    if(auto* modalComponentManager = juce::ModalComponentManager::getInstance())
    {
        for(int i = 0; i < modalComponentManager->getNumModalComponents(); ++i)
        {
            if(auto* resizableWindow = dynamic_cast<juce::ResizableWindow*>(modalComponentManager->getModalComponent(i)))
            {
                resizableWindow->setBackgroundColour(mLookAndFeel->findColour(juce::ResizableWindow::ColourIds::backgroundColourId));
            }
        }
    }
}

ANALYSE_FILE_END
