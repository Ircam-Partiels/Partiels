#include "AnlDocumentExecutor.h"
#include "../Track/AnlTrackTools.h"
#include "AnlDocumentFileBased.h"

ANALYSE_FILE_BEGIN

Document::Executor::Executor()
{
    anlDebug("Executor", "Register audio formats...");
    mAudioFormatManager.registerBasicFormats();
}

juce::Result Document::Executor::load(juce::File const& documentFile)
{
    anlDebug("Executor", "Loading document file...");
    auto fileResult = Document::FileBased::parse(documentFile);
    if(fileResult.index() == 1_z)
    {
        return std::get<1>(fileResult);
    }
    auto xml = std::move(std::get<0>(fileResult));

    // Adjust the path in the XML to match the new template file location
    XmlParser::replaceAllAttributeValues(*xml.get(), FileBased::getPathReplacement(*xml.get()), juce::File::addTrailingSeparator(documentFile.getParentDirectory().getFullPathName()));

    AlertWindow::Catcher catcher;
    mDirector.setAlertCatcher(&catcher);
    mAccessor.fromXml(*xml.get(), {"document"}, NotificationType::synchronous);
    mDirector.setAlertCatcher(nullptr);

    return juce::Result::ok();
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
    XmlParser::replaceAllAttributeValues(*xml.get(), FileBased::getPathReplacement(*xml.get()), juce::File::addTrailingSeparator(templateFile.getParentDirectory().getFullPathName()));

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

    anlDebug("Executor", "Saving document...");
    return FileBased::saveTo(mAccessor, outputFile);
}

juce::Result Document::Executor::exportTo(juce::File const& outputDir, juce::String const& filePrefix, Document::Exporter::Options const& options, bool useGroupOverview, bool ignoreGridResults)
{
    MiscDebug("Executor", "Exporting results...");
    MiscWeakAssert(!isRunning());
    if(isRunning())
    {
        MiscDebug("Executor", "Cannot export while running analysis or file parsing");
        return juce::Result::fail(juce::translate("Cannot export while running analysis or file parsing"));
    }
    std::atomic<bool> shouldAbort{false};
    std::set<juce::String> identifiers;
    juce::MessageManager::Lock lock;
    if(!lock.tryEnter())
    {
        MiscDebug("Executor", "Invalid threaded access");
        return juce::Result::fail(juce::translate("Invalid threaded access"));
    }
    if(options.useImageFormat() && useGroupOverview)
    {
        for(auto const& identifier : Tools::getEffectiveGroupIdentifiers(mAccessor))
        {
            identifiers.insert(identifier);
        }
    }
    else
    {
        for(auto const& identifier : Tools::getEffectiveTrackIdentifiers(mAccessor))
        {
            auto const& trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
            auto const frameType = Track::Tools::getFrameType(trackAcsr);
            if(frameType.has_value())
            {
                auto const trackIgnored = frameType.value() == Track::FrameType::vector && ignoreGridResults;
                if(options.isCompatible(frameType.value()) && !trackIgnored)
                {
                    identifiers.insert(identifier);
                }
            }
        }
    }
    lock.exit();
    if(identifiers.empty())
    {
        MiscDebug("Executor", "No results to export");
        return juce::Result::fail(juce::translate("No results to export"));
    }
    return Exporter::exportTo(mAccessor, outputDir, {}, {}, filePrefix, identifiers, options, shouldAbort);
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
