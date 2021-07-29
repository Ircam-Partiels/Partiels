#include "AnlApplicationCommandLine.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

static juce::Result analyzeAndExport(juce::File const& audioFile, juce::File const& templateFile, juce::File const& outputDir, Document::Exporter::Options const& options, juce::String const& identifier)
{
    anlDebug("CommandLine", "Begin analysis...");
    auto* messageManager = juce::MessageManager::getInstanceWithoutCreating();
    if(messageManager == nullptr)
    {
        return juce::Result::fail("Message manager doesn't exist!");
    }

    Application::LookAndFeel lookAndFeel;
    juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);

    if(options.useAutoSize)
    {
        return juce::Result::fail(juce::translate("The auto-size option is not supported by the command-line interface!"));
    }

    anlDebug("CommandLine", "Register audio formats...");
    juce::AudioFormatManager audioFormatManager;
    audioFormatManager.registerBasicFormats();
    if(std::none_of(audioFormatManager.begin(), audioFormatManager.end(), [&](auto* audioFormat)
                    {
                        return audioFormat != nullptr && audioFormat->canHandleFile(audioFile);
                    }))
    {
        return juce::Result::fail(juce::translate("The audio file FLNM is not supported!").replace("FLNM", audioFile.getFileName()));
    }

    anlDebug("CommandLine", "Preparing document...");
    juce::UndoManager undoManager;
    Document::Accessor documentAccessor;
    Document::Director documentDirector(documentAccessor, audioFormatManager, undoManager);
    Document::FileBased documentFileBased(documentDirector, Application::Instance::getFileExtension(), Application::Instance::getFileWildCard(), "", "");

    anlDebug("CommandLine", "Loading audio file...");
    documentAccessor.setAttr<Document::AttrType::reader>({AudioFileLayout{audioFile, AudioFileLayout::ChannelLayout::all}}, NotificationType::synchronous);
    
    anlDebug("CommandLine", "Loading template file...");
    auto const result = documentFileBased.loadTemplate(templateFile);
    if(result.failed())
    {
        return result;
    }
    documentAccessor.getAcsr<Document::AcsrType::timeZoom>().setAttr<Zoom::AttrType::visibleRange>(Zoom::Range{0.0, std::numeric_limits<double>::max()}, NotificationType::synchronous);

    auto const trackAcsrs = documentAccessor.getAcsrs<Document::AcsrType::tracks>();
    if(std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                   {
                       return trackAcsr.get().template getAttr<Track::AttrType::warnings>() != Track::WarningType::none;
                   }))
    {
        return juce::Result::fail(juce::translate("Error"));
    }

    for(auto trackAcsr : trackAcsrs)
    {
        auto trackChannelsLayout = trackAcsr.get().getAttr<Track::AttrType::channelsLayout>();
        std::fill(trackChannelsLayout.begin(), trackChannelsLayout.end(), true);
        trackAcsr.get().setAttr<Track::AttrType::channelsLayout>(trackChannelsLayout, NotificationType::synchronous);
    }

    anlDebug("CommandLine", "Analysing file...");
    while(std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                      {
                          auto const& processing = trackAcsr.get().template getAttr<Track::AttrType::processing>();
                          return std::get<0>(processing) || std::get<2>(processing);
                      }))
    {
        messageManager->runDispatchLoopUntil(2);
    }

    anlDebug("CommandLine", "Exporting results...");
    std::atomic<bool> shouldAbort{false};
    auto const results =  Document::Exporter::toFile(documentAccessor, outputDir, audioFile.getFileNameWithoutExtension() + " ", identifier, options, shouldAbort, nullptr);
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    return results;
}

Application::CommandLine::CommandLine()
{
    addHelpCommand("--help|-h", "Usage:", false);
    addVersionCommand("--version|-v", juce::String(ProjectInfo::projectName) + " v" + ProjectInfo::versionString);
    addCommand(
        {"--export|-e",
         "--export|-e [options]",
         "Analyzes an audio file and exports the results.\n\t"
         "--input|-i <audiofile> Defines the path to the audio file to analyze (required).\n\t"
         "--template|-t <templatefile> Defines the path to the template file (required).\n\t"
         "--output|-o <outputdirectory> Defines the path of the output folder (required).\n\t"
         "--format|-f <formatname> Defines the export format (jpeg, png, csv or json) (required).\n\t"
         "--width|-w <width> Defines the width of the exported image in pixels (required with the jpeg and png formats).\n\t"
         "--height|-h <height> Defines the height of the exported image in pixels (required with the jpeg and png formats).\n\t"
         "--groups Exports the images of group and not the image of the tracks (optional with the jpeg and png formats).\n\t"
         "--nogrids Ignores the export of the grid tracks (optional with the csv and json formats).\n\t"
         "--header Includes header row before the data rows (optional with the csv format).\n\t"
         "--separator <character> Defines the seperatror character between colummns (optional with the csv format, default is ',').\n\t"
         "",
         "",
         [](juce::ArgumentList const& args)
         {
             anlDebug("CommandLine", "Parsing arguments...");
             using Options = Document::Exporter::Options;

             auto const audioFile = args.getExistingFileForOption("-i|--input");
             auto const templateFile = args.getFileForOption("-t|--template");
             auto const outputDir = args.getFileForOption("-o|--output");
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

             Options options;
             auto const format = args.getValueForOption("-f|--format");
             if(args.containsOption("--options"))
             {
                 auto const optionsFile = args.getExistingFileForOption("--options");
                 auto xml = juce::XmlDocument::parse(optionsFile);
                 if(xml == nullptr)
                 {
                     auto const result = juce::Result::fail(juce::translate("The options file FLNM cannot be parsed!").replace("FLNM", optionsFile.getFileName()));
                     juce::ConsoleApplication::fail(result.getErrorMessage());
                 }

                 options = XmlParser::fromXml(*xml, "exportOptions", options);
             }
             else if(!args.containsOption("-f|--format"))
             {
                 juce::ConsoleApplication::fail("Format not specified! Available formats are jpeg, png, csv or json.");
             }
             else if(format == "jpeg" || format == "png")
             {
                 if(!args.containsOption("-w|--width"))
                 {
                     juce::ConsoleApplication::fail("Width not specified! Specifiy the width of the image in pixels.");
                 }
                 if(!args.containsOption("-h|--height"))
                 {
                     juce::ConsoleApplication::fail("Height not specified! Specifiy the height of the image in pixels.");
                 }

                 options.format = format == "jpeg" ? Options::Format::jpeg : Options::Format::png;
                 options.imageWidth = args.getValueForOption("-w|--width").getIntValue();
                 options.imageHeight = args.getValueForOption("-h|--height").getIntValue();
                 options.useGroupOverview = args.containsOption("--groups");
             }
             else if(format == "csv" || format == "json")
             {
                 options.format = format == "csv" ? Options::Format::csv : Options::Format::json;
                 options.ignoreGridResults = args.containsOption("--nogrids");
                 options.includeHeaderRaw = args.containsOption("--header");
                 auto const separator = magic_enum::enum_cast<Document::Exporter::Options::ColumnSeparator>(args.getValueForOption("--separator").toStdString());
                 if(separator.has_value())
                 {
                     options.columnSeparator = *separator;
                 }
             }
             else
             {
                 juce::ConsoleApplication::fail("Format '" + format + "' unsupported! Available formats are jpeg, png, csv or json.");
             }

             auto const result = analyzeAndExport(audioFile, templateFile, outputDir, options, "");
             if(result.failed())
             {
                 juce::ConsoleApplication::fail(result.getErrorMessage());
             }
         }});
}

std::optional<int> Application::CommandLine::tryToRun(juce::String const& commandLine)
{
    if(commandLine.isEmpty() || commandLine.startsWith("-NSDocumentRevisionsDebugMode"))
    {
        anlDebug("Application", "Command line is empty or contains '-NSDocumentRevisionsDebugMode'");
        return {};
    }

    juce::ArgumentList const args("Partiels", commandLine);
    if(!args[0].isLongOption() && !args[0].isShortOption())
    {
        anlDebug("Application", "Command line doesn't contains any option");
        return {};
    }

    if(args[0].isLongOption("unit-tests"))
    {
        std::unique_ptr<juce::MessageManager> mm(juce::MessageManager::getInstance());
        juce::UnitTestRunner unitTestRunner;
        unitTestRunner.runAllTests();

        int failures = 0;
        for(int i = 0; i < unitTestRunner.getNumResults(); ++i)
        {
            if(auto* result = unitTestRunner.getResult(i))
            {
                failures += result->failures;
            }
        }
        return failures;
    }

    anlDebug("Application", "Running as CLI");
#ifdef JUCE_MAC
    juce::Process::setDockIconVisible(false);
#endif
    CommandLine cmd;
    return cmd.findAndRunCommand(args);
}

ANALYSE_FILE_END
