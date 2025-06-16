#include "AnlApplicationCommandLine.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::CommandLine::CommandLine()
{
    addHelpCommand("--help|-h", "Usage:", false);
    addVersionCommand("--version|-v", juce::String(ProjectInfo::projectName) + " v" + Instance::get().getApplicationVersion());
    addCommand({"", "[file(s)]", "Loads the document or creates a new document with the audio files specified as arguments.", "", {}});
    addCommand(
        {"--new|-n",
         "--new|-n [options]",
         "Creates a new document with a template and an audio file.\n\t"
         "--input|-i <audiofile> Defines the path to the audio file to analyze (required).\n\t"
         "--template|-t <templatefile> Defines the path to the template file (required).\n\t"
         "--output|-o <outputfile> Defines the path of the output file (required).\n\t"
         "--adapt Defines if the block size and the step size of the analyzes are adapted following the sample rate (optional).",
         "",
         [this](juce::ArgumentList const& args)
         {
             anlDebug("CommandLine", "Parsing arguments...");
             auto const audioFile = args.getExistingFileForOption("-i|--input");
             auto const templateFile = args.getExistingFileForOption("-t|--template");
             auto const outputFile = args.getFileForOption("-o|--output").withFileExtension(Instance::getExtensionForDocumentFile());
             auto const adaptToSampleRate = args.containsOption("--adapt");

             auto parentDirectoryResult = outputFile.getParentDirectory().createDirectory();
             if(parentDirectoryResult.failed())
             {
                 fail(parentDirectoryResult.getErrorMessage());
             }

             mExecutor = std::make_unique<Document::Executor>();
             if(mExecutor == nullptr)
             {
                 fail("Cannot allocate executor!");
             }
             auto result = mExecutor->load(audioFile, templateFile, adaptToSampleRate);
             if(result.failed())
             {
                 fail(result.getErrorMessage());
             }

             result = mExecutor->saveTo(outputFile);
             if(result.failed())
             {
                 fail(result.getErrorMessage());
             }
         }});
    addCommand(
        {"--export|-e",
         "--export|-e [options]",
         "Analyzes an audio file and exports the results.\n\t"
         "--input|-i <audiofile> Defines the path to the audio file to analyze (required).\n\t"
         "--template|-t <templatefile> Defines the path to the template file (required).\n\t"
         "--output|-o <outputdirectory> Defines the path of the output folder (required).\n\t"
         "--format|-f <formatname> Defines the export format (jpeg, png, csv, lab, json, cue, reaper or sdif) (required).\n\t"
         "--width <width> Defines the width of the exported image in pixels (required with the jpeg and png formats).\n\t"
         "--height <height> Defines the height of the exported image in pixels (required with the jpeg and png formats).\n\t"
         "--adapt Defines if the block size and the step size of the analyzes are adapted following the sample rate (optional).\n\t"
         "--groups Exports the images of group and not the image of the tracks (optional with the jpeg and png formats).\n\t"
         "--nogrids Ignores the export of the grid tracks (optional with the csv, json or cue formats).\n\t"
         "--header Includes header row before the data rows (optional with the csv format).\n\t"
         "--separator <character> Defines the separator character between columns (optional with the csv format, default is ',').\n\t"
         "--reapertype <type> Defines the type of the reaper format  (optional with the reaper format 'marker' or 'region', default is 'region').\n\t"
         "--description Includes the plugin description (optional with the json format).\n\t"
         "--frame <framesignature> Defines the 4 characters frame signature (required with the sdif format).\n\t"
         "--matrix <matrixsignature> Defines the 4 characters matrix signature (required with the sdif format).\n\t"
         "--colname <string> Defines the name of the column (optional with the sdif format).",
         "",
         [this](juce::ArgumentList const& args)
         {
             mShouldWait = false;
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
                     fail(result.getErrorMessage());
                 }
             }
             if(!outputDir.isDirectory())
             {
                 fail("Could not find folder: " + outputDir.getFullPathName());
             }

             auto const adaptToSampleRate = args.containsOption("--adapt");

             Options options;
             auto const format = args.getValueForOption("-f|--format");
             if(args.containsOption("--options"))
             {
                 auto const optionsFile = args.getExistingFileForOption("--options");
                 auto xml = juce::XmlDocument::parse(optionsFile);
                 if(xml == nullptr)
                 {
                     auto const result = juce::Result::fail(juce::translate("The options file FLNM cannot be parsed!").replace("FLNM", optionsFile.getFileName()));
                     fail(result.getErrorMessage());
                 }

                 options = XmlParser::fromXml(*xml, "exportOptions", options);
             }
             else if(!args.containsOption("-f|--format"))
             {
                 fail("Format not specified! Available formats are jpeg, png, csv, json or cue.");
             }
             else if(format == "jpeg" || format == "png")
             {
                 if(!args.containsOption("--width"))
                 {
                     fail("Width not specified! Specify the width of the image in pixels.");
                 }
                 if(!args.containsOption("--height"))
                 {
                     fail("Height not specified! Specify the height of the image in pixels.");
                 }

                 options.format = format == "jpeg" ? Options::Format::jpeg : Options::Format::png;
                 options.imageWidth = args.getValueForOption("--width").getIntValue();
                 options.imageHeight = args.getValueForOption("--height").getIntValue();
                 options.useGroupOverview = args.containsOption("--groups");
             }
             else if(format == "csv" || format == "lab" || format == "json" || format == "cue" || format == "reaper" || format == "sdif")
             {
                 if(format == "csv")
                 {
                     options.format = Options::Format::csv;
                 }
                 else if(format == "lab")
                 {
                     options.format = Options::Format::lab;
                 }
                 else if(format == "json")
                 {
                     options.format = Options::Format::json;
                 }
                 else if(format == "cue")
                 {
                     options.format = Options::Format::cue;
                 }
                 else if(format == "reaper")
                 {
                     options.format = Options::Format::reaper;
                 }
                 else
                 {
                     options.format = Options::Format::sdif;
                     if(!args.containsOption("--frame"))
                     {
                         fail("Frame signature not specified! Specifiy the frame signature of the SDIf file.");
                     }
                     if(!args.containsOption("--matrix"))
                     {
                         fail("Matrix signature not specified! Specifiy the matrix signature of the SDIf file.");
                     }
                 }
                 options.ignoreGridResults = args.containsOption("--nogrids");
                 options.includeHeaderRaw = args.containsOption("--header");
                 options.includeDescription = args.containsOption("--description");
                 options.reaperType = args.getValueForOption("--reapertype").toLowerCase() == "marker" ? Options::ReaperType::marker : Options::ReaperType::region;
                 options.sdifFrameSignature = args.getValueForOption("--frame").removeCharacters("\"").toUpperCase();
                 options.sdifMatrixSignature = args.getValueForOption("--matrix").removeCharacters("\"").toUpperCase();
                 options.sdifColumnName = args.getValueForOption("--colname");
                 auto const separator = magic_enum::enum_cast<Document::Exporter::Options::ColumnSeparator>(args.getValueForOption("--separator").toStdString());
                 if(separator.has_value())
                 {
                     options.columnSeparator = *separator;
                 }
             }
             else
             {
                 fail("Format '" + format + "' unsupported! Available formats are jpeg, png, csv or json.");
             }

             options.useAutoSize = false;

             mExecutor = std::make_unique<Document::Executor>();
             if(mExecutor == nullptr)
             {
                 fail("Cannot allocate executor!");
             }
             mExecutor->onEnded = [=, this]()
             {
                 LookAndFeel lookAndFeel;
                 juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
                 auto const result = mExecutor->exportTo(outputDir, audioFile.getFileNameWithoutExtension() + " ", options, "");
                 mShouldWait = false;
                 if(result.failed())
                 {
                     juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
                     fail(result.getErrorMessage());
                 }
                 juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
                 sendQuitSignal(0);
             };

             mShouldWait = true;
             auto result = mExecutor->load(audioFile, templateFile, adaptToSampleRate);
             if(result.failed())
             {
                 mShouldWait = false;
                 fail(result.getErrorMessage());
             }

             result = mExecutor->launch();
             if(result.failed())
             {
                 mShouldWait = false;
                 fail(result.getErrorMessage());
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
         "--max <value> The maximum possible value of the results (required if min defined).",
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
                 fail(result.getErrorMessage());
             }
         }});
    addCommand(
        {"--json2sdif",
         "--json2sdif [options]",
         "Converts a JSON file to a SDIF file.\n\t"
         "--input|-i <jsonfile> Defines the path to the input JSON file to convert (required).\n\t"
         "--output|-o <sdiffile> Defines the path of the output SDIF file (required).\n\t"
         "--frame|-f <framesignature> The 4 characters frame signature (required).\n\t"
         "--matrix|-m <matrixsignature> The 4 characters matrix signature (required).\n\t"
         "--colname <name> The name of the column (optional).",
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
                 fail(result.getErrorMessage());
             }
         }});
    addCommand(
        {"--plugin-list",
         "--plugin-list [options]",
         "Prints an exhaustive list of installed plugins.\n\t"
         "--description Includes the descriptions of the plugins (optional).",
         "",
         []([[maybe_unused]] juce::ArgumentList const& args)
         {
             auto const includeDescription = args.containsOption("--description");
             PluginList::Scanner scanner;
             auto const plugins = scanner.getPlugins(48000.0);
             nlohmann::json json;
             for(auto const& plugin : std::get<0>(plugins))
             {
                 nlohmann::json plug;
                 plug["key"] = plugin.first;
                 if(includeDescription)
                 {
                     plug["description"] = plugin.second;
                 }
                 json.push_back(plug);
             }
             std::cout << json.dump(4) << std::endl;
         }});
}

Application::CommandLine::~CommandLine()
{
    anlWeakAssert(!isRunning());
}

bool Application::CommandLine::isRunning() const
{
    return mShouldWait || (mExecutor != nullptr && mExecutor->isRunning());
}

void Application::CommandLine::sendQuitSignal(int value)
{
    Instance::get().setApplicationReturnValue(value);
    Instance::get().systemRequestedQuit();
}

void Application::CommandLine::runUnitTests()
{
    juce::UnitTestRunner unitTestRunner;
    unitTestRunner.setAssertOnFailure(false);
    unitTestRunner.runAllTests();

    int failures = 0;
    for(int i = 0; i < unitTestRunner.getNumResults(); ++i)
    {
        if(auto* result = unitTestRunner.getResult(i))
        {
            failures += result->failures;
        }
    }
    if(failures > 0)
    {
        fail("Unit Tests Failed!");
    }
}

void Application::CommandLine::compareFiles(juce::ArgumentList const& args)
{
    if(args.size() < 3 || args.size() > 4)
    {
        fail("Missing arguments! Expected two result files and one optional XML file!");
    }
    auto const expectedFile = args[1].resolveAsFile();
    if(!expectedFile.existsAsFile())
    {
        fail("Could not find file:" + expectedFile.getFullPathName());
    }
    auto const generatedFile = args[2].resolveAsFile();
    if(!generatedFile.existsAsFile())
    {
        fail("Could not find file:" + generatedFile.getFullPathName());
    }
    auto const argumentsFile = args.size() == 4 ? args[3].resolveAsFile() : juce::File();
    if(argumentsFile != juce::File() && !argumentsFile.existsAsFile())
    {
        fail("Could not find file:" + argumentsFile.getFullPathName());
    }

    std::atomic<bool> const shouldAbort{false};
    std::atomic<float> advancement{0.0f};
    Track::FileInfo expectedTrackInfo;
    expectedTrackInfo.file = expectedFile;
    Track::FileInfo generatedTrackInfo;
    generatedTrackInfo.file = generatedFile;
    if(argumentsFile != juce::File{})
    {
        auto xml = juce::XmlDocument::parse(argumentsFile);
        if(xml == nullptr)
        {
            fail("Cannot parse arguments!");
        }
        auto const xmlArgs = XmlParser::fromXml(*xml.get(), "args", juce::StringPairArray{});
        generatedTrackInfo.args = xmlArgs;
        expectedTrackInfo.args = xmlArgs;
    }

    auto const expectedResults = Track::Loader::loadFromFile(expectedTrackInfo, shouldAbort, advancement);
    auto const generatedResults = Track::Loader::loadFromFile(generatedTrackInfo, shouldAbort, advancement);
    if(expectedResults.index() == 1_z)
    {
        fail(*std::get_if<juce::String>(&expectedResults));
    }
    if(generatedResults.index() == 1_z)
    {
        fail(*std::get_if<juce::String>(&generatedResults));
    }
    auto const expectedTrackResults = *std::get_if<Track::Results>(&expectedResults);
    auto const expectedAccess = expectedTrackResults.getReadAccess();
    if(!static_cast<bool>(expectedAccess))
    {
        fail("Cannot get access to expected results!");
    }
    auto const generatedTrackResults = *std::get_if<Track::Results>(&generatedResults);
    auto const generatedAccess = generatedTrackResults.getReadAccess();
    if(!static_cast<bool>(generatedAccess))
    {
        fail("Cannot get access to expected results!");
    }
    if(!expectedTrackResults.matchWithEpsilon(generatedTrackResults, 0.00001, 0.01f))
    {
        fail("Different results!");
    }
}

std::unique_ptr<Application::CommandLine> Application::CommandLine::createAndRun(juce::String const& commandLine)
{
    if(commandLine.isEmpty() || commandLine.startsWith("-NSDocumentRevisionsDebugMode"))
    {
        anlDebug("Application", "Command line is empty or contains '-NSDocumentRevisionsDebugMode'");
        return nullptr;
    }

    juce::ArgumentList const args("Partiels", commandLine);
    if(!args[0].isLongOption() && !args[0].isShortOption())
    {
        anlDebug("Application", "Command line doesn't contains any option");
        return nullptr;
    }

    auto cli = std::make_unique<CommandLine>();
    if(cli == nullptr)
    {
        anlDebug("Application", "Command line allocation failed!");
        return nullptr;
    }

#if JUCE_MAC
    juce::Process::setDockIconVisible(false);
#endif

    if(args[0].isLongOption("unit-tests"))
    {
        anlDebug("Application", "Running as CLI - Unit Tests");
        auto const result = cli->invokeCatchingFailures([&]()
                                                        {
                                                            cli->runUnitTests();
                                                            return 0;
                                                        });
        sendQuitSignal(result);
    }
    else if(args[0].isLongOption("compare-files"))
    {
        anlDebug("Application", "Running as CLI - Compare Files");
        auto const result = cli->invokeCatchingFailures([&]()
                                                        {
                                                            cli->compareFiles(args);
                                                            return 0;
                                                        });
        sendQuitSignal(result);
    }
    else
    {
        anlDebug("Application", "Running as CLI - Default");
        auto const result = cli->findAndRunCommand(args);
        if(!cli->isRunning())
        {
            sendQuitSignal(result);
        }
    }
    return cli;
}

ANALYSE_FILE_END
