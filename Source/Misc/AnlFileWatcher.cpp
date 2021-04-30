#include "AnlFileWatcher.h"

ANALYSE_FILE_BEGIN

void FileWatcher::setFile(juce::File const& file)
{
    if(mFile == file)
    {
        return;
    }
    
    stopTimer();
    mFile = file;
    mModificationTime = file.getLastModificationTime();
    if(file.existsAsFile())
    {
        startTimer(200);
    }
}

void FileWatcher::timerCallback()
{
    if(!mFile.existsAsFile())
    {
        stopTimer();
        fileHasBeenRemoved();
        return;
    }
    auto const time = mFile.getLastModificationTime();
    if(time != mModificationTime)
    {
        mModificationTime = time;
        fileHasBeenModified();
    }
}

ANALYSE_FILE_END
