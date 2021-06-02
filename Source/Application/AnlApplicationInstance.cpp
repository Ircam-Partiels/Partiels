#include "AnlApplicationInstance.h"
#include <TranslationsData.h>

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
    anlDebug("Application", "Begin...");
    juce::ignoreUnused(commandLine);
    juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDocumentsDirectory).getChildFile("Ircam").setAsCurrentWorkingDirectory();
    juce::LookAndFeel::setDefaultLookAndFeel(&mLookAndFeel);

    mAudioFormatManager.registerBasicFormats();
    mWindow = std::make_unique<Window>();
    if(mWindow == nullptr)
    {
        anlDebug("Application", "Failed.");
        return;
    }
    mMainMenuModel = std::make_unique<MainMenuModel>(*mWindow.get());
    if(mMainMenuModel == nullptr)
    {
        anlDebug("Application", "Failed.");
        return;
    }
    mAudioSettings = std::make_unique<AudioSettings>();
    if(mAudioSettings == nullptr)
    {
        anlDebug("Application", "Failed.");
        return;
    }
    mAbout = std::make_unique<About>();
    if(mAbout == nullptr)
    {
        anlDebug("Application", "Failed.");
        return;
    }

    mExporter = std::make_unique<Exporter>();
    if(mExporter == nullptr)
    {
        anlDebug("Application", "Failed.");
        return;
    }

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
                mLookAndFeel.setColourChart({acsr.getAttr<AttrType::colourMode>()});
                juce::LookAndFeel::setDefaultLookAndFeel(&mLookAndFeel);
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
                            resizableWindow->setBackgroundColour(mLookAndFeel.findColour(juce::ResizableWindow::ColourIds::backgroundColourId));
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
    auto const path = commandLine.removeCharacters("\"");
    if(juce::File::isAbsolutePath(path) && juce::File(path).existsAsFile())
    {
        mDocumentFileBased.addChangeListener(this);
        anlDebug("Application", "Opening new file...");
        openFile(juce::File(path));
        return;
    }

    auto const backupFile = getBackupFile();
    if(backupFile.existsAsFile())
    {
        if(AlertWindow::showOkCancel(AlertWindow::MessageType::question, "Do you want to restore your document?", "The application unexpectedly quit. Do you want to restore the last saved state of the document before this?"))
        {
            auto const result = mDocumentFileBased.loadBackup(backupFile);
            anlWeakAssert(!result.failed());
            if(!result.failed())
            {
                mApplicationAccessor.setAttr<AttrType::currentDocumentFile>(backupFile, NotificationType::synchronous);
            }
            mDocumentFileBased.addChangeListener(this);
            return;
        }
    }

    mDocumentFileBased.addChangeListener(this);
    anlDebug("Application", "Reopening last document...");
    openFile(mApplicationAccessor.getAttr<AttrType::currentDocumentFile>());
}

void Application::Instance::anotherInstanceStarted(juce::String const& commandLine)
{
    if(mWindow == nullptr)
    {
        anlDebug("Application", "Failed: not initialized.");
        return;
    }
    auto const path = commandLine.removeCharacters("\"");
    if(juce::File::isAbsolutePath(path) && juce::File(path).existsAsFile())
    {
        anlDebug("Application", "Opening new file...");
        openFile(juce::File(path));
    }
}

void Application::Instance::systemRequestedQuit()
{
    anlDebug("Application", "Begin...");
    if(mDocumentFileBased.saveIfNeededAndUserAgrees() != juce::FileBasedDocument::SaveResult::savedOk)
    {
        return;
    }
    mApplicationAccessor.setAttr<AttrType::currentDocumentFile>(mDocumentFileBased.getFile(), NotificationType::synchronous);

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
    anlDebug("Application", "Ready");
    quit();
}

void Application::Instance::shutdown()
{
    mApplicationAccessor.removeListener(mApplicationListener);
    mDocumentFileBased.removeChangeListener(this);
    getBackupFile().deleteFile();
    mExporter.reset();
    mAbout.reset();
    mAudioSettings.reset();
    mMainMenuModel.reset();
    mWindow.reset();
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
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

void Application::Instance::openFile(juce::File const& file)
{
    auto const fileExtension = file.getFileExtension();
    if(file == juce::File{})
    {
        mDocumentAccessor.copyFrom({Document::FileBased::getDefaultContainer()}, NotificationType::synchronous);
        mDocumentFileBased.setFile(file);
    }
    else if(getFileExtension() == fileExtension)
    {
        if(!mDocumentFileBased.loadFrom(file, true))
        {
            return;
        }
    }
    else if(mAudioFormatManager.getWildcardForAllFormats().contains(fileExtension))
    {
        mDocumentAccessor.copyFrom({Document::FileBased::getDefaultContainer()}, NotificationType::synchronous);
        mDocumentAccessor.setAttr<Document::AttrType::file>(file, NotificationType::synchronous);
        mDocumentFileBased.setFile({});
    }
    else
    {
        AlertWindow::showMessage(AlertWindow::MessageType::warning, "File format not supported!", "The format of the file FILENAME is not supported by thee application.", {{"FILENAME", file.getFullPathName()}});
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

PluginList::Accessor& Application::Instance::getPluginListAccessor()
{
    return mPluginListAccessor;
}

PluginList::Scanner& Application::Instance::getPluginListScanner()
{
    return mPluginListScanner;
}

Document::Accessor& Application::Instance::getDocumentAccessor()
{
    return mDocumentAccessor;
}

Document::Director& Application::Instance::getDocumentDirector()
{
    return mDocumentDirector;
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

juce::UndoManager& Application::Instance::getUndoManager()
{
    return mUndoManager;
}

void Application::Instance::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    anlStrongAssert(source == &mDocumentFileBased);
    if(source != &mDocumentFileBased)
    {
        return;
    }
    auto const result = mDocumentFileBased.saveBackup(getBackupFile());
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
