#include "AnlApplicationNeuralyzerDownloader.h"
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

namespace
{
    static juce::Result downloadModel(juce::File output, juce::URL url, std::function<bool(int, int)> callback)
    {
        if(callback != nullptr)
        {
            callback(0, 100);
        }

        if(!url.isWellFormed())
        {
            return juce::Result::fail(juce::translate("Invalid URL \"URLPATH\"").replace("URLPATH", url.toString(true)));
        }

        if(output.existsAsFile())
        {
            if(callback != nullptr)
            {
                callback(100, 100);
            }
            return juce::Result::ok();
        }

        // Download the model into a temporary file, which is moved to the final location only on success, so that an
        // interrupted download never leaves an incomplete file at the final location. The juce::TemporaryFile object
        // deletes the temporary file when it goes out of scope, unless it has been moved to the final location.
        auto const tempDirectory = Application::Neuralyzer::getDownloadTempDirectory();
        auto const createResult = tempDirectory.createDirectory();
        if(createResult.failed())
        {
            return juce::Result::fail(juce::translate("The output directory 'DIRPATH' cannot be created: ERRSTR").replace("DIRPATH", tempDirectory.getFullPathName()).replace("ERRSTR", createResult.getErrorMessage()));
        }
        juce::TemporaryFile const tempFile{output, tempDirectory.getChildFile(juce::String::toHexString(static_cast<juce::int64>(output.getFullPathName().hashCode64())) + "-" + output.getFileName())};

        static auto constexpr connectionTimeoutMs = 10000;
        std::atomic<bool> shouldContinue{true};
        int statusCode = 0;
        auto const options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                 .withConnectionTimeoutMs(connectionTimeoutMs)
                                 .withStatusCode(&statusCode)
                                 .withProgressCallback([=, sc = &shouldContinue](int avd, int total)
                                                       {
                                                           sc->store(callback != nullptr ? callback(avd, total) : true);
                                                           return sc->load();
                                                       });
        auto const result = Downloader::download(url, tempFile.getFile(), options);
        if(result.failed())
        {
            return result;
        }
        if(statusCode < 200 || statusCode >= 300)
        {
            return juce::Result::fail(juce::translate("Failed to download from URL \"URLPATH\": HTTP STATUS.").replace("URLPATH", url.toString(true)).replace("STATUS", juce::String(statusCode)));
        }
        if(!shouldContinue.load())
        {
            return juce::Result::fail(juce::translate("Download from URL \"URLPATH\" cancelled by user.").replace("URLPATH", url.toString(true)));
        }
        if(!tempFile.overwriteTargetFileWithTemporary())
        {
            return juce::Result::fail(juce::translate("Failed to move the file \"FILEPATH\" to its final location.").replace("FILEPATH", output.getFullPathName()));
        }
        if(callback != nullptr)
        {
            callback(100, 100);
        }

        return juce::Result::ok();
    }
} // namespace

Application::Neuralyzer::Downloader::Process::Process(juce::File destinationFile, juce::URL sourceUrl, std::function<void(Process const&)> callback)
: mCallback(std::move(callback))
{
    auto const createResult = destinationFile.getParentDirectory().createDirectory();
    if(createResult.failed())
    {
        mResult = juce::Result::fail(juce::translate("The output directory 'DIRPATH' cannot be created: ERRSTR").replace("DIRPATH", destinationFile.getParentDirectory().getFullPathName()).replace("ERRSTR", createResult.getErrorMessage()));
        triggerAsyncUpdate();
        return;
    }

    mOutputFile = destinationFile;
    mIsRunning.store(true);
    mAsyncProcess = std::async([this, destinationFile, sourceUrl]()
                               {
                                   juce::Thread::setCurrentThreadName("Application::Neuralyzer::Downloader");
                                   auto result = downloadModel(destinationFile, sourceUrl, [this](int avd, int total)
                                                               {
                                                                   mProcessProgress.store(total > 0 ? static_cast<float>(avd) / static_cast<float>(total) : 0.0f);
                                                                   return mShouldContinue.load();
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

void Application::Neuralyzer::Downloader::Process::handleAsyncUpdate()
{
    if(mAsyncProcess.valid())
    {
        mAsyncProcess.wait();
        mResult = mAsyncProcess.get();
    }
    mIsRunning.store(false);
    sendChangeMessage();
    if(mShouldContinue.load() && mCallback != nullptr)
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
                           return process->getOutputFile() == output;
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
, mCancelButton(juce::ImageCache::getFromMemory(AnlIconsData::cancel_png, AnlIconsData::cancel_pngSize))
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
