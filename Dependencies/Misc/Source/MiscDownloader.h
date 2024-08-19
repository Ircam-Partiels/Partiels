#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

class Downloader
: private juce::URL::DownloadTaskListener
, private juce::AsyncUpdater
{
public:
    Downloader() = default;
    ~Downloader() override;

    void launch(juce::URL url, juce::String const& header, std::function<void(juce::File file)> callback);

private:
    // juce::URL::DownloadTaskListener
    void finished(juce::URL::DownloadTask* task, bool success) override;

    // juce::AsyncUpdater
    void handleAsyncUpdate() override;

    std::unique_ptr<juce::URL::DownloadTask> mTask;
    std::function<void(juce::File)> mCallback;
};

MISC_FILE_END
