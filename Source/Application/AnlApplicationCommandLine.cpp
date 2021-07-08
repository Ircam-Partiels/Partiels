#include "AnlApplicationCommandLine.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

juce::Result Application::CommandLine::analyzeAndExport(juce::File const& audioFile, juce::File const& templateFile, juce::File const& outputDir, juce::File const& optionsFile, juce::String const& identifier)
{
    auto* messageManager = juce::MessageManager::getInstanceWithoutCreating();
    if(messageManager == nullptr)
    {
        return juce::Result::fail("Message manager doesn't exist!");
    }
    auto xml = juce::XmlDocument::parse(optionsFile);
    if(xml == nullptr)
    {
        return juce::Result::fail(juce::translate("The options file FLNM cannot be parsed!").replace("FLNM", optionsFile.getFileName()));
    }
    Document::Exporter::Options options;
    options = XmlParser::fromXml(*xml, "exportOptions", options);
    if(options.useAutoSize)
    {
        return juce::Result::fail(juce::translate("The auto-size option is not supported by the command-line interface!"));
    }

    juce::AudioFormatManager audioFormatManager;
    audioFormatManager.registerBasicFormats();
    if(std::none_of(audioFormatManager.begin(), audioFormatManager.end(), [&](auto* audioFormat)
                    {
                        return audioFormat != nullptr && audioFormat->canHandleFile(audioFile);
                    }))
    {
        return juce::Result::fail(juce::translate("The audio file FLNM is not supported!").replace("FLNM", audioFile.getFileName()));
    }

    juce::UndoManager undoManager;
    Document::Accessor documentAccessor;
    Document::Director documentDirector(documentAccessor, audioFormatManager, undoManager);
    Document::FileBased documentFileBased(documentDirector, Application::Instance::getFileExtension(), Application::Instance::getFileWildCard(), "", "");

    documentAccessor.setAttr<Document::AttrType::reader>({AudioFileLayout{audioFile, AudioFileLayout::ChannelLayout::all}}, NotificationType::synchronous);
    documentAccessor.getAcsr<Document::AcsrType::timeZoom>().setAttr<Zoom::AttrType::visibleRange>(Zoom::Range{0.0, std::numeric_limits<double>::max()}, NotificationType::synchronous);
    auto const result = documentFileBased.loadTemplate(templateFile);
    if(result.failed())
    {
        return result;
    }

    auto const trackAcsrs = documentAccessor.getAcsrs<Document::AcsrType::tracks>();
    for(auto trackAcsr : trackAcsrs)
    {
        auto trackChannelsLayout = trackAcsr.get().getAttr<Track::AttrType::channelsLayout>();
        std::fill(trackChannelsLayout.begin(), trackChannelsLayout.end(), true);
        trackAcsr.get().setAttr<Track::AttrType::channelsLayout>(trackChannelsLayout, NotificationType::synchronous);
    }

    while(std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                      {
                          auto const& processing = trackAcsr.get().template getAttr<Track::AttrType::processing>();
                          return std::get<0>(processing) || std::get<2>(processing);
                      }))
    {
        messageManager->runDispatchLoopUntil(20);
    }

    std::atomic<bool> shouldAbort{false};
    return Document::Exporter::toFile(documentAccessor, outputDir, audioFile.getFileNameWithoutExtension() + " ", identifier, options, shouldAbort, nullptr);
}

int Application::CommandLine::run(int argc, char* argv[])
{
    juce::ConsoleApplication app;
    app.addHelpCommand("--help|-h", "Usage:", true);
    app.addVersionCommand("--version|-v", juce::String(ProjectInfo::projectName) + " v" + ProjectInfo::versionString);
    app.addDefaultCommand(
        {"",
         "audio-file template-file output-directory options-file",
         "Analyzes an audio file and exports the results",
         "Analyzes an audio file with a given template and exports the results with given options in a directory.",
         [](juce::ArgumentList const& args)
         {
             if(args.size() < 4)
             {
                 juce::ConsoleApplication::fail("Missing arguments");
             }
             if(args.size() > 4)
             {
                 juce::ConsoleApplication::fail("Extra arguments");
             }
             auto const audioFile = args[0].resolveAsExistingFile();
             auto const templateFile = args[1].resolveAsExistingFile();
             auto const outputDir = args[2].resolveAsFile();
             if(!outputDir.exists())
             {
                 auto const result = outputDir.createDirectory();
                 if(result.failed())
                 {
                     juce::ConsoleApplication::fail(result.getErrorMessage());
                 }
             }
             if(!outputDir.isDirectory())
             {
                 juce::ConsoleApplication::fail("Could not find folder: " + outputDir.getFullPathName());
             }
             auto const optionsFile = args[3].resolveAsExistingFile();
             juce::MessageManager::getInstance();
             LookAndFeel lookAndFeel;
             juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
             auto const result = analyzeAndExport(audioFile, templateFile, outputDir, optionsFile, "");
             juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
             juce::MessageManager::deleteInstance();
             juce::DeletedAtShutdown::deleteAll();
             if(result.failed())
             {
                 juce::ConsoleApplication::fail(result.getErrorMessage());
             }
         }});

    return app.findAndRunCommand(argc, argv);
}

ANALYSE_FILE_END
