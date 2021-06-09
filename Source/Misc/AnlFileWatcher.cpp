#include "AnlFileWatcher.h"

ANALYSE_FILE_BEGIN

void FileWatcher::clearFilesToWatch()
{
    mFileStates.clear();
    stopTimer();
}

void FileWatcher::addFileToWatch(juce::File const& file)
{
    if(mFileStates.count(file) > 0_z)
    {
        return;
    }
    
    stopTimer();
    mFileStates[file] = std::make_tuple(file.getLastModificationTime(), file.existsAsFile());
    if(std::any_of(mFileStates.cbegin(), mFileStates.cend(), [](auto const fileState)
    {
        return fileState.first.existsAsFile();
    }))
    {
        startTimer(200);
    }
}

void FileWatcher::timerCallback()
{
    if(std::none_of(mFileStates.cbegin(), mFileStates.cend(), [](auto const fileState)
    {
        return fileState.first.existsAsFile();
    }))
    {
        stopTimer();
    }
    for(auto& fileState : mFileStates)
    {
        if(std::get<1>(fileState.second) && !fileState.first.existsAsFile())
        {
            std::get<1>(fileState.second) = false;
            fileHasBeenRemoved(fileState.first);
        }
        else if(fileState.first.existsAsFile())
        {
            auto const time = fileState.first.getLastModificationTime();
            if(!std::get<1>(fileState.second) || std::get<0>(fileState.second) != time)
            {
                std::get<0>(fileState.second) = time;
                std::get<1>(fileState.second) = true;
                fileHasBeenModified(fileState.first);
            }
        }
    }
}

ANALYSE_FILE_END
