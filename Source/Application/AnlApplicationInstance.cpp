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
    return ProjectInfo::versionString;
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

    mProperties = std::make_unique<Properties>();
    AppQuitIfInvalidPointer(mProperties);
    checkPluginsQuarantine();

    mAudioReader = std::make_unique<AudioReader>();
    AppQuitIfInvalidPointer(mAudioReader);

    mLookAndFeel = std::make_unique<LookAndFeel>();
    AppQuitIfInvalidPointer(mLookAndFeel);
    juce::LookAndFeel::setDefaultLookAndFeel(mLookAndFeel.get());

    mDocumentFileBased = std::make_unique<Document::FileBased>(*mDocumentDirector.get(), getExtensionForDocumentFile(), getWildCardForDocumentFile(), "Open a document", "Save the document");
    AppQuitIfInvalidPointer(mDocumentFileBased);

    mWindow = std::make_unique<Window>();
    AppQuitIfInvalidPointer(mWindow);

    mMainMenuModel = std::make_unique<MainMenuModel>(*mWindow.get());
    AppQuitIfInvalidPointer(mMainMenuModel);

    mAudioSettings = std::make_unique<AudioSettings>();
    AppQuitIfInvalidPointer(mAudioSettings);

    mAbout = std::make_unique<About>();
    AppQuitIfInvalidPointer(mAbout);

    mExporter = std::make_unique<Exporter>();
    AppQuitIfInvalidPointer(mExporter);

    mBatcher = std::make_unique<Batcher>();
    AppQuitIfInvalidPointer(mBatcher);

    mTrackLoader = std::make_unique<Track::Loader::ArgumentSelector>();
    AppQuitIfInvalidPointer(mTrackLoader);

    mApplicationListener->onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::recentlyOpenedFilesList:
            case AttrType::currentDocumentFile:
            case AttrType::windowState:
            case AttrType::showInfoBubble:
            case AttrType::exportOptions:
            case AttrType::adaptationToSampleRate:
            case AttrType::autoLoadConvertedFile:
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
            }
            break;
            case AttrType::colourMode:
            {
                mLookAndFeel->setColourChart({acsr.getAttr<AttrType::colourMode>()});
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
            break;
        }
    };
    mApplicationAccessor->addListener(*mApplicationListener.get(), NotificationType::synchronous);

    anlDebug("Application", "Ready!");

    juce::ArgumentList const args("Partiels", commandLine);
    std::vector<juce::File> commandFiles;
    for(int i = 0; i < args.size(); ++i)
    {
        auto const file = args[i].resolveAsFile();
        if(file.existsAsFile())
        {
            commandFiles.push_back(file);
        }
    }
    auto const previousFile = mApplicationAccessor->getAttr<AttrType::currentDocumentFile>();
    juce::WeakReference<Instance> weakReference(this);
    juce::MessageManager::callAsync([=, this]()
                                    {
                                        if(weakReference.get() == nullptr)
                                        {
                                            return;
                                        }
                                        openStartupFiles(commandFiles, previousFile);
                                    });
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
    if(!files.empty())
    {
        anlDebug("Application", "Opening new file(s)...");
        openFiles(files);
    }
}

void Application::Instance::systemRequestedQuit()
{
    anlDebug("Application", "Begin...");
    if(mCommandLine != nullptr && mCommandLine->isRunning())
    {
        anlDebug("Application", "Delayed...");
        juce::WeakReference<Instance> weakReference(this);
        juce::Timer::callAfterDelay(500, [=, this]()
                                    {
                                        if(weakReference.get() == nullptr)
                                        {
                                            return;
                                        }
                                        systemRequestedQuit();
                                    });
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
            anlDebug("Application", "Delayed...");
            juce::WeakReference<Instance> weakReference(this);
            juce::Timer::callAfterDelay(500, [=, this]()
                                        {
                                            if(weakReference.get() == nullptr)
                                            {
                                                return;
                                            }
                                            systemRequestedQuit();
                                        });
            return;
        }
    }
    if(mDocumentFileBased != nullptr)
    {
        mDocumentFileBased->saveIfNeededAndUserAgreesAsync([](juce::FileBasedDocument::SaveResult result)
                                                           {
                                                               if(result == juce::FileBasedDocument::SaveResult::savedOk)
                                                               {
                                                                   anlDebug("Application", "Ready");
                                                                   quit();
                                                               }
                                                           });
    }
    else
    {
        quit();
    }
}

void Application::Instance::shutdown()
{
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
    getBackupFile().deleteFile();

    mBatcher.reset();
    mExporter.reset();
    mAbout.reset();
    mAudioSettings.reset();
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
    mTrackLoader.reset();

    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    mLookAndFeel.reset();

    anlDebug("Application", "Done");
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
    auto* window = get().getWindow();
    if(!identifier.isEmpty() && window != nullptr)
    {
        auto const bounds = juce::Desktop::getInstance().getDisplays().logicalToPhysical(window->getPlotBounds(identifier));
        anlWeakAssert(!bounds.isEmpty());
        return {bounds.getWidth(), bounds.getHeight()};
    }
    return {0, 0};
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
                                                           mDocumentAccessor->copyFrom(mDocumentFileBased->getDefaultAccessor(), NotificationType::synchronous);
                                                           mDocumentFileBased->setFile({});
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
                                                               mDocumentAccessor->copyFrom(mDocumentFileBased->getDefaultAccessor(), NotificationType::synchronous);
                                                               mDocumentAccessor->setAttr<Document::AttrType::reader>(readerLayout, NotificationType::synchronous);
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
    if(mTrackLoader == nullptr || !file.existsAsFile() || fileExtension.isEmpty() || !importFileWildcard.contains(fileExtension))
    {
        return;
    }
    juce::WeakReference<Instance> weakReference(this);
    mTrackLoader->onLoad = [=, this](Track::FileInfo fileInfo)
    {
        if(weakReference.get() == nullptr)
        {
            return;
        }
        Tools::addFileTrack(position, fileInfo);
        mTrackLoader->hide();
    };
    mTrackLoader->setFile(file);
    mTrackLoader->show();
    juce::MessageManager::callAsync([=, this]()
                                    {
                                        if(weakReference.get() == nullptr)
                                        {
                                            return;
                                        }
                                        mTrackLoader->toFront(true);
                                    });
}

Application::Accessor& Application::Instance::getApplicationAccessor()
{
    return *mApplicationAccessor.get();
}

Application::AudioSettings* Application::Instance::getAudioSettings()
{
    return mAudioSettings.get();
}

Application::About* Application::Instance::getAbout()
{
    return mAbout.get();
}

Application::Window* Application::Instance::getWindow()
{
    return mWindow.get();
}

Application::Exporter* Application::Instance::getExporter()
{
    return mExporter.get();
}

Application::Batcher* Application::Instance::getBatcher()
{
    return mBatcher.get();
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
    anlStrongAssert(source == mDocumentFileBased.get());
    if(source != mDocumentFileBased.get())
    {
        return;
    }
    auto const result = mDocumentFileBased->saveBackup(getBackupFile());
    anlWeakAssert(!result.failed());
}

juce::File Application::Instance::getBackupFile() const
{
    return juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory).getChildFile("backup").withFileExtension(getExtensionForDocumentFile());
}

juce::ApplicationCommandManager* App::getApplicationCommandManager()
{
    return &Application::Instance::get().getApplicationCommandManager();
}

void Application::Instance::openStartupFiles(std::vector<juce::File> const commandFiles, juce::File const previousFile)
{
    if(!mIsPluginListReady)
    {
        juce::WeakReference<Instance> weakReference(this);
        juce::MessageManager::callAsync([=, this]()
                                        {
                                            if(weakReference.get() == nullptr)
                                            {
                                                return;
                                            }
                                            openStartupFiles(commandFiles, previousFile);
                                        });
        return;
    }

    if(!commandFiles.empty())
    {
        mDocumentFileBased->addChangeListener(this);
        anlDebug("Application", "Opening new file(s)...");
        openFiles(commandFiles);
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
    }
}

void Application::Instance::checkPluginsQuarantine()
{
#if JUCE_MAC
    auto& pluginListAcsr = getPluginListAccessor();
    if(pluginListAcsr.getAttr<PluginList::AttrType::quarantineMode>() != PluginList::QuarantineMode::force)
    {
        return;
    }
    auto const files = PluginList::findLibrariesInQuarantine(pluginListAcsr);
    if(!files.empty())
    {
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
        juce::AlertWindow::showAsync(options, [=, this](int result)
                                     {
                                         if(weakReference.get() == nullptr)
                                         {
                                             return;
                                         }
                                         if(result == 0)
                                         {
                                             mIsPluginListReady = true;
                                             return;
                                         }
                                         if(PluginList::removeLibrariesFromQuarantine(files))
                                         {
                                             Instance::get().getPluginListAccessor().sendSignal(PluginList::SignalType::rescan, {}, NotificationType::synchronous);
                                         }
                                         mIsPluginListReady = true;
                                     });
    }
#endif
}

ANALYSE_FILE_END
