#include "AnlApplicationCommandLine.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

static juce::Result createAndSave(juce::File const& audioFile, juce::File const& templateFile, juce::File const& outputFile, bool adaptationToSampleRate)
{
    anlDebug("CommandLine", "Begin creation...");
    juce::AudioFormatManager audioFormatManager;
    audioFormatManager.registerBasicFormats();
    if(std::none_of(audioFormatManager.begin(), audioFormatManager.end(), [&](auto* audioFormat)
                    {
                        return audioFormat != nullptr && audioFormat->canHandleFile(audioFile);
                    }))
    {
        return juce::Result::fail(juce::translate("The audio file FLNM is not supported!").replace("FLNM", audioFile.getFileName()));
    }

    auto fileResult = Document::FileBased::parse(templateFile);
    if(fileResult.index() == 1_z)
    {
        return *std::get_if<1>(&fileResult);
    }
    auto xml = std::move(*std::get_if<0>(&fileResult));

    auto parentDirectoryResult = outputFile.getParentDirectory().createDirectory();
    if(parentDirectoryResult.failed())
    {
        return parentDirectoryResult;
    }

    Document::Accessor documentAccessor;
    documentAccessor.setAttr<Document::AttrType::reader>({AudioFileLayout{audioFile, AudioFileLayout::ChannelLayout::all}}, NotificationType::synchronous);
    Document::FileBased::loadTemplate(documentAccessor, *xml.get(), adaptationToSampleRate);
    return Document::FileBased::saveTo(documentAccessor, outputFile);
}

static juce::Result analyzeAndExport(juce::File const& audioFile, juce::File const& templateFile, juce::File const& outputDir, Document::Exporter::Options const& options, bool adaptationToSampleRate, juce::String const& identifier)
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
    auto const result = documentFileBased.loadTemplate(templateFile, adaptationToSampleRate);
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
    auto const results = Document::Exporter::toFile(documentAccessor, outputDir, audioFile.getFileNameWithoutExtension() + " ", identifier, options, shouldAbort, nullptr);
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    return results;
}

Application::CommandLine::CommandLine()
{
    addHelpCommand("--help|-h", "Usage:", false);
    addVersionCommand("--version|-v", juce::String(ProjectInfo::projectName) + " v" + ProjectInfo::versionString);
    addCommand(
        {"--new|-n",
         "--new|-n [options]",
         "Creates a new document with a template and an audio file.\n\t"
         "--input|-i <audiofile> Defines the path to the audio file to analyze (required).\n\t"
         "--template|-t <templatefile> Defines the path to the template file (required).\n\t"
         "--output|-o <outputfile> Defines the path of the output file (required).\n\t"
         "--adapt Defines if the block size and the step size of the analyzes are adapted following the sample rate (optional).\n\t"
         "",
         "",
         [](juce::ArgumentList const& args)
         {
             anlDebug("CommandLine", "Parsing arguments...");
             auto const audioFile = args.getExistingFileForOption("-i|--input");
             auto const templateFile = args.getExistingFileForOption("-t|--template");
             auto const outputFile = args.getFileForOption("-o|--output");
             auto const adaptationToSampleRate = args.containsOption("-adapt");

             auto const result = createAndSave(audioFile, templateFile, outputFile, adaptationToSampleRate);
             if(result.failed())
             {
                 juce::ConsoleApplication::fail(result.getErrorMessage());
             }
         }});
    addCommand(
        {"--export|-e",
         "--export|-e [options]",
         "Analyzes an audio file and exports the results.\n\t"
         "--input|-i <audiofile> Defines the path to the audio file to analyze (required).\n\t"
         "--template|-t <templatefile> Defines the path to the template file (required).\n\t"
         "--output|-o <outputdirectory> Defines the path of the output folder (required).\n\t"
         "--format|-f <formatname> Defines the export format (jpeg, png, csv, json or cue) (required).\n\t"
         "--width|-w <width> Defines the width of the exported image in pixels (required with the jpeg and png formats).\n\t"
         "--height|-h <height> Defines the height of the exported image in pixels (required with the jpeg and png formats).\n\t"
         "--adapt Defines if the block size and the step size of the analyzes are adapted following the sample rate (optional).\n\t"
         "--groups Exports the images of group and not the image of the tracks (optional with the jpeg and png formats).\n\t"
         "--nogrids Ignores the export of the grid tracks (optional with the csv, json or cue formats).\n\t"
         "--header Includes header row before the data rows (optional with the csv format).\n\t"
         "--separator <character> Defines the separator character between columns (optional with the csv format, default is ',').\n\t"
         "--description Includes the plugin description (optional with the json format).\n\t"
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

             auto const adaptationToSampleRate = args.containsOption("-adapt");

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
                 juce::ConsoleApplication::fail("Format not specified! Available formats are jpeg, png, csv, json or cue.");
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
             else if(format == "csv" || format == "json" || format == "cue")
             {
                 if(format == "csv")
                 {
                     options.format = Options::Format::csv;
                 }
                 else if(format == "json")
                 {
                     options.format = Options::Format::json;
                 }
                 else
                 {
                     options.format = Options::Format::cue;
                 }
                 options.ignoreGridResults = args.containsOption("--nogrids");
                 options.includeHeaderRaw = args.containsOption("--header");
                 options.includeDescription = args.containsOption("--description");
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

             auto const result = analyzeAndExport(audioFile, templateFile, outputDir, options, adaptationToSampleRate, "");
             if(result.failed())
             {
                 juce::ConsoleApplication::fail(result.getErrorMessage());
             }
         }});
    addCommand(
        {"--sdif2json",
         "--sdif2json [options]",
         "Converts a SDIF file to a JSON file.\n\t"
         "--input|-i <sdiffile> Defines the path to the input SDIF file to convert (required).\n\t"
         "--output|-o <jsonfile> Defines the path of the output JSON file (required).\n\t"
         "--frame|-f <framesignature> The 4 characters frame signature (required).\n\t"
         "--matrix|-m <matrixsignature> The 4 characters matrix signature (required).\n\t"
         "--row|-r <rowindex> The index of the row (optional - all rows if not defined).\n\t"
         "--column|-c <columindex> The index of the column (optional - all columns if not defined).\n\t"
         "--unit <unit> The unit of the results (optional).\n\t"
         "--min <value> The minimum possible value of the results (required if max defined).\n\t"
         "--max <value> The maximum possible value of the results (required if max defined).\n\t",
         "",
         [](juce::ArgumentList const& args)
         {
             args.failIfOptionIsMissing("-i|--input");
             args.failIfOptionIsMissing("-o|--output");
             args.failIfOptionIsMissing("-f|--frame");
             args.failIfOptionIsMissing("-m|--matrix");
             if(args.containsOption("--min") || args.containsOption("--max"))
             {
                 args.failIfOptionIsMissing("--min");
                 args.failIfOptionIsMissing("--max");
             }
             auto const inputFile = args.getExistingFileForOption("-i|--input");
             auto const outputFile = args.getFileForOption("-o|--output");
             auto const frame = args.getValueForOption("-f|--frame");
             auto const matrix = args.getValueForOption("-m|--matrix");
             auto const row = args.containsOption("-r|--row") ? args.getValueForOption("-r|--row").getIntValue() : -1;
             auto const column = args.containsOption("-c|--column") ? args.getValueForOption("-c|--column").getIntValue() : -1;
             if(frame.length() != 4 || matrix.length() != 4)
             {
                 fail("Signature must contain 4 characters!");
             }
             auto const frameSig = SdifConverter::getSignature(frame);
             auto const matrixSig = SdifConverter::getSignature(matrix);

             auto const unit = args.getValueForOption("--unit");
             juce::Range<double> const range{static_cast<double>(args.getValueForOption("--min").getFloatValue()), (args.getValueForOption("--max").getFloatValue())};
             auto extra = std::optional<nlohmann::json>{};
             if(!unit.isEmpty() || !range.isEmpty())
             {
                 auto json = nlohmann::json::object();
                 auto& output = json["track"]["description"]["output"];
                 if(!unit.isEmpty())
                 {
                     output["unit"] = unit.toStdString();
                 }
                 if(!range.isEmpty())
                 {
                     output["minValue"] = range.getStart();
                     output["maxValue"] = range.getEnd();
                     output["hasKnownExtents"] = true;
                 }
                 extra = std::optional<nlohmann::json>(std::move(json));
             }

             auto const result = SdifConverter::toJson(inputFile, outputFile, frameSig, matrixSig, row < 0 ? std::optional<size_t>{} : static_cast<size_t>(row), column < 0 ? std::optional<size_t>{} : static_cast<size_t>(column), extra);
             if(result.failed())
             {
                 juce::ConsoleApplication::fail(result.getErrorMessage());
             }
         }});
    addCommand(
        {"--json2sdif",
         "--json2sdif [options]",
         "Converts a JSON file to a SDIF file.\n\t"
         "--input|-i <jsonfile> Defines the path to the input JSON file to convert (required).\n\t"
         "--output|-o <sdiffile> Defines the path of the output SDIF file (required).\n\t"
         "--frame|-f <framesignature> The 4 characters frame signature (required).\n\t"
         "--matrix|-m <matrixsignature> The 4 characters matrix signature (required).\n\t",
         "--colname <name> The name of the column (optional).\n\t"
         "",
         [](juce::ArgumentList const& args)
         {
             args.failIfOptionIsMissing("-i|--input");
             args.failIfOptionIsMissing("-o|--output");
             args.failIfOptionIsMissing("-f|--frame");
             args.failIfOptionIsMissing("-m|--matrix");
             auto const inputFile = args.getExistingFileForOption("-i|--input");
             auto const outputFile = args.getFileForOption("-o|--output");
             auto const frame = args.getValueForOption("-f|--frame");
             auto const matrix = args.getValueForOption("-m|--matrix");
             if(frame.length() != 4 || matrix.length() != 4)
             {
                 fail("Signature must contain 4 characters!");
             }
             auto const frameSig = SdifConverter::getSignature(frame);
             auto const matrixSig = SdifConverter::getSignature(matrix);
             auto const colname = args.containsOption("--colname") ? args.getValueForOption("--colname") : std::optional<juce::String>{};
             auto const result = SdifConverter::fromJson(inputFile, outputFile, frameSig, matrixSig, colname);
             if(result.failed())
             {
                 juce::ConsoleApplication::fail(result.getErrorMessage());
             }
         }});
}

static juce::Result compareResultsFiles(juce::File const& expectedFile, juce::File const& generatedFile, juce::File const& arguments)
{
    std::atomic<bool> const shouldAbort{false};
    std::atomic<float> advancement{0.0f};
    Track::FileInfo expectedTrackInfo;
    expectedTrackInfo.file = expectedFile;
    Track::FileInfo generatedTrackInfo;
    generatedTrackInfo.file = generatedFile;
    if(arguments != juce::File{})
    {
        auto xml = juce::XmlDocument::parse(arguments);
        if(xml == nullptr)
        {
            return juce::Result::fail("Cannot parse arguments!");
        }
        auto const args = XmlParser::fromXml(*xml.get(), "args", juce::StringPairArray{});
        generatedTrackInfo.args = args;
        expectedTrackInfo.args = args;
    }

    auto const expectedResults = Track::Loader::loadFromFile(expectedTrackInfo, shouldAbort, advancement);
    auto const generatedResults = Track::Loader::loadFromFile(generatedTrackInfo, shouldAbort, advancement);
    if(expectedResults.index() == 1_z)
    {
        return juce::Result::fail(*std::get_if<juce::String>(&expectedResults));
    }
    if(generatedResults.index() == 1_z)
    {
        return juce::Result::fail(*std::get_if<juce::String>(&generatedResults));
    }
    auto const expectedTrackResults = *std::get_if<Track::Results>(&expectedResults);
    auto const generatedTrackResults = *std::get_if<Track::Results>(&generatedResults);
    if(expectedTrackResults.isEmpty())
    {
        if(!generatedTrackResults.isEmpty())
        {
            return juce::Result::fail("Different types!");
        }
        return juce::Result::ok();
    }

    auto timeAndDurationAreEquivalent = [](auto const& lhs, auto const& rhs)
    {
        return std::abs(std::get<0>(lhs) - std::get<0>(rhs)) < 0.00001 && std::abs(std::get<1>(lhs) - std::get<1>(rhs)) < 0.00001;
    };

    auto resultsAreEquivalent = [](auto const& expectedResultsPtr, auto const& generatedResultsPtr, auto lambda)
    {
        if(expectedResultsPtr == nullptr && generatedResultsPtr == nullptr)
        {
            return true;
        }
        if(expectedResultsPtr == nullptr || generatedResultsPtr == nullptr)
        {
            return false;
        }
        if(expectedResultsPtr->size() != generatedResultsPtr->size())
        {
            return false;
        }
        return std::equal(expectedResultsPtr->cbegin(), expectedResultsPtr->cend(), generatedResultsPtr->cbegin(), [=](auto const& expectedChannel, auto const& generatedChannel)
                          {
                              if(expectedChannel.size() != generatedChannel.size())
                              {
                                  return false;
                              }
                              return std::equal(expectedChannel.cbegin(), expectedChannel.cend(), generatedChannel.cbegin(), lambda);
                          });
    };

    if(!resultsAreEquivalent(expectedTrackResults.getMarkers(), generatedTrackResults.getMarkers(), [&](Track::Results::Marker const& lhs, Track::Results::Marker const& rhs)
                             {
                                 return timeAndDurationAreEquivalent(lhs, rhs) && std::get<2>(lhs) == std::get<2>(rhs);
                             }))
    {
        return juce::Result::fail("Different results!");
    }

    if(!resultsAreEquivalent(expectedTrackResults.getPoints(), generatedTrackResults.getPoints(), [&](Track::Results::Point const& lhs, Track::Results::Point const& rhs)
                             {
                                 if(timeAndDurationAreEquivalent(lhs, rhs) && std::get<2>(lhs).has_value() == std::get<2>(rhs).has_value())
                                 {
                                     return !std::get<2>(lhs).has_value() || std::abs(*std::get<2>(lhs) - *std::get<2>(rhs)) < 0.01;
                                 }
                                 return false;
                             }))
    {
        return juce::Result::fail("Different results!");
    }

    if(!resultsAreEquivalent(expectedTrackResults.getColumns(), generatedTrackResults.getColumns(), [&](Track::Results::Column const& lhs, Track::Results::Column const& rhs)
                             {
                                 if(timeAndDurationAreEquivalent(lhs, rhs) && std::get<2>(lhs).size() == std::get<2>(rhs).size())
                                 {
                                     return std::equal(std::get<2>(lhs).cbegin(), std::get<2>(lhs).cend(), std::get<2>(rhs).cbegin(), [](auto const& lhsv, auto const& rhsv)
                                                       {
                                                           return std::abs(lhsv - rhsv) < 0.01;
                                                       });
                                 }
                                 return false;
                             }))
    {
        return juce::Result::fail("Different results!");
    }
    return juce::Result::ok();
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

#if JUCE_MAC
    juce::Process::setDockIconVisible(false);
#endif

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

    if(args[0].isLongOption("compare-files"))
    {
        if(args.size() < 3 || args.size() > 4)
        {
            std::cerr << "Missing arguments! Expected two results file and one optional argument xml file!" << std::endl;
            return 1;
        }
        auto const arguments = args.size() == 4 ? args[3].resolveAsFile() : juce::File();
        auto const results = compareResultsFiles(args[1].resolveAsFile(), args[2].resolveAsFile(), arguments);
        if(results.failed())
        {
            std::cerr << results.getErrorMessage() << std::endl;
            return 1;
        }
        return 0;
    }

    anlDebug("Application", "Running as CLI");
    CommandLine cmd;
    return cmd.findAndRunCommand(args);
}

ANALYSE_FILE_END
