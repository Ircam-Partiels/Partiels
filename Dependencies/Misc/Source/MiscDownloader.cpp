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

juce::Result Downloader::download(juce::URL const& url, juce::File const& target, juce::URL::InputStreamOptions const& options)
{
    try
    {
        auto inputStream = url.createInputStream(options);
        MiscWeakAssert(inputStream != nullptr);
        if(inputStream == nullptr)
        {
            MiscDebug("Downloader", "Failed to create input stream from URL \"" + url.toString(true) + "\"");
            return juce::Result::fail(juce::translate("Failed to create input stream from URL \"URLPATH\".").replace("URLPATH", url.toString(true)));
        }

        juce::FileOutputStream outputStream(target);
        if(!outputStream.openedOk())
        {
            MiscDebug("Downloader", "Failed to create output stream for the file \"" + target.getFullPathName() + "\"");
            return juce::Result::fail(juce::translate("Failed to create output stream for the file \"FILEPATH\".").replace("FILEPATH", target.getFullPathName()));
        }

        static auto constexpr bufferSize = 8192;
        std::vector<char> buffer(bufferSize);
        while(!inputStream->isExhausted())
        {
            auto bytesRead = inputStream->read(buffer.data(), static_cast<int>(buffer.size()));
            if(bytesRead < 0)
            {
                MiscDebug("Downloader", "Error while reading from URL \"" + url.toString(true) + "\" (negative bytes read).");
                return juce::Result::fail(juce::translate("Failed to download the file from URL \"URLPATH\": read error.")
                                              .replace("URLPATH", url.toString(true)));
            }
            if(bytesRead == 0)
            {
                if(inputStream->isExhausted())
                {
                    break;
                }
                MiscDebug("Downloader", "Error while reading from URL \"" + url.toString(true) + "\" (premature end of stream).");
                return juce::Result::fail(juce::translate("Failed to download the file from URL \"URLPATH\": premature end of stream.")
                                              .replace("URLPATH", url.toString(true)));
            }
            if(!outputStream.write(buffer.data(), static_cast<size_t>(bytesRead)))
            {
                MiscDebug("Downloader", "Failed to write to the file \"" + target.getFullPathName() + "\"");
                return juce::Result::fail(juce::translate("Failed to write to the file \"FILEPATH\".").replace("FILEPATH", target.getFullPathName()));
            }
        }
        outputStream.flush();
    }
    catch(std::exception const& e)
    {
        MiscDebug("Downloader", "Failed to download the file from URL \"" + url.toString(true) + "\": " + e.what() + ".");
        return juce::Result::fail(juce::translate("Failed to download the file from URL \"URLPATH\": ERROR.").replace("URLPATH", url.toString(true)).replace("ERROR", e.what()));
    }
    return juce::Result::ok();
}

MISC_FILE_END
