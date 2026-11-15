#include "AnlApplicationCommandLine.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::CommandLine::CommandLine()
: mInitialVampPath(juce::SystemStats::getEnvironmentVariable("VAMP_PATH", {}))
{
    PluginList::Accessor pluginListAcsr;
    pluginListAcsr.setAttr<PluginList::AttrType::useEnvVariable>(true, NotificationType::synchronous);
    pluginListAcsr.setAttr<PluginList::AttrType::quarantineMode>(PluginList::QuarantineMode::ignore, NotificationType::synchronous);
    PluginList::setEnvironment(pluginListAcsr, true);

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
             MiscDebug("CommandLine", "Parsing arguments...");
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
         "--document|-d <document> Defines the path to the document to analyze (required if --input/template are undefined).\n\t"
         "--input|-i <audiofile> Defines the path to the audio file to analyze (required if --template is defined).\n\t"
         "--template|-t <templatefile> Defines the path to the template file (required if --input is defined).\n\t"
         "--output|-o <outputdirectory> Defines the path of the output folder (required).\n\t"
         "--format|-f <formatname> Defines the export format (jpeg, png, csv, lab, json, cue, reaper, puredata, max or sdif) (required).\n\t"
         "--width <width> Defines the width of the exported image in pixels (required with the jpeg and png formats).\n\t"
         "--height <height> Defines the height of the exported image in pixels (required with the jpeg and png formats).\n\t"
         "--ppi <ppi> Defines the pixel density of the exported image in pixels per inch (optional with the jpeg and png formats - default 72).\n\t"
         "--adapt Defines if the block size and the step size of the analyzes are adapted following the sample rate (optional if --input/template are defined).\n\t"
         "--groups Exports the images of group and not the image of the tracks (optional with the jpeg and png formats).\n\t"
         "--nogrids Ignores the export of the grid tracks (optional with the csv, json or cue formats).\n\t"
         "--header Defines the CSV header format ('none', 'generic', 'specific') (optional with the csv format, default is 'none').\n\t"
         "--separator <character> Defines the separator character between columns (optional with the csv and lab formats, default is ',' for csv and ' ' for lab).\n\t"
         "--noescape Disables escaping of special characters in labels (optional with the csv and lab formats).\n\t"
         "--reapertype <type> Defines the type of the reaper format  (optional with the reaper format 'marker' or 'region', default is 'region').\n\t"
         "--description Includes the plugin description (optional with the json format).\n\t"
         "--thresholds Applies extra thresholds filtering to the exported results (optional with the csv, lab, json, cue, and reaper formats).\n\t"
         "--frame <framesignature> Defines the 4 characters frame signature (required with the sdif format).\n\t"
         "--matrix <matrixsignature> Defines the 4 characters matrix signature (required with the sdif format).\n\t"
         "--colname <string> Defines the name of the column (optional with the sdif format).",
         "",
         [this](juce::ArgumentList const& args)
         {
             mShouldWait = false;
             MiscDebug("CommandLine", "Parsing arguments...");
             using Options = Document::Exporter::Options;

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

             Options options;
             bool useGroupOverview = false;
             bool ignoreGridResults = false;
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
                 fail("Format not specified! Available formats are jpeg, png, csv, lab, json, cue, reaper, puredata, max or sdif.");
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
                 options.imagePpi = args.containsOption("--ppi") ? args.getValueForOption("--ppi").getIntValue() : 72;
                 useGroupOverview = args.containsOption("--groups");
             }
             else if(format == "csv" || format == "lab" || format == "json" || format == "cue" || format == "reaper" || format == "sdif" || format == "puredata" || format == "max")
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
                 else if(format == "puredata")
                 {
                     options.format = Options::Format::puredata;
                 }
                 else if(format == "max")
                 {
                     options.format = Options::Format::max;
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
                 ignoreGridResults = args.containsOption("--nogrids");
                 if(args.containsOption("--header"))
                 {
                     auto const header = args.getValueForOption("--header");
                     auto const csvHeader = magic_enum::enum_cast<Document::Exporter::Options::CsvHeaderType>(header.toStdString());
                     if(csvHeader.has_value())
                     {
                         options.csvHeaderType = csvHeader.value();
                     }
                     else
                     {
                         fail("Header '" + header + "' unsupported! Available headers are none, generic, or specific.");
                     }
                 }
                 else
                 {
                     options.csvHeaderType = Document::Exporter::Options::CsvHeaderType::none;
                 }
                 options.includeDescription = args.containsOption("--description");
                 options.applyExtraThresholds = args.containsOption("--thresholds");
                 options.disableLabelEscaping = args.containsOption("--noescape");
                 options.reaperType = args.getValueForOption("--reapertype").toLowerCase() == "marker" ? Options::ReaperType::marker : Options::ReaperType::region;
                 options.sdifFrameSignature = args.getValueForOption("--frame").removeCharacters("\"").toUpperCase();
                 options.sdifMatrixSignature = args.getValueForOption("--matrix").removeCharacters("\"").toUpperCase();
                 options.sdifColumnName = args.getValueForOption("--colname");
                 auto const separator = magic_enum::enum_cast<Document::Exporter::Options::ColumnSeparator>(args.getValueForOption("--separator").toStdString());
                 if(separator.has_value())
                 {
                     options.columnSeparator = *separator;
                     options.labSeparator = *separator;
                 }
             }
             else
             {
                 fail("Format '" + format + "' unsupported! Available formats are jpeg, png, csv, lab, json, cue, reaper, puredata, max or sdif.");
             }

             options.useAutoSize = false;

             mExecutor = std::make_unique<Document::Executor>();
             mExecutor->onEnded = [=, this]()
             {
                 LookAndFeel lookAndFeel;
                 juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
                 auto const result = mExecutor->exportTo(outputDir, "", options, useGroupOverview, ignoreGridResults);
                 mShouldWait = false;
                 juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
                 // The method fail() cannot be used because the exception is not caught outside the command
                 if(result.failed())
                 {
                     std::cerr << result.getErrorMessage() << std::endl;
                     sendQuitSignal(1);
                     return;
                 }
                 sendQuitSignal(0);
             };

             mShouldWait = true;
             if(args.containsOption("-d|--document"))
             {
                 auto const documentFile = args.getExistingFileForOption("-d|--document");
                 auto const result = mExecutor->load(documentFile);
                 if(result.failed())
                 {
                     mShouldWait = false;
                     fail(result.getErrorMessage());
                 }
             }
             else
             {
                 auto const audioFile = args.getExistingFileForOption("-i|--input");
                 auto const templateFile = args.getExistingFileForOption("-t|--template");
                 auto const adaptToSampleRate = args.containsOption("--adapt");
                 auto const result = mExecutor->load(audioFile, templateFile, adaptToSampleRate);
                 if(result.failed())
                 {
                     mShouldWait = false;
                     fail(result.getErrorMessage());
                 }
             }
             auto const result = mExecutor->launch();
             if(result.failed())
             {
                 mShouldWait = false;
                 fail(result.getErrorMessage());
             }
         }});
    addCommand(
        {"--plugin-list",
         "--plugin-list [options]",
         "Prints an exhaustive list of installed plugins.\n\t"
         "--description Includes the descriptions of the plugins (optional).\n\t"
         "--format <fileformat> Defines the output format (json or xml) (optional - default: json).\n\t"
         "--vamp-path-only Ignores the default plugin directories of the system and only uses the directories defined by the VAMP_PATH environment variable and the plugins embedded in Partiels (optional).",
         "",
         [this](juce::ArgumentList const& args)
         {
             auto const includeDescription = args.containsOption("--description");
             auto const format = args.containsOption("--format") ? args.getValueForOption("--format").toLowerCase() : "json";
             if(format != "json" && format != "xml")
             {
                 fail("Invalid value for '--format'. Accepted values are 'json' or 'xml'. Use '--help' for more information.");
             }
             if(args.containsOption("--vamp-path-only"))
             {
#if JUCE_WINDOWS
                 auto constexpr separator = ";";
#else
                 auto constexpr separator = ":";
#endif
                 std::vector<juce::File> searchPath;
                 for(auto const& path : juce::StringArray::fromTokens(mInitialVampPath, separator, ""))
                 {
                     if(path.isNotEmpty())
                     {
                         searchPath.push_back(juce::File::getCurrentWorkingDirectory().getChildFile(path));
                     }
                 }
                 PluginList::Accessor scopedPluginListAcsr;
                 scopedPluginListAcsr.setAttr<PluginList::AttrType::useEnvVariable>(false, NotificationType::synchronous);
                 scopedPluginListAcsr.setAttr<PluginList::AttrType::quarantineMode>(PluginList::QuarantineMode::ignore, NotificationType::synchronous);
                 scopedPluginListAcsr.setAttr<PluginList::AttrType::searchPath>(searchPath, NotificationType::synchronous);
                 PluginList::setEnvironment(scopedPluginListAcsr, true);
             }
             PluginList::Scanner scanner;
             auto const plugins = scanner.getPlugins(48000.0, false);
             if(format == "xml")
             {
                 juce::XmlElement xml("plugins");
                 for(auto const& plugin : std::get<0>(plugins))
                 {
                     auto pluginXml = std::make_unique<juce::XmlElement>("plugin");
                     XmlParser::toXml(*pluginXml, "key", plugin.first);
                     if(includeDescription)
                     {
                         XmlParser::toXml(*pluginXml, "description", plugin.second);
                     }
                     xml.addChildElement(pluginXml.release());
                 }
                 std::cout << xml.toString() << std::endl;
             }
             else
             {
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
             }
         }});
}

Application::CommandLine::~CommandLine()
{
    MiscWeakAssert(!isRunning());
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
    if(args.size() < 3)
    {
        fail("Missing arguments! Expected two result files!");
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

    std::atomic<bool> const shouldAbort{false};
    std::atomic<float> advancement{0.0f};
    auto const expectedDescription = Track::Loader::getFileDescription(expectedFile, 44100.0);
    if(std::get<0_z>(expectedDescription).failed())
    {
        fail(std::get<0_z>(expectedDescription).getErrorMessage());
    }
    auto const generatedDescription = Track::Loader::getFileDescription(generatedFile, 44100.0);
    if(std::get<0_z>(generatedDescription).failed())
    {
        fail(std::get<0_z>(generatedDescription).getErrorMessage());
    }
    auto const expectedResults = Track::Loader::loadFromFile(std::get<1_z>(expectedDescription), shouldAbort, advancement);
    auto const generatedResults = Track::Loader::loadFromFile(std::get<1_z>(generatedDescription), shouldAbort, advancement);
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
        MiscDebug("Application", "Command line is empty or contains '-NSDocumentRevisionsDebugMode'");
        return nullptr;
    }

    juce::ArgumentList const args("Partiels", commandLine);
    if(!args[0].isLongOption() && !args[0].isShortOption())
    {
        MiscDebug("Application", "Command line doesn't contains any option");
        return nullptr;
    }

    auto cli = std::make_unique<CommandLine>();

#if JUCE_MAC
    juce::Process::setDockIconVisible(false);
#endif

    if(args[0].isLongOption("unit-tests"))
    {
        MiscDebug("Application", "Running as CLI - Unit Tests");
        auto const result = cli->invokeCatchingFailures([&]()
                                                        {
                                                            cli->runUnitTests();
                                                            return 0;
                                                        });
        sendQuitSignal(result);
    }
    else if(args[0].isLongOption("compare-files"))
    {
        MiscDebug("Application", "Running as CLI - Compare Files");
        auto const result = cli->invokeCatchingFailures([&]()
                                                        {
                                                            cli->compareFiles(args);
                                                            return 0;
                                                        });
        sendQuitSignal(result);
    }
    else
    {
        MiscDebug("Application", "Running as CLI - Default");
        auto const result = cli->findAndRunCommand(args);
        if(!cli->isRunning())
        {
            sendQuitSignal(result);
        }
    }
    return cli;
}

ANALYSE_FILE_END
