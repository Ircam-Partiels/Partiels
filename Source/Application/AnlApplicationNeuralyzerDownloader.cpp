#include "AnlApplicationNeuralyzerDownloader.h"
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

namespace
{
    // The result of a download attempt and whether a new attempt can succeed
    struct AttemptResult
    {
        juce::Result result{juce::Result::ok()};
        bool canRetry{false};
    };

    // Gets the first byte position of the HTTP header "Content-Range: bytes first-last/total" (or -1)
    juce::int64 getContentRangeStart(juce::StringPairArray const& responseHeaders)
    {
        auto const contentRange = responseHeaders.getValue("Content-Range", {}).trim();
        if(!contentRange.startsWithIgnoreCase("bytes"))
        {
            return -1;
        }
        auto const range = contentRange.fromFirstOccurrenceOf("bytes", false, true).trim();
        if(range.isEmpty() || !juce::CharacterFunctions::isDigit(range[0]))
        {
            return -1;
        }
        return range.upToFirstOccurrenceOf("-", false, false).getLargeIntValue();
    }

    // Downloads the file into the temporary file and resumes the download if the temporary file is not empty
    AttemptResult downloadAttempt(juce::File const& tempFile, juce::URL const& url, std::atomic<bool> const& shouldContinue, std::function<void(juce::int64, juce::int64)> const& progressCallback)
    {
        // The timeout is also used as the maximum duration without receiving data on some platforms
        static auto constexpr connectionTimeoutMs = 20000;
        // A large buffer limits the number of reads (and the copies of the internal buffer of the stream on macOS)
        static auto constexpr bufferSize = 4 * 1024 * 1024;

        auto const urlPath = url.toString(true);
        auto const initialOffset = tempFile.existsAsFile() ? tempFile.getSize() : juce::int64(0);
        auto const cancelResult = AttemptResult{juce::Result::fail(juce::translate("Download from URL \"URLPATH\" cancelled by user.").replace("URLPATH", urlPath)), false};

        int statusCode = 0;
        juce::StringPairArray responseHeaders;
        auto const extraHeaders = initialOffset > 0 ? juce::String("Range: bytes=") + juce::String(initialOffset) + "-" : juce::String();
        auto const options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                 .withConnectionTimeoutMs(connectionTimeoutMs)
                                 .withExtraHeaders(extraHeaders)
                                 .withStatusCode(&statusCode)
                                 .withResponseHeaders(&responseHeaders)
                                 .withProgressCallback([&](int, int)
                                                       {
                                                           return shouldContinue.load();
                                                       });
        auto stream = url.createInputStream(options);
        if(!shouldContinue.load())
        {
            return cancelResult;
        }
        if(stream == nullptr)
        {
            return {juce::Result::fail(juce::translate("Failed to create input stream from URL \"URLPATH\".").replace("URLPATH", urlPath)), true};
        }
        if(statusCode == 416 && initialOffset > 0)
        {
            // The range is not satisfiable (the remote file has probably changed): the download restarts from scratch
            tempFile.deleteFile();
            return {juce::Result::fail(juce::translate("Failed to download from URL \"URLPATH\": HTTP STATUS.").replace("URLPATH", urlPath).replace("STATUS", juce::String(statusCode))), true};
        }
        if(statusCode < 200 || statusCode >= 300)
        {
            auto const canRetry = statusCode == 0 || statusCode == 408 || statusCode == 429 || statusCode >= 500;
            return {juce::Result::fail(juce::translate("Failed to download from URL \"URLPATH\": HTTP STATUS.").replace("URLPATH", urlPath).replace("STATUS", juce::String(statusCode))), canRetry};
        }

        // The download is resumed only if the server returns the expected range, otherwise it restarts from scratch
        auto const isResumed = initialOffset > 0 && statusCode == 206 && getContentRangeStart(responseHeaders) == initialOffset;
        if(statusCode == 206 && !isResumed)
        {
            tempFile.deleteFile();
            return {juce::Result::fail(juce::translate("Failed to download the file from URL \"URLPATH\": incomplete download.").replace("URLPATH", urlPath)), true};
        }
        auto position = isResumed ? initialOffset : juce::int64(0);
        auto const contentLength = stream->getTotalLength();
        auto const totalLength = contentLength >= 0 ? position + contentLength : juce::int64(-1);

        juce::FileOutputStream outputStream(tempFile);
        if(!outputStream.openedOk())
        {
            return {juce::Result::fail(juce::translate("Failed to create output stream for the file \"FILEPATH\".").replace("FILEPATH", tempFile.getFullPathName())), false};
        }
        if(!isResumed && (!outputStream.setPosition(0) || outputStream.truncate().failed()))
        {
            return {juce::Result::fail(juce::translate("Failed to write to the file \"FILEPATH\".").replace("FILEPATH", tempFile.getFullPathName())), false};
        }
        if(progressCallback != nullptr)
        {
            progressCallback(position, totalLength);
        }

        std::vector<char> buffer(static_cast<size_t>(bufferSize));
        while(!stream->isExhausted())
        {
            if(!shouldContinue.load())
            {
                return cancelResult;
            }
            auto const numBytesRead = stream->read(buffer.data(), bufferSize);
            if(numBytesRead < 0)
            {
                return {juce::Result::fail(juce::translate("Failed to download the file from URL \"URLPATH\": read error.").replace("URLPATH", urlPath)), true};
            }
            if(numBytesRead == 0)
            {
                break;
            }
            if(!outputStream.write(buffer.data(), static_cast<size_t>(numBytesRead)))
            {
                return {juce::Result::fail(juce::translate("Failed to write to the file \"FILEPATH\".").replace("FILEPATH", tempFile.getFullPathName())), false};
            }
            position += numBytesRead;
            if(progressCallback != nullptr)
            {
                progressCallback(position, totalLength);
            }
        }
        outputStream.flush();
        if(outputStream.getStatus().failed())
        {
            return {juce::Result::fail(juce::translate("Failed to write to the file \"FILEPATH\".").replace("FILEPATH", tempFile.getFullPathName())), false};
        }
        if(!shouldContinue.load())
        {
            return cancelResult;
        }
        // A network error during the transfer can end the stream prematurely without any error
        if(totalLength > 0 && position != totalLength)
        {
            return {juce::Result::fail(juce::translate("Failed to download the file from URL \"URLPATH\": incomplete download.").replace("URLPATH", urlPath)), true};
        }
        return {};
    }

    juce::Result downloadModel(juce::File const& output, juce::URL const& url, std::atomic<bool> const& shouldContinue, std::function<void(juce::int64, juce::int64)> const& progressCallback)
    {
        // The number of attempts and the delay before the first new attempt (doubled at each new attempt)
        static auto constexpr maxNumAttempts = 6;
        static auto constexpr initialRetryDelayMs = 2000;

        if(!url.isWellFormed())
        {
            return juce::Result::fail(juce::translate("Invalid URL \"URLPATH\"").replace("URLPATH", url.toString(true)));
        }

        if(output.existsAsFile())
        {
            return juce::Result::ok();
        }

        // Download the model into a temporary file, which is moved to the final location only on success, so that an
        // interrupted download never leaves an incomplete file at the final location. The temporary file is kept if
        // the download fails, so that a new download (during the same session) resumes where the previous one stopped.
        // The directory is normally created by the process on the message thread, because several downloads running
        // concurrently can try to create it at the same time (and all but one fail with the error "File exists").
        auto const tempDirectory = Application::Neuralyzer::getDownloadTempDirectory();
        auto const createResult = tempDirectory.createDirectory();
        if(createResult.failed() && !tempDirectory.isDirectory())
        {
            return juce::Result::fail(juce::translate("The output directory 'DIRPATH' cannot be created: ERRSTR").replace("DIRPATH", tempDirectory.getFullPathName()).replace("ERRSTR", createResult.getErrorMessage()));
        }
        auto const tempFile = tempDirectory.getChildFile(juce::String::toHexString(static_cast<juce::int64>(output.getFullPathName().hashCode64())) + "-" + output.getFileName());

        AttemptResult attemptResult;
        auto retryDelayMs = initialRetryDelayMs;
        for(auto attempt = 0; attempt < maxNumAttempts; ++attempt)
        {
            if(attempt > 0)
            {
                // Wait before the new attempt (the network can be temporarily unavailable)
                auto const endTime = juce::Time::getMillisecondCounter() + static_cast<juce::uint32>(retryDelayMs);
                while(shouldContinue.load() && juce::Time::getMillisecondCounter() < endTime)
                {
                    juce::Thread::sleep(100);
                }
                retryDelayMs *= 2;
            }
            if(!shouldContinue.load())
            {
                break;
            }
            attemptResult = downloadAttempt(tempFile, url, shouldContinue, progressCallback);
            if(attemptResult.result.wasOk() || !attemptResult.canRetry)
            {
                break;
            }
        }

        if(!shouldContinue.load())
        {
            tempFile.deleteFile();
            return juce::Result::fail(juce::translate("Download from URL \"URLPATH\" cancelled by user.").replace("URLPATH", url.toString(true)));
        }
        if(attemptResult.result.failed())
        {
            return attemptResult.result;
        }
        if(!tempFile.moveFileTo(output))
        {
            return juce::Result::fail(juce::translate("Failed to move the file \"FILEPATH\" to its final location.").replace("FILEPATH", output.getFullPathName()));
        }
        return juce::Result::ok();
    }
} // namespace

Application::Neuralyzer::Downloader::Process::Process(juce::File destinationFile, juce::URL sourceUrl, std::function<void(Process const&)> callback)
: mCallback(std::move(callback))
{
    // The directories are created on the message thread to prevent concurrent creations by the download threads
    for(auto const& directory : {destinationFile.getParentDirectory(), Application::Neuralyzer::getDownloadTempDirectory()})
    {
        auto const createResult = directory.createDirectory();
        if(createResult.failed() && !directory.isDirectory())
        {
            mResult = juce::Result::fail(juce::translate("The output directory 'DIRPATH' cannot be created: ERRSTR").replace("DIRPATH", directory.getFullPathName()).replace("ERRSTR", createResult.getErrorMessage()));
            triggerAsyncUpdate();
            return;
        }
    }

    mOutputFile = destinationFile;
    mIsRunning.store(true);
    mAsyncProcess = std::async([this, destinationFile, sourceUrl]()
                               {
                                   juce::Thread::setCurrentThreadName("Application::Neuralyzer::Downloader");
                                   auto result = downloadModel(destinationFile, sourceUrl, mShouldContinue, [this](juce::int64 position, juce::int64 total)
                                                               {
                                                                   mProcessProgress.store(total > 0 ? static_cast<float>(static_cast<double>(position) / static_cast<double>(total)) : 0.0f);
                                                               });
                                   triggerAsyncUpdate();
                                   return result;
                               });
}

Application::Neuralyzer::Downloader::Process::~Process()
{
    cancel();
    if(mAsyncProcess.valid())
    {
        mAsyncProcess.wait();
        mResult = mAsyncProcess.get();
    }
    mIsRunning.store(false);
    cancelPendingUpdate();
}

juce::File Application::Neuralyzer::Downloader::Process::getOutputFile() const
{
    return mOutputFile;
}

double Application::Neuralyzer::Downloader::Process::getProgress() const
{
    return static_cast<double>(mProcessProgress.load());
}

bool Application::Neuralyzer::Downloader::Process::isRunning() const
{
    return mIsRunning.load();
}

juce::Result Application::Neuralyzer::Downloader::Process::getResult() const
{
    return mResult;
}

void Application::Neuralyzer::Downloader::Process::cancel()
{
    mShouldContinue.store(false);
}

bool Application::Neuralyzer::Downloader::Process::wasCancelled() const
{
    return !mShouldContinue.load();
}

void Application::Neuralyzer::Downloader::Process::handleAsyncUpdate()
{
    if(mAsyncProcess.valid())
    {
        mAsyncProcess.wait();
        mResult = mAsyncProcess.get();
    }
    mIsRunning.store(false);
    sendChangeMessage();
    if(mCallback != nullptr)
    {
        mCallback(*this);
    }
}

Application::Neuralyzer::Downloader::Manager::Manager()
{
    // Remove the temporary files of the downloads that did not complete in a previous session
    Application::Neuralyzer::getDownloadTempDirectory().deleteRecursively();
}

Application::Neuralyzer::Downloader::Manager::~Manager()
{
    for(auto const& process : mProcesses)
    {
        process->removeChangeListener(this);
    }
}

bool Application::Neuralyzer::Downloader::Manager::start(juce::File file, juce::URL url, std::function<void(Process const&)> callback)
{
    if(isDownloading(file))
    {
        return false;
    }

    auto process = std::make_unique<Process>(std::move(file), std::move(url), std::move(callback));
    auto const started = process->isRunning();
    process->addChangeListener(this);
    mProcesses.push_back(std::move(process));
    sendChangeMessage();
    return started;
}

std::vector<std::reference_wrapper<Application::Neuralyzer::Downloader::Process>> Application::Neuralyzer::Downloader::Manager::getProcesses()
{
    std::vector<std::reference_wrapper<Process>> processes;
    processes.reserve(mProcesses.size());
    for(auto const& process : mProcesses)
    {
        processes.push_back(std::ref(*process.get()));
    }
    return processes;
}

bool Application::Neuralyzer::Downloader::Manager::isDownloading(juce::File const& output) const
{
    return std::any_of(mProcesses.cbegin(), mProcesses.cend(), [&](std::unique_ptr<Process> const& process)
                       {
                           return process->isRunning() && process->getOutputFile() == output;
                       });
}

void Application::Neuralyzer::Downloader::Manager::changeListenerCallback([[maybe_unused]] juce::ChangeBroadcaster* source)
{
    mProcesses.erase(std::remove_if(mProcesses.begin(), mProcesses.end(), [](auto const& entry)
                                    {
                                        return !entry->isRunning();
                                    }),
                     mProcesses.end());
    sendChangeMessage();
}

bool Application::Neuralyzer::Downloader::Manager::startIfNecessary(juce::File const& file, juce::URL const& url, std::function<void(Process const&)> callback)
{
    if(file.existsAsFile())
    {
        // Validate the size of the existing file against the size of its remote counterpart
        std::optional<juce::int64> remoteFileSize;
        if(url.isWellFormed())
        {
            static auto constexpr connectionTimeoutMs = 3000;
            juce::WebInputStream stream{url, false};
            stream.withCustomRequestCommand("HEAD").withConnectionTimeout(connectionTimeoutMs);
            if(stream.connect(nullptr) && stream.getStatusCode() >= 200 && stream.getStatusCode() < 300)
            {
                auto const length = stream.getTotalLength();
                if(length >= 0)
                {
                    remoteFileSize = length;
                }
            }
        }
        if(!remoteFileSize.has_value())
        {
            // The size of the remote file cannot be retrieved: keep the existing file rather than downloading it again
            return false;
        }
        if(file.getSize() == remoteFileSize.value())
        {
            return false;
        }

        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::AlertIconType::WarningIcon)
                                 .withTitle(juce::translate("Invalid Model File"))
                                 .withMessage(juce::translate("The file \"FLNAME\" is incomplete or invalid. It will be downloaded again.").replace("FLNAME", file.getFileName()))
                                 .withButton(juce::translate("Ok"), 0);
        juce::AlertWindow::showAsync(options, nullptr);
        file.deleteFile();
    }
    return start(file, url, std::move(callback));
}

Application::Neuralyzer::Downloader::View::View(Process& process)
: mProcess(process)
, mCancelButton(ScalableImage::fromSvg(AnlIconsData::Cancel_svg, AnlIconsData::Cancel_svgSize))
{
    addAndMakeVisible(mSeparator);
    addAndMakeVisible(mNameLabel);
    mNameLabel.setText(mProcess.getOutputFile().getFileNameWithoutExtension(), juce::dontSendNotification);
    addAndMakeVisible(mProgressBar);
    mProgressBar.setStyle(std::make_optional(juce::ProgressBar::Style::linear));
    mProgressBar.setPercentageDisplay(true);
    addAndMakeVisible(mCancelButton);
    mCancelButton.onClick = [this]()
    {
        mProcess.cancel();
        mCancelButton.setEnabled(false);
    };
    startTimer(20);
    setSize(400, 37);
}

Application::Neuralyzer::Downloader::View::~View()
{
    stopTimer();
}

Application::Neuralyzer::Downloader::Process& Application::Neuralyzer::Downloader::View::getProcess() const
{
    return mProcess;
}

void Application::Neuralyzer::Downloader::View::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    mSeparator.setBounds(bounds.removeFromTop(1));
    auto nameBounds = bounds.removeFromTop(24);
    mCancelButton.setBounds(nameBounds.removeFromRight(24).reduced(4));
    mNameLabel.setBounds(nameBounds);
    mProgressBar.setBounds(bounds.removeFromTop(12).reduced(4, 0));
    setSize(bounds.getWidth(), bounds.getY() + 1);
}

void Application::Neuralyzer::Downloader::View::timerCallback()
{
    mEffectiveProgress = mProcess.getProgress();
    mCancelButton.setEnabled(mProcess.isRunning());
    if(!mProcess.isRunning())
    {
        stopTimer();
    }
}

ANALYSE_FILE_END
