#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class FileWatcher
: private juce::Timer
{
public:
    FileWatcher() = default;
    virtual ~FileWatcher() override = default;
    
    void clearFilesToWatch();
    void addFileToWatch(juce::File const& file);
    
protected:
    virtual void fileHasBeenRemoved(juce::File const& file) = 0;
    virtual void fileHasBeenModified(juce::File const& file) = 0;
    
private:
    // juce::Timer
    void timerCallback() override;
    
    std::map<juce::File, std::tuple<juce::Time, bool>> mFileStates;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileWatcher)
};
ANALYSE_FILE_END
