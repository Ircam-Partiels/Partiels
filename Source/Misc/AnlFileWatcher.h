#pragma once

#include "AnlBasics.h"
#include "AnlTimerClock.h"

ANALYSE_FILE_BEGIN

class FileWatcher
{
public:
    FileWatcher();
    virtual ~FileWatcher() = default;

    void clearFilesToWatch();
    void addFileToWatch(juce::File const& file);

protected:
    virtual bool fileHasBeenRemoved(juce::File const& file) = 0;
    virtual bool fileHasBeenRestored(juce::File const& file) = 0;
    virtual bool fileHasBeenModified(juce::File const& file) = 0;

private:
    TimerClock mTimerClock;
    std::map<juce::File, std::tuple<juce::Time, bool>> mFileStates;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileWatcher)
};
ANALYSE_FILE_END
