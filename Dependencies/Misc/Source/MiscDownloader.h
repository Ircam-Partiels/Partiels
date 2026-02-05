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

    // A method for downloading from a URL to a local file using the provided
    // juce::URL::InputStreamOptions (which may include a progress callback that
    // allows the process to be stopped if necessary).
    static juce::Result download(juce::URL const& url, juce::File const& target, juce::URL::InputStreamOptions const& options);

private:
    // juce::URL::DownloadTaskListener
    void finished(juce::URL::DownloadTask* task, bool success) override;

    // juce::AsyncUpdater
    void handleAsyncUpdate() override;

    std::unique_ptr<juce::URL::DownloadTask> mTask;
    std::function<void(juce::File)> mCallback;
};

MISC_FILE_END
