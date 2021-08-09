#include "AnlSdifConverter.h"
#include "AnlAlertWindow.h"
#include <sdif.h>

ANALYSE_FILE_BEGIN

uint32_t SdifConverter::getSignature(juce::String const& name)
{
    if(name.length() != 4)
    {
        return 0;
    }
    return SdifSignatureConst(name[0], name[1], name[2], name[3]);
}

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

juce::Result SdifConverter::toJson(juce::File const& inputFile, juce::File const& outputFile, uint32_t frameId, uint32_t matrixId, size_t row)
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
        
        void clear()
        {
            if(file != nullptr)
            {
                SdifFClose(file);
                file = nullptr;
            }
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
            auto const channelIndex = SdifFCurrID(file);
            auto const numMatrix = SdifFCurrNbMatrix(file);
            for(SdifUInt4 m = 0; m < numMatrix; m++)
            {
                bytesRead += SdifFReadMatrixHeader(file);
                if(SdifFCurrMatrixIsSelected(file) && SdifFCurrMatrixSignature(file) == matrixId)
                {
                    auto const numRows = SdifFCurrNbRow(file);
                    auto const numColumns = SdifFCurrNbCol(file);
                    if(row < numRows)
                    {
                        auto rowIndex = 0_z;
                        while(rowIndex < row)
                        {
                            bytesRead += SdifFReadOneRow(file);
                        }
                        auto& cjson = json[channelIndex];
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
                    else
                    {
                        auto rowIndex = 0_z;
                        while(rowIndex < numRows)
                        {
                            bytesRead += SdifFReadOneRow(file);
                        }
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
    
    scopedFile.clear();

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

juce::Result SdifConverter::fromJson(juce::File const& inputFile, juce::File const& outputFile, uint32_t frameId, uint32_t matrixId)
{
    class ScopedFile
    {
    public:
        ScopedFile(SdifFileT** f, const char* path)
        {
            SdifGenInit(nullptr);
            file = *f = SdifFOpen(path, eWriteFile);
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

    std::ifstream inputStream(inputFile.getFullPathName().toStdString());
    if(!inputStream || !inputStream.is_open() || !inputStream.good())
    {
        return juce::Result::fail(juce::translate("The input stream of cannot be opened"));
    }

    nlohmann::basic_json container;
    try
    {
        container = nlohmann::json::parse(inputStream);
    }
    catch(nlohmann::json::parse_error& e)
    {
        inputStream.close();
        return juce::Result::fail(juce::translate(e.what()));
    }
    inputStream.close();

    if(container.is_discarded())
    {
        return juce::Result::fail(juce::translate("Parsing error"));
    }

    SdifFileT* file;
    juce::TemporaryFile temp(outputFile);
    if(temp.getFile().create().failed())
    {
        return juce::Result::fail("Can't create temporary input file");
    }
    ScopedFile scopedFile(&file, temp.getFile().getFullPathName().toRawUTF8());
    if(file == nullptr)
    {
        return juce::Result::fail("Can't open input file");
    }

    SdifFWriteGeneralHeader(file);
    SdifFWriteAllASCIIChunks(file);

    auto const& json = container.count("results") ? container.at("results") : container;
    for(size_t channelIndex = 0_z; channelIndex < json.size(); ++channelIndex)
    {
        auto const& channelData = json[channelIndex];
        for(size_t frameIndex = 0_z; frameIndex < channelData.size(); ++frameIndex)
        {
            auto const& frameData = channelData[frameIndex];

            auto const timeIt = frameData.find("time");
            auto const time = timeIt != frameData.cend() ? static_cast<double>(timeIt.value()) : 0.0;

            auto const labelIt = frameData.find("label");
            if(labelIt != frameData.cend())
            {
                std::string label = labelIt.value();
                std::vector<SdifChar> data;
                data.resize(label.length());
                strncpy(data.data(), label.c_str(), data.size());
                SdifFWriteFrameAndOneMatrix(file, frameId, static_cast<SdifUInt4>(channelIndex), static_cast<SdifFloat8>(time), matrixId, SdifDataTypeET::eChar, static_cast<SdifUInt4>(1), static_cast<SdifUInt4>(data.size()), reinterpret_cast<void*>(data.data()));
            }
            else
            {
                auto const valueIt = frameData.find("value");
                if(valueIt != frameData.cend())
                {
                    SdifFloat8 value = valueIt.value();
                    SdifFWriteFrameAndOneMatrix(file, frameId, static_cast<SdifUInt4>(channelIndex), static_cast<SdifFloat8>(time), matrixId, SdifDataTypeET::eFloat8, static_cast<SdifUInt4>(1), static_cast<SdifUInt4>(1), reinterpret_cast<void*>(&value));
                }
                else
                {
                    auto const valuesIt = frameData.find("values");
                    if(valuesIt != frameData.cend())
                    {
                        std::vector<SdifFloat8> values;
                        values.resize(valuesIt->size());
                        for(size_t binIndex = 0_z; binIndex < valuesIt->size(); ++binIndex)
                        {
                            values[binIndex] = valuesIt->at(binIndex);
                        }
                        SdifFWriteFrameAndOneMatrix(file, frameId, static_cast<SdifUInt4>(channelIndex), static_cast<SdifFloat8>(time), matrixId, SdifDataTypeET::eFloat8, static_cast<SdifUInt4>(1), static_cast<SdifUInt4>(values.size()), reinterpret_cast<void*>(values.data()));
                    }
                }
            }
        }
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
    auto const result = toJson(mFile, jsonFile, frameIdentifier, matrixIdentifier, 0_z);
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
