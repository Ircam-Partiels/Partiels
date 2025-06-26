#include "AnlDocumentExecutor.h"
#include "AnlDocumentFileBased.h"

ANALYSE_FILE_BEGIN

Document::Executor::Executor()
{
    anlDebug("Executor", "Register audio formats...");
    mAudioFormatManager.registerBasicFormats();
}

juce::Result Document::Executor::load(juce::File const& audioFile, juce::File const& templateFile, bool adaptOnSampleRate)
{
    if(std::none_of(mAudioFormatManager.begin(), mAudioFormatManager.end(), [&](auto* audioFormat)
                    {
                        return audioFormat != nullptr && audioFormat->canHandleFile(audioFile);
                    }))
    {
        return juce::Result::fail(juce::translate("The audio file FLNM is not supported!").replace("FLNM", audioFile.getFileName()));
    }

    anlDebug("Executor", "Reset document...");
    mAccessor.copyFrom(Accessor(), NotificationType::synchronous);

    anlDebug("Executor", "Loading audio file...");
    auto const layout = getAudioFileLayouts(mAudioFormatManager, {audioFile}, AudioFileLayout::ChannelLayout::split);
    mAccessor.setAttr<AttrType::reader>(layout, NotificationType::synchronous);

    anlDebug("Executor", "Loading template file...");
    AlertWindow::Catcher catcher;
    mDirector.setAlertCatcher(&catcher);

    auto fileResult = Document::FileBased::parse(templateFile);
    if(fileResult.index() == 1_z)
    {
        return std::get<1>(fileResult);
    }
    auto xml = std::move(std::get<0>(fileResult));

    // Adjust the path in the XML to match the new template file location
    XmlParser::replaceAllAttributeValues(*xml.get(), FileBased::getPathReplacement(*xml.get()), templateFile.getParentDirectory().getFullPathName() + juce::File::getSeparatorString());

    Document::FileBased::loadTemplate(mAccessor, *xml.get(), adaptOnSampleRate);

    anlDebug("Executor", "Sanitize document...");
    [[maybe_unused]] auto const references = mDirector.sanitize(NotificationType::synchronous);

    mDirector.setAlertCatcher(nullptr);

    anlDebug("Executor", "Reset time range...");
    mAccessor.getAcsr<Document::AcsrType::timeZoom>().setAttr<Zoom::AttrType::visibleRange>(Zoom::Range{0.0, std::numeric_limits<double>::max()}, NotificationType::synchronous);

    anlDebug("Executor", "Set channels layout...");
    auto const trackAcsrs = mAccessor.getAcsrs<Document::AcsrType::tracks>();
    for(auto trackAcsr : trackAcsrs)
    {
        auto trackChannelsLayout = trackAcsr.get().getAttr<Track::AttrType::channelsLayout>();
        std::fill(trackChannelsLayout.begin(), trackChannelsLayout.end(), true);
        trackAcsr.get().setAttr<Track::AttrType::channelsLayout>(trackChannelsLayout, NotificationType::synchronous);
    }

    return juce::Result::ok();
}

juce::Result Document::Executor::launch()
{
    anlDebug("Executor", "Check for warnings...");
    auto const trackAcsrs = mAccessor.getAcsrs<Document::AcsrType::tracks>();
    auto const warningTrackIt = std::find_if(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                                             {
                                                 return trackAcsr.get().template getAttr<Track::AttrType::warnings>() != Track::WarningType::none;
                                             });
    if(warningTrackIt != trackAcsrs.cend())
    {
        auto const trackName = warningTrackIt->get().getAttr<Track::AttrType::name>();
        auto const warningType = std::string(magic_enum::enum_name(warningTrackIt->get().getAttr<Track::AttrType::warnings>()));
        return juce::Result::fail(juce::translate("Error: The track TRACKNAME contains the error type 'ERRORTYPE'").replace("TRACKNAME", trackName).replace("ERRORTYPE", warningType));
    }

    timerCallback();
    startTimer(20);
    return juce::Result::ok();
}

juce::Result Document::Executor::saveTo(juce::File const& outputFile)
{
    anlDebug("Executor", "Check for warnings...");
    auto const trackAcsrs = mAccessor.getAcsrs<Document::AcsrType::tracks>();
    if(std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                   {
                       return trackAcsr.get().template getAttr<Track::AttrType::warnings>() != Track::WarningType::none;
                   }))
    {
        return juce::Result::fail(juce::translate("Warning generated!"));
    }

    anlDebug("Executor", "Saving document...");
    return FileBased::saveTo(mAccessor, outputFile);
}

juce::Result Document::Executor::exportTo(juce::File const& outputDir, juce::String const& filePrefix, Document::Exporter::Options const& options, juce::String const& identifier)
{
    anlDebug("Executor", "Exporting results...");
    std::atomic<bool> shouldAbort{false};
    return Exporter::toFile(mAccessor, outputDir, {}, filePrefix, identifier, options, shouldAbort, nullptr);
}

void Document::Executor::timerCallback()
{
    anlDebug("Executor", "Waiting for results...");
    auto const trackAcsrs = mAccessor.getAcsrs<Document::AcsrType::tracks>();
    if(std::none_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                    {
                        auto const& processing = trackAcsr.get().template getAttr<Track::AttrType::processing>();
                        return std::get<0>(processing) || std::get<2>(processing);
                    }))
    {
        stopTimer();
        if(onEnded != nullptr)
        {
            onEnded();
        }
    }
}

bool Document::Executor::isRunning() const
{
    return isTimerRunning();
}

ANALYSE_FILE_END
