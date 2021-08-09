#include "AnlSdifConverter.h"
#include "AnlAlertWindow.h"
#include <sdif.h>

ANALYSE_FILE_BEGIN

static int sdifOpenFileQueryCallback(SdifFileT* file, void* userdata)
{
    juce::ignoreUnused(file, userdata);
    return 2;
}

std::map<uint32_t, std::set<uint32_t>> SdifConverter::getSignatures(juce::File const& inputFile)
{
    if(!inputFile.existsAsFile())
    {
        return {};
    }
    SdifGenInit(nullptr);
    auto* sigs = SdifCreateQueryTree(1024);
    SdifQuery(inputFile.getFullPathName().toRawUTF8(), sdifOpenFileQueryCallback, sigs);
    std::map<uint32_t, std::set<uint32_t>> signatures;
    for(int i = 0; i < sigs->num; i++)
    {
        if(sigs->elems[i].parent == -1)
        {
            auto& frameRef = signatures[sigs->elems[i].sig];
            for(int m = 0; m < sigs->num; m++)
            {
                if(sigs->elems[m].parent == i)
                {
                    frameRef.insert(sigs->elems[m].sig);
                }
            }
        }
    }

    SdifFreeQueryTree(sigs);
    SdifGenKill();
    return signatures;
}

juce::Result SdifConverter::toJson(juce::File const& inputFile, juce::File const& outputFile, uint32_t frameId, uint32_t matrixId)
{
    class ScopedFile
    {
    public:
        ScopedFile(SdifFileT** f, const char* path)
        {
            SdifGenInit(nullptr);
            file = *f = SdifFOpen(path, eReadFile);
        }
        ~ScopedFile()
        {
            if(file != nullptr)
            {
                SdifFClose(file);
            }
            SdifGenKill();
        }

    private:
        SdifFileT* file = nullptr;
    };

    SdifFileT* file;
    ScopedFile scopedFile(&file, inputFile.getFullPathName().toRawUTF8());
    if(file == nullptr)
    {
        return juce::Result::fail("Can't open input file");
    }

    auto container = nlohmann::json::object();
    auto& json = container["results"];

    int endOfFile = 0;
    size_t bytesRead = 0;

    {
        auto const numBytes = SdifFReadGeneralHeader(file);
        if(numBytes == 0_z)
        {
            return juce::Result::fail("Can't read header info of input file");
        }
        bytesRead += numBytes;
    }
    {
        auto const numBytes = SdifFReadAllASCIIChunks(file);
        if(numBytes == 0)
        {
            return juce::Result::fail("Can't read ASCII header info of input file");
        }
        bytesRead += numBytes;
    }

    while(!endOfFile && SdifFLastError(file) == nullptr)
    {
        bytesRead += SdifFReadFrameHeader(file);
        while(!SdifFCurrFrameIsSelected(file) || SdifFCurrSignature(file) != frameId)
        {
            SdifFSkipFrameData(file);
            endOfFile = SdifFGetSignature(file, &bytesRead);
            if(endOfFile == eEof)
            {
                break;
            }
            bytesRead += SdifFReadFrameHeader(file);
        }

        if(SdifFCurrFrameSignature(file) != frameId)
        {
            return juce::Result::fail("Can't find frame signature");
        }

        if(!endOfFile)
        {
            auto const time = SdifFCurrTime(file);
            auto const numMatrix = SdifFCurrNbMatrix(file);
            for(SdifUInt4 m = 0; m < numMatrix; m++)
            {
                bytesRead += SdifFReadMatrixHeader(file);
                if(SdifFCurrMatrixIsSelected(file) && SdifFCurrMatrixSignature(file) == matrixId)
                {
                    auto const numRows = SdifFCurrNbRow(file);
                    auto const numColumns = SdifFCurrNbCol(file);
                    for(SdifUInt4 row = 0; row < numRows; row++)
                    {
                        auto& cjson = json[row];
                        nlohmann::json vjson;
                        vjson["time"] = time;
                        bytesRead += SdifFReadOneRow(file);
                        if(numColumns == 1)
                        {
                            vjson["value"] = SdifFCurrOneRowCol(file, 1);
                        }
                        else
                        {
                            for(SdifUInt4 col = 1; col <= numColumns; col++)
                            {
                                auto const value = SdifFCurrOneRowCol(file, col);
                                vjson["values"].push_back(value);
                            }
                        }

                        cjson.push_back(std::move(vjson));
                    }
                }
                else
                {
                    bytesRead += SdifFSkipMatrixData(file);
                }

                bytesRead += SdifFReadPadding(file, SdifFPaddingCalculate(file->Stream, bytesRead));
            }

            endOfFile = SdifFGetSignature(file, &bytesRead) == eEof;
        }
    }

    auto* error = SdifFLastError(file);
    if(error != nullptr)
    {
        return juce::Result::fail(error->UserMess != nullptr ? error->UserMess : "");
    }

    juce::TemporaryFile temp(outputFile);
    std::ofstream stream(temp.getFile().getFullPathName().toStdString());
    if(!stream.is_open() || !stream.good())
    {
        return juce::Result::fail(juce::translate("Can't open the output stream"));
    }
    stream << container << std::endl;
    stream.close();
    if(!stream.good())
    {
        return juce::Result::fail(juce::translate("Can't writing to the output stream"));
    }

    if(!temp.overwriteTargetFileWithTemporary())
    {
        return juce::Result::fail(juce::translate("Can't write to the output file"));
    }

    return juce::Result::ok();
}

SdifConverter::Panel::Panel()
: FloatingWindowContainer("SDIF to JSON", *this)
, mPropertyOpen("Open", "Select an SDIF file to convert", [&]()
                {
                    juce::FileChooser fc("Load a SDIF file", {}, "*.sdif");
                    if(fc.browseForFileToOpen())
                    {
                        setFile(fc.getResult());
                    }
                })
, mPropertyName("SDIF File", "The SDIF file to convert", nullptr)
, mPropertyFrameId("Frame ID", "Select an frame identifier to convert", "", {}, [&](size_t index)
                   {
                       juce::ignoreUnused(index);
                       selectedFrameUpdated();
                   })
, mPropertyMatrixId("Matrix ID", "Select an matrix identifier to convert", "", {}, nullptr)
, mPropertyExport("Convert", "Convert the SDIF file to a JSON file", [&]()
                  {
                      exportFile();
                  })
{
    mPropertyName.entry.setEnabled(false);
    addAndMakeVisible(mPropertyName);
    addAndMakeVisible(mPropertyOpen);
    addAndMakeVisible(mPropertyFrameId);
    addAndMakeVisible(mPropertyMatrixId);
    addAndMakeVisible(mPropertyExport);
    mPropertyFrameId.entry.setTextWhenNoChoicesAvailable(juce::translate("No frame available"));
    mPropertyMatrixId.entry.setTextWhenNoChoicesAvailable(juce::translate("No matrix available"));
    setFile({});
    setSize(300, 200);
}

void SdifConverter::Panel::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyOpen);
    setBounds(mPropertyName);
    setBounds(mPropertyFrameId);
    setBounds(mPropertyMatrixId);
    setBounds(mPropertyExport);
    setSize(300, std::max(bounds.getY(), 120) + 2);
}

void SdifConverter::Panel::paintOverChildren(juce::Graphics& g)
{
    if(mFileIsDragging)
    {
        g.fillAll(findColour(juce::TextButton::ColourIds::buttonColourId).withAlpha(0.5f));
    }
}

bool SdifConverter::Panel::isInterestedInFileDrag(juce::StringArray const& files)
{
    return std::any_of(files.begin(), files.end(), [](auto const& path)
                       {
                           return juce::File(path).hasFileExtension("sdif");
                       });
}

void SdifConverter::Panel::fileDragEnter(juce::StringArray const& files, int x, int y)
{
    juce::ignoreUnused(files, x, y);
    mFileIsDragging = true;
    repaint();
}

void SdifConverter::Panel::fileDragExit(juce::StringArray const& files)
{
    juce::ignoreUnused(files);
    mFileIsDragging = false;
    repaint();
}

void SdifConverter::Panel::filesDropped(juce::StringArray const& files, int x, int y)
{
    juce::ignoreUnused(files, x, y);
    mFileIsDragging = false;
    repaint();
    for(auto const& path : files)
    {
        juce::File const file(path);
        if(file.hasFileExtension("sdif"))
        {
            setFile(file);
            return;
        }
    }
}

void SdifConverter::Panel::setFile(juce::File const& file)
{
    mFile = file;
    if(mFile == juce::File{})
    {
        mPropertyName.entry.setText("No SDIF file selected", juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyName.entry.setText(file.getFileNameWithoutExtension(), juce::NotificationType::dontSendNotification);
    }
    mSignatures = getSignatures(file);

    mPropertyFrameId.entry.clear(juce::NotificationType::dontSendNotification);
    mFrameSigLinks.clear();
    for(auto const& signature : mSignatures)
    {
        mFrameSigLinks.push_back(signature.first);
        mPropertyFrameId.entry.addItem(SdifSignatureToString(signature.first), static_cast<int>(mFrameSigLinks.size()));
    }
    mPropertyFrameId.entry.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
    mPropertyFrameId.entry.setEnabled(mPropertyFrameId.entry.getNumItems() > 0);
    selectedFrameUpdated();
}

void SdifConverter::Panel::selectedFrameUpdated()
{
    mPropertyMatrixId.entry.clear(juce::NotificationType::dontSendNotification);
    mMatrixSigLinks.clear();
    auto const selectedFrameIndex = mPropertyFrameId.entry.getSelectedItemIndex();
    if(selectedFrameIndex < 0 || static_cast<size_t>(selectedFrameIndex) > mFrameSigLinks.size())
    {
        mPropertyMatrixId.entry.setEnabled(false);
        mPropertyExport.entry.setEnabled(false);
        return;
    }
    auto const frameIdentifier = mFrameSigLinks[static_cast<size_t>(selectedFrameIndex)];
    if(mSignatures.count(frameIdentifier) == 0_z)
    {
        mPropertyMatrixId.entry.setEnabled(false);
        mPropertyExport.entry.setEnabled(false);
        return;
    }
    auto const& matrixSignatures = mSignatures.at(frameIdentifier);
    for(auto const& signature : matrixSignatures)
    {
        mMatrixSigLinks.push_back(signature);
        mPropertyMatrixId.entry.addItem(SdifSignatureToString(signature), static_cast<int>(mMatrixSigLinks.size()));
    }
    mPropertyMatrixId.entry.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
    mPropertyMatrixId.entry.setEnabled(mPropertyMatrixId.entry.getNumItems() > 0);
    mPropertyExport.entry.setEnabled(mPropertyMatrixId.entry.getNumItems() > 0);
}

void SdifConverter::Panel::exportFile()
{
    auto const selectedFrameIndex = mPropertyFrameId.entry.getSelectedItemIndex();
    if(selectedFrameIndex < 0 || static_cast<size_t>(selectedFrameIndex) > mFrameSigLinks.size())
    {
        anlWeakAssert(false);
        return;
    }
    auto const frameIdentifier = mFrameSigLinks[static_cast<size_t>(selectedFrameIndex)];
    if(mSignatures.count(frameIdentifier) == 0_z)
    {
        anlWeakAssert(false);
        return;
    }

    auto const selectedMatrixIndex = mPropertyMatrixId.entry.getSelectedItemIndex();
    if(selectedMatrixIndex < 0 || static_cast<size_t>(selectedMatrixIndex) > mMatrixSigLinks.size())
    {
        anlWeakAssert(false);
        return;
    }
    auto const matrixIdentifier = mMatrixSigLinks[static_cast<size_t>(selectedMatrixIndex)];

    juce::FileChooser fc("Select a JSON file", {}, "*.json");
    if(!fc.browseForFileToSave(true))
    {
        return;
    }
    auto const jsonFile = fc.getResult();
    enterModalState();
    auto const result = toJson(mFile, jsonFile, frameIdentifier, matrixIdentifier);
    exitModalState(0);
    if(result.wasOk())
    {
        AlertWindow::showMessage(AlertWindow::MessageType::info, juce::translate("Convert succeeded"), juce::translate("The SDIF file 'SDIFFLNM' has been successfully converted to the JSON file 'JSONFLNM'.").replace("SDIFFLNM", mFile.getFullPathName()).replace("JSONFLNM", jsonFile.getFullPathName()) + "\n\n" + result.getErrorMessage());
    }
    else
    {
        AlertWindow::showMessage(AlertWindow::MessageType::warning, juce::translate("Failed to convert SDIF file..."), juce::translate("There was an error while trying to convert the SDIF file 'SDIFFLNM' to the JSON file 'JSONFLNM'.").replace("SDIFFLNM", mFile.getFullPathName()).replace("JSONFLNM", jsonFile.getFullPathName()) + "\n\n" + result.getErrorMessage());
    }
}

ANALYSE_FILE_END
