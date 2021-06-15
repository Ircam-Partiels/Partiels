#include "AnlFileWatcher.h"

ANALYSE_FILE_BEGIN

FileWatcher::FileWatcher()
{
    mTimerClock.callback = [this]()
    {
        for(auto& fileState : mFileStates)
        {
            if(std::get<1>(fileState.second) && !fileState.first.existsAsFile())
            {
                std::get<1>(fileState.second) = false;
                if(!fileHasBeenRemoved(fileState.first))
                {
                    return;
                }
            }
            else if(!std::get<1>(fileState.second) && fileState.first.existsAsFile())
            {
                std::get<0>(fileState.second) = fileState.first.getLastModificationTime();
                std::get<1>(fileState.second) = true;
                if(!fileHasBeenRestored(fileState.first))
                {
                    return;
                }
            }
            else if(fileState.first.existsAsFile())
            {
                auto const time = fileState.first.getLastModificationTime();
                if(std::get<0>(fileState.second) != time)
                {
                    std::get<0>(fileState.second) = time;
                    std::get<1>(fileState.second) = true;
                    if(!fileHasBeenModified(fileState.first))
                    {
                        return;
                    }
                }
            }
        }
    };
}

void FileWatcher::clearFilesToWatch()
{
    mFileStates.clear();
    mTimerClock.stopTimer();
}

void FileWatcher::addFileToWatch(juce::File const& file)
{
    if(mFileStates.count(file) > 0_z)
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

ANALYSE_FILE_END
