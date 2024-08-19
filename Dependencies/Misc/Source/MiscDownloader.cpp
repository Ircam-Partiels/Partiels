#include "MiscDownloader.h"

MISC_FILE_BEGIN

Downloader::~Downloader()
{
    cancelPendingUpdate();
}

void Downloader::launch(juce::URL url, juce::String const& header, std::function<void(juce::File file)> callback)
{
    cancelPendingUpdate();

    auto const directory = juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory);
    if(!directory.createDirectory())
    {
        MiscWeakAssert(false);
        return;
    }
    auto const file = directory.getChildFile("temp");
    if(!file.deleteFile())
    {
        MiscWeakAssert(false);
        return;
    }

    mTask = url.downloadToFile(file, juce::URL::DownloadTaskOptions().withExtraHeaders(header).withListener(this));
    MiscWeakAssert(mTask != nullptr);
    mCallback = callback;
}

void Downloader::finished(juce::URL::DownloadTask* task, bool success)
{
    MiscWeakAssert(mTask.get() == task);
    if(mTask.get() != task)
    {
        return;
    }
    if(success)
    {
        triggerAsyncUpdate();
    }
}

void Downloader::handleAsyncUpdate()
{
    auto task = std::move(mTask);
    MiscWeakAssert(task->isFinished());
    if(!task->isFinished())
    {
        return;
    }
    auto const file = task->getTargetLocation();
    MiscWeakAssert(file.existsAsFile());
    if(!file.existsAsFile())
    {
        return;
    }
    if(mCallback != nullptr)
    {
        mCallback(file);
    }
}

MISC_FILE_END
