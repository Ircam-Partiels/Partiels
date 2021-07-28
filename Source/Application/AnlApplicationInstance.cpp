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

    mAudioFormatManager.registerBasicFormats();

    mPluginListAccessor = std::make_unique<PluginList::Accessor>();
    AppQuitIfInvalidPointer(mPluginListAccessor);

    mPluginListScanner = std::make_unique<PluginList::Scanner>();
    AppQuitIfInvalidPointer(mPluginListScanner);

    mDocumentAccessor = std::make_unique<Document::Accessor>();
    AppQuitIfInvalidPointer(mDocumentAccessor);

    mDocumentDirector = std::make_unique<Document::Director>(*mDocumentAccessor.get(), mAudioFormatManager, mUndoManager);
    AppQuitIfInvalidPointer(mDocumentDirector);

    mProperties = std::make_unique<Properties>();
    AppQuitIfInvalidPointer(mProperties);

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

    mApplicationListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::recentlyOpenedFilesList:
                break;
            case AttrType::currentDocumentFile:
                break;
            case AttrType::windowState:
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
            case AttrType::showInfoBubble:
            case AttrType::exportOptions:
                break;
        }
    };
    mApplicationAccessor.addListener(mApplicationListener, NotificationType::synchronous);

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
        anlDebug("Application", "Opening new file(s)...");
        openFiles(files);
    }

    auto const backupFile = getBackupFile();
    if(backupFile.existsAsFile())
    {
        if(AlertWindow::showOkCancel(AlertWindow::MessageType::question, "Do you want to restore your document?", "The application unexpectedly quit. Do you want to restore the last saved state of the document before this?"))
        {
            anlDebug("Application", "Restoring saved document...");
            auto const result = mDocumentFileBased->loadBackup(backupFile);
            anlWeakAssert(!result.failed());
            if(!result.failed())
            {
                mApplicationAccessor.setAttr<AttrType::currentDocumentFile>(backupFile, NotificationType::synchronous);
            }
            mDocumentFileBased->addChangeListener(this);
            return;
        }
    }

    mDocumentFileBased->addChangeListener(this);
    anlDebug("Application", "Reopening last document...");
    openFiles({mApplicationAccessor.getAttr<AttrType::currentDocumentFile>()});
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
    mApplicationAccessor.setAttr<AttrType::currentDocumentFile>(mDocumentFileBased->getFile(), NotificationType::synchronous);

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

    if(mDocumentFileBased->saveIfNeededAndUserAgrees() != juce::FileBasedDocument::SaveResult::savedOk)
    {
        return;
    }

    anlDebug("Application", "Ready");
    quit();
}

void Application::Instance::shutdown()
{
    anlDebug("Application", "Begin...");

    mApplicationAccessor.removeListener(mApplicationListener);
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
        else if(!file.getFileExtension().isEmpty() && mAudioFormatManager.getWildcardForAllFormats().contains(file.getFileExtension()))
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
            auto reader = std::unique_ptr<juce::AudioFormatReader>(mAudioFormatManager.createReaderFor(file));
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
    return mApplicationAccessor;
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

juce::UndoManager& Application::Instance::getUndoManager()
{
    return mUndoManager;
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

ANALYSE_FILE_END
