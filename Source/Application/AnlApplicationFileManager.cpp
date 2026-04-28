#include "AnlApplicationFileManager.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationNeuralyzerModel.h"

ANALYSE_FILE_BEGIN

void Application::FileManager::newDocument()
{
    auto& fileBased = Instance::get().getDocumentFileBased();
    if(Instance::get().getDocumentAccessor().isEquivalentTo(fileBased.getDefaultAccessor()))
    {
        if(fileBased.onSaved != nullptr)
        {
            fileBased.onSaved(fileBased.getFile());
        }
        if(fileBased.onLoaded != nullptr)
        {
            fileBased.onLoaded({});
        }
        return;
    }

    juce::WeakReference<Instance> weakReference(&Instance::get());
    fileBased.saveIfNeededAndUserAgreesAsync([=](juce::FileBasedDocument::SaveResult saveResult)
                                             {
                                                 if(weakReference.get() == nullptr)
                                                 {
                                                     return;
                                                 }
                                                 if(saveResult != juce::FileBasedDocument::SaveResult::savedOk)
                                                 {
                                                     return;
                                                 }
                                                 auto const& defaultAcsr = Instance::get().getDocumentFileBased().getDefaultAccessor();
                                                 Instance::get().getDocumentAccessor().copyFrom(defaultAcsr, NotificationType::synchronous);
                                                 Instance::get().getDocumentFileBased().setFile({});
                                                 if(Instance::get().getDocumentFileBased().onLoaded != nullptr)
                                                 {
                                                     Instance::get().getDocumentFileBased().onLoaded({});
                                                 }
                                                 Instance::get().getUndoManager().clearUndoHistory();
                                                 if(auto* window = Instance::get().getWindow())
                                                 {
                                                     window->getInterface().hidePluginListTablePanel();
                                                 }
                                             });
}

void Application::FileManager::openDocumentFile(juce::File const& newFile)
{
    auto& fileBased = Instance::get().getDocumentFileBased();
    auto const documentFileExtension = Instance::getExtensionForDocumentFile();
    MiscWeakAssert(newFile.existsAsFile() && newFile.hasFileExtension(documentFileExtension));
    if(!newFile.existsAsFile() || !newFile.hasFileExtension(documentFileExtension) || newFile == fileBased.getFile())
    {
        return;
    }

    juce::WeakReference<Instance> weakReference(&Instance::get());
    fileBased.saveIfNeededAndUserAgreesAsync([=](juce::FileBasedDocument::SaveResult saveResult)
                                             {
                                                 if(weakReference.get() == nullptr)
                                                 {
                                                     return;
                                                 }
                                                 if(saveResult != juce::FileBasedDocument::SaveResult::savedOk)
                                                 {
                                                     return;
                                                 }
                                                 Instance::get().getDocumentFileBased().loadFrom(newFile, true);
                                                 Instance::get().getUndoManager().clearUndoHistory();
                                             });
}

void Application::FileManager::openAudioFiles(std::vector<juce::File> const& files)
{
    auto& fileBased = Instance::get().getDocumentFileBased();
    auto& audioFormatManager = Instance::get().getAudioFormatManager();
    auto const audioFileWilcards = Instance::getWildCardForAudioFormats();
    std::vector<AudioFileLayout> readerLayout;
    for(auto const& file : files)
    {
        auto const fileExtension = file.getFileExtension().toLowerCase();
        MiscWeakAssert(file.existsAsFile() && file.hasReadAccess() && audioFileWilcards.contains(fileExtension));
        if(file.existsAsFile() && file.hasReadAccess() && audioFileWilcards.contains(fileExtension))
        {
            auto reader = std::unique_ptr<juce::AudioFormatReader>(audioFormatManager.createReaderFor(file));
            MiscWeakAssert(reader != nullptr);
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
        juce::WeakReference<Instance> weakReference(&Instance::get());
        fileBased.saveIfNeededAndUserAgreesAsync([=, rl = std::move(readerLayout)](juce::FileBasedDocument::SaveResult saveResult)
                                                 {
                                                     if(weakReference.get() == nullptr)
                                                     {
                                                         return;
                                                     }
                                                     if(saveResult != juce::FileBasedDocument::SaveResult::savedOk)
                                                     {
                                                         return;
                                                     }
                                                     if(!Instance::get().getDocumentFileBased().hasChangedSinceSaved())
                                                     {
                                                     }
                                                     auto const& defaultAcsr = Instance::get().getDocumentFileBased().getDefaultAccessor();
                                                     Instance::get().getDocumentAccessor().copyFrom(defaultAcsr, NotificationType::synchronous);
                                                     Instance::get().getDocumentAccessor().setAttr<Document::AttrType::reader>(rl, NotificationType::synchronous);
                                                     auto const templateFile = Instance::get().getApplicationAccessor().getAttr<AttrType::defaultTemplateFile>();
                                                     auto const adaptationToSampleRate = Instance::get().getApplicationAccessor().getAttr<AttrType::adaptationToSampleRate>();
                                                     if(templateFile != juce::File{})
                                                     {
                                                         Instance::get().getDocumentFileBased().loadTemplate(templateFile, adaptationToSampleRate);
                                                     }
                                                     Instance::get().getUndoManager().clearUndoHistory();
                                                     Instance::get().getDocumentFileBased().setFile({});
                                                     if(Instance::get().getDocumentFileBased().onLoaded != nullptr)
                                                     {
                                                         Instance::get().getDocumentFileBased().onLoaded({});
                                                     }
                                                 });
    }
    juce::StringArray remainingFiles;
    for(auto const& file : files)
    {
        if(std::none_of(readerLayout.cbegin(), readerLayout.cend(), [&](auto const& afl)
                        {
                            return afl.file == file;
                        }))
        {
            remainingFiles.add(file.getFullPathName());
        }
    }
    if(!remainingFiles.isEmpty())
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::WarningIcon)
                                 .withTitle(juce::translate("File(s) cannot be open!"))
                                 .withMessage(juce::translate("The file(s) cannot be open by the application:\n FILEPATHS\nEnsure the file(s) exist(s) and you have read access.").replace("FILEPATHS", remainingFiles.joinIntoString("\n")))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
    }
}

void Application::FileManager::openFiles(std::vector<juce::File> const& files)
{
    auto const documentFileExtension = Instance::getExtensionForDocumentFile();
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
    auto const audioFormatsWildCard = Instance::getWildCardForAudioFormats();
    for(auto const& file : files)
    {
        auto const fileExtension = file.getFileExtension().toLowerCase();
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

void Application::FileManager::saveDocument()
{
    juce::WeakReference<Instance> weakReference(&Instance::get());
    Instance::get().getDocumentFileBased().saveAsync(true, true, [=](juce::FileBasedDocument::SaveResult saveResult)
                                                     {
                                                         if(weakReference.get() == nullptr)
                                                         {
                                                             return;
                                                         }
                                                         if(saveResult != juce::FileBasedDocument::SaveResult::savedOk)
                                                         {
                                                             return;
                                                         }
                                                     });
}

void Application::FileManager::saveDocumentAs()
{
    Instance::get().getDocumentFileBased().saveAsInteractiveAsync(true, nullptr);
}

void Application::FileManager::consolidateDocument()
{
    juce::WeakReference<Instance> weakReference(&Instance::get());
    Instance::get().getDocumentFileBased().saveAsync(true, true, [=](juce::FileBasedDocument::SaveResult saveResult)
                                                     {
                                                         if(weakReference.get() == nullptr)
                                                         {
                                                             return;
                                                         }
                                                         if(saveResult != juce::FileBasedDocument::SaveResult::savedOk)
                                                         {
                                                             return;
                                                         }
                                                         auto const result = Instance::get().getDocumentFileBased().consolidate();
                                                         auto const fileName = Instance::get().getDocumentFileBased().getFile().getFullPathName();
                                                         if(result.wasOk())
                                                         {
                                                             auto const options = juce::MessageBoxOptions()
                                                                                      .withIconType(juce::AlertWindow::InfoIcon)
                                                                                      .withTitle(juce::translate("Document consolidated!"))
                                                                                      .withMessage(juce::translate("The document has been consolidated with the audio files and the analyses to FLNAME.").replace("FLNAME", fileName))
                                                                                      .withButton(juce::translate("Ok"));
                                                             juce::AlertWindow::showAsync(options, nullptr);
                                                         }
                                                         else
                                                         {
                                                             auto const options = juce::MessageBoxOptions()
                                                                                      .withIconType(juce::AlertWindow::WarningIcon)
                                                                                      .withTitle(juce::translate("Document consolidation failed!"))
                                                                                      .withMessage(juce::translate("The document cannot be consolidated with the audio files and the analyses to FLNAME: ERROR.").replace("FLNAME", fileName).replace("ERROR", result.getErrorMessage()))
                                                                                      .withButton(juce::translate("Ok"));
                                                             juce::AlertWindow::showAsync(options, nullptr);
                                                         }
                                                     });
}

void Application::FileManager::saveAndQuit()
{
    auto& fileBased = Instance::get().getDocumentFileBased();
    juce::WeakReference<Instance> weakReference(&Instance::get());
    fileBased.saveIfNeededAndUserAgreesAsync([=](juce::FileBasedDocument::SaveResult result)
                                             {
                                                 if(weakReference.get() == nullptr)
                                                 {
                                                     return;
                                                 }
                                                 if(result != juce::FileBasedDocument::SaveResult::savedOk)
                                                 {
                                                     return;
                                                 }
                                                 MiscDebug("Application", "Ready to Quit");
                                                 Instance::get().quit();
                                             });
}

ANALYSE_FILE_END
