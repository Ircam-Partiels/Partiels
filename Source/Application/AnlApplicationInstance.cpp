#include "AnlApplicationInstance.h"
#include "AnlApplicationCommandLine.h"
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

    auto const cmdResult = CommandLine::tryToRun(commandLine);
    if(cmdResult.has_value())
    {
        setApplicationReturnValue(*cmdResult);
        quit();
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

    mDocumentFileBased = std::make_unique<Document::FileBased>(*mDocumentDirector.get(), getFileExtension(), getFileWildCard(), "Open a document", "Save the document");
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

    auto const paths = juce::StringArray::fromTokens(commandLine, " ", "\"");
    std::vector<juce::File> files;
    for(auto const& path : paths)
    {
        if(juce::File::isAbsolutePath(path) && juce::File(path).existsAsFile())
        {
            files.push_back(juce::File(path));
        }
    }
    if(!files.empty())
    {
        mDocumentFileBased->addChangeListener(this);
        anlDebug("Application", "Opening new file(s)...");
        openFiles(files);
        return;
    }

    auto const backupFile = getBackupFile();
    auto const currentFile = mApplicationAccessor->getAttr<AttrType::currentDocumentFile>();
    if(backupFile.existsAsFile())
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::QuestionIcon)
                                 .withTitle(juce::translate("Do you want to restore your document?"))
                                 .withMessage(juce::translate("The application unexpectedly quit. Do you want to restore the last saved state of the document before this?"))
                                 .withButton(juce::translate("Restore"))
                                 .withButton(juce::translate("Ignore"));

        juce::AlertWindow::showAsync(options, [=](int result)
                                     {
                                         if(result == 0)
                                         {
                                             mDocumentFileBased->addChangeListener(this);
                                             anlDebug("Application", "Reopening last document...");
                                             openFiles({currentFile});
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
        anlDebug("Application", "Reopening last document...");
        openFiles({currentFile});
    }
}

void Application::Instance::anotherInstanceStarted(juce::String const& commandLine)
{
    if(mWindow == nullptr)
    {
        anlDebug("Application", "Failed: not initialized.");
        return;
    }
    auto const paths = juce::StringArray::fromTokens(commandLine, " ", "\"");
    std::vector<juce::File> files;
    for(auto const& path : paths)
    {
        if(juce::File::isAbsolutePath(path) && juce::File(path).existsAsFile())
        {
            anlDebug("Application", "Opening new file(s)...");
            files.push_back(juce::File(path));
        }
    }
    if(!files.empty())
    {
        openFiles(files);
    }
}

void Application::Instance::systemRequestedQuit()
{
    anlDebug("Application", "Begin...");
    if(mApplicationAccessor != nullptr)
    {
        mApplicationAccessor->setAttr<AttrType::currentDocumentFile>(mDocumentFileBased->getFile(), NotificationType::synchronous);
    }

    if(auto* modalComponentManager = juce::ModalComponentManager::getInstance())
    {
        if(modalComponentManager->cancelAllModalComponents())
        {
            anlDebug("Application", "Delayed...");
            juce::Timer::callAfterDelay(500, [this]()
                                        {
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
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    mLookAndFeel.reset();
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

    anlDebug("Application", "Done");
}

Application::Instance& Application::Instance::get()
{
    return *static_cast<Instance*>(JUCEApplication::getInstance());
}

juce::String Application::Instance::getFileExtension()
{
    return App::getFileExtensionFor("doc");
}

juce::String Application::Instance::getFileWildCard()
{
    return App::getFileWildCardFor("doc");
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

void Application::Instance::openFiles(std::vector<juce::File> const& files)
{
    std::vector<juce::File> audioFiles;
    juce::StringArray array;
    for(auto const& file : files)
    {
        if(file.hasFileExtension(getFileExtension()))
        {
            mDocumentFileBased->loadFrom(file, true);
            return;
        }
        if(file == juce::File{})
        {
        }
        else if(!file.getFileExtension().isEmpty() && mAudioFormatManager->getWildcardForAllFormats().contains(file.getFileExtension()))
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
        mDocumentAccessor->copyFrom(mDocumentFileBased->getDefaultAccessor(), NotificationType::synchronous);
        std::vector<AudioFileLayout> readerLayout;
        for(auto const& file : files)
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
        mDocumentAccessor->setAttr<Document::AttrType::reader>(readerLayout, NotificationType::synchronous);
        mDocumentFileBased->setFile({});
    }
    else if(array.isEmpty())
    {
        mDocumentAccessor->copyFrom(mDocumentFileBased->getDefaultAccessor(), NotificationType::synchronous);
        mDocumentFileBased->setFile({});
    }
    else
    {
        AlertWindow::showMessage(AlertWindow::MessageType::warning, "File format not supported!", "The format(s) of the file(s) FILENAMES is not supported by the application.", {{"FILENAMES", array.joinIntoString(",")}});
    }
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
    return juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory).getChildFile("backup").withFileExtension(getFileExtension());
}

juce::ApplicationCommandManager* App::getApplicationCommandManager()
{
    return &Application::Instance::get().getApplicationCommandManager();
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
        juce::AlertWindow::showAsync(options, [this, files](int result)
                                     {
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
