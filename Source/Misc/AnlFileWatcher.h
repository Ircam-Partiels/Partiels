#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class FileWatcher
: private juce::Timer
{
public:
    FileWatcher() = default;
    virtual ~FileWatcher() override = default;
    
    void setFile(juce::File const& file);
    
protected:
    virtual void fileHasBeenRemoved() = 0;
    virtual void fileHasBeenModified() = 0;
    
private:
    // juce::Timer
    void timerCallback() override;
    
    juce::File mFile;
    juce::Time mModificationTime;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileWatcher)
};
ANALYSE_FILE_END
