#pragma once

#include "MiscBasics.h"
#include "MiscTimerClock.h"

MISC_FILE_BEGIN

class FileWatcher
{
public:
    FileWatcher();
    virtual ~FileWatcher() = default;

    void addFile(juce::File const& file);
    void removeFile(juce::File const& file);
    void clearAllFiles();

    std::function<void(juce::File const&)> onFileHasBeenRemoved = nullptr;
    std::function<void(juce::File const&)> onFileHasBeenRestored = nullptr;
    std::function<void(juce::File const&)> onFileHasBeenModified = nullptr;

protected:
    virtual void fileHasBeenRemoved(juce::File const& file);
    virtual void fileHasBeenRestored(juce::File const& file);
    virtual void fileHasBeenModified(juce::File const& file);

private:
    TimerClock mTimerClock;
    std::map<juce::File, std::tuple<juce::Time, bool>> mFileStates;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileWatcher)
};

MISC_FILE_END
