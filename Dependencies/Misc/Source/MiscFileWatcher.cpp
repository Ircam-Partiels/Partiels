#include "MiscFileWatcher.h"

MISC_FILE_BEGIN

FileWatcher::FileWatcher()
{
    mTimerClock.callback = [this]()
    {
        std::vector<juce::File> removedFiles;
        std::vector<juce::File> restoredFiles;
        std::vector<juce::File> modifiedFiles;
        for(auto& fileState : mFileStates)
        {
            if(std::get<1>(fileState.second) && !fileState.first.existsAsFile())
            {
                std::get<1>(fileState.second) = false;
                removedFiles.push_back(fileState.first);
            }
            else if(!std::get<1>(fileState.second) && fileState.first.existsAsFile())
            {
                std::get<0>(fileState.second) = fileState.first.getLastModificationTime();
                std::get<1>(fileState.second) = true;
                restoredFiles.push_back(fileState.first);
            }
            else if(fileState.first.existsAsFile())
            {
                auto const time = fileState.first.getLastModificationTime();
                if(std::get<0>(fileState.second) != time)
                {
                    std::get<0>(fileState.second) = time;
                    std::get<1>(fileState.second) = true;
                    modifiedFiles.push_back(fileState.first);
                }
            }
        }

        for(auto const& removedFile : removedFiles)
        {
            fileHasBeenRemoved(removedFile);
            if(onFileHasBeenRemoved != nullptr)
            {
                onFileHasBeenRemoved(removedFile);
            }
        }
        for(auto const& restoredFile : restoredFiles)
        {
            fileHasBeenRestored(restoredFile);
            if(onFileHasBeenRestored != nullptr)
            {
                onFileHasBeenRestored(restoredFile);
            }
        }
        for(auto const& modifiedFile : modifiedFiles)
        {
            fileHasBeenModified(modifiedFile);
            if(onFileHasBeenModified != nullptr)
            {
                onFileHasBeenModified(modifiedFile);
            }
        }
    };
}

void FileWatcher::addFile(juce::File const& file)
{
    if(file == juce::File{} || mFileStates.count(file) > 0_z)
    {
        return;
    }

    mTimerClock.stopTimer();
    mFileStates[file] = std::make_tuple(file.getLastModificationTime(), file.existsAsFile());
    if(!mFileStates.empty())
    {
        mTimerClock.startTimer(200);
    }
}

void FileWatcher::removeFile(juce::File const& file)
{
    if(file == juce::File{} || mFileStates.count(file) == 0_z)
    {
        return;
    }
    mTimerClock.stopTimer();
    mFileStates.erase(file);
    if(!mFileStates.empty())
    {
        mTimerClock.startTimer(200);
    }
}

void FileWatcher::clearAllFiles()
{
    mTimerClock.stopTimer();
    mFileStates.clear();
}

void FileWatcher::fileHasBeenRemoved(juce::File const& file)
{
    juce::ignoreUnused(file);
}

void FileWatcher::fileHasBeenRestored(juce::File const& file)
{
    juce::ignoreUnused(file);
}

void FileWatcher::fileHasBeenModified(juce::File const& file)
{
    juce::ignoreUnused(file);
}

MISC_FILE_END
