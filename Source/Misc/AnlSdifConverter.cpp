#include "AnlSdifConverter.h"
#include "AnlAlertWindow.h"
#include <magic_enum/magic_enum.hpp>
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

static void sdifDummyExit(void)
{
}

static juce::String sdifWarning;
static void sdifPrintWarning(SdifErrorTagET tag, SdifErrorLevelET level, char* message, SdifFileT*, SdifErrorT* error, char*, int)
{
    auto const tagName = juce::String(std::string(magic_enum::enum_name(tag).substr()));
    auto const levelName = juce::String(std::string(magic_enum::enum_name(level).substr()));
    auto const extra = (error != nullptr && error->UserMess != nullptr) ? (juce::String(" - ") + juce::String(error->UserMess)) : juce::String();
    sdifWarning = tagName + juce::String(" (") + levelName + juce::String("): ") + juce::String(message) + extra;
}

std::map<uint32_t, std::map<uint32_t, std::pair<size_t, size_t>>> SdifConverter::getEntries(juce::File const& inputFile)
{
    if(!inputFile.existsAsFile())
    {
        return {};
    }
    SdifGenInit(nullptr);
    SdifSetExitFunc(sdifDummyExit);
    auto* sigs = SdifCreateQueryTree(1024);
    SdifQuery(inputFile.getFullPathName().toRawUTF8(), sdifOpenFileQueryCallback, sigs);
    std::map<uint32_t, std::map<uint32_t, matrix_size_t>> entries;
    for(int i = 0; i < sigs->num; i++)
    {
        if(sigs->elems[i].parent == -1)
        {
            auto const frameSig = sigs->elems[i].sig;
            auto& frameRef = entries[frameSig];
            for(int m = 0; m < sigs->num; m++)
            {
                if(sigs->elems[m].parent == i)
                {
                    auto const matrixSig = sigs->elems[m].sig;
                    auto const numRows = frameRef.count(matrixSig) > 0_z ? frameRef.at(matrixSig).first : 0_z;
                    auto const numCols = frameRef.count(matrixSig) > 0_z ? frameRef.at(matrixSig).second : 0_z;
                    auto const currNumRows = sigs->elems[m].nrow.max > 0.0 ? static_cast<size_t>(sigs->elems[m].nrow.max) : 0_z;
                    auto const currNumCols = sigs->elems[m].ncol.max > 0.0 ? static_cast<size_t>(sigs->elems[m].ncol.max) : 0_z;
                    frameRef[matrixSig].first = std::max(currNumRows, numRows);
                    frameRef[matrixSig].second = std::max(currNumCols, numCols);
                }
            }
        }
    }

    SdifFreeQueryTree(sigs);
    SdifGenKill();
    return entries;
}

juce::Result SdifConverter::toJson(juce::File const& inputFile, juce::File const& outputFile, uint32_t frameId, uint32_t matrixId, size_t row, std::optional<size_t> column)
{
    class ScopedFile
    {
    public:
        ScopedFile(SdifFileT** f, const char* path)
        {
            sdifWarning.clear();
            SdifGenInit(nullptr);
            SdifSetExitFunc(sdifDummyExit);
            SdifSetWarningFunc(sdifPrintWarning);
            SdifSetErrorFunc(sdifPrintWarning);
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

    auto container = nlohmann::json::object();
    auto& json = container["results"];

    {
        SdifFileT* file;
        ScopedFile scopedFile(&file, inputFile.getFullPathName().toRawUTF8());
        if(file == nullptr)
        {
            return juce::Result::fail("Can't open input file");
        }

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
                            if(numColumns == 1 || column.has_value())
                            {
                                auto const columnIndex = column.has_value() ? static_cast<SdifUInt4>(*column) + static_cast<SdifUInt4>(1) : static_cast<SdifUInt4>(1);
                                if(columnIndex <= numColumns)
                                {
                                    vjson["value"] = SdifFCurrOneRowCol(file, 1);
                                }
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

        if(sdifWarning.isNotEmpty())
        {
            return juce::Result::fail(sdifWarning);
        }
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

juce::Result SdifConverter::fromJson(juce::File const& inputFile, juce::File const& outputFile, uint32_t frameId, uint32_t matrixId)
{
    class ScopedFile
    {
    public:
        ScopedFile(SdifFileT** f, const char* path)
        {
            sdifWarning.clear();
            SdifGenInit(nullptr);
            SdifSetExitFunc(sdifDummyExit);
            SdifSetWarningFunc(sdifPrintWarning);
            SdifSetErrorFunc(sdifPrintWarning);
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

    juce::TemporaryFile temp(outputFile);
    {
        SdifFileT* file;
        if(temp.getFile().create().failed())
        {
            return juce::Result::fail("Can't create temporary input file");
        }
        ScopedFile scopedFile(&file, temp.getFile().getFullPathName().toRawUTF8());
        if(file == nullptr)
        {
            return juce::Result::fail("Can't open input file");
        }

        if(SdifGetMatrixType(gSdifPredefinedTypes->MatrixTypesTable, matrixId) == nullptr)
        {
            auto* newMatrixType = SdifCreateMatrixType(matrixId, nullptr);
            if(newMatrixType == nullptr)
            {
                return juce::Result::fail(juce::translate("Can't create new matrix type"));
            }
            SdifPutMatrixType(file->MatrixTypesTable, newMatrixType);
        }

        if(SdifTestMatrixType(file, matrixId) == nullptr)
        {
            return juce::Result::fail("Matrix type undefined");
        }

        if(SdifGetFrameType(gSdifPredefinedTypes->FrameTypesTable, frameId) == nullptr)
        {
            auto* newFrameType = SdifCreateFrameType(frameId, nullptr);
            if(newFrameType == nullptr)
            {
                return juce::Result::fail(juce::translate("Can't create new frame type"));
            }
            SdifPutFrameType(file->FrameTypesTable, newFrameType);
            char name[] = "MAT0";
            newFrameType = SdifFrameTypePutComponent(newFrameType, matrixId, name);
        }

        if(SdifTestFrameType(file, frameId) == nullptr)
        {
            return juce::Result::fail("Frame type undefined");
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
        if(sdifWarning.isNotEmpty())
        {
            return juce::Result::fail(sdifWarning);
        }
    }

    if(!temp.overwriteTargetFileWithTemporary())
    {
        return juce::Result::fail(juce::translate("Can't write to the output file"));
    }

    return juce::Result::ok();
}

SdifConverter::Panel::Panel()
: FloatingWindowContainer("SDIF Converter", *this)
, mPropertyOpen("Open", "Select a SDIF or a JSON file to convert", [&]()
                {
                    juce::FileChooser fc("Load a SDIF or a JSON file", {}, "*.sdif,*.json");
                    if(fc.browseForFileToOpen())
                    {
                        setFile(fc.getResult());
                    }
                })
, mPropertyName("File", "The SDIF file to convert", nullptr)

, mPropertyToSdifFrame("Frame", "Define the frame signature to encode the results in the SDIF file", nullptr)
, mPropertyToSdifMatrix("Matrix", "Define the matrix signature to encode the results in the SDIF file", nullptr)
, mPropertyToSdifExport("Convert to SDIF", "Convert the JSON file to a SDIF file", [&]()
                        {
                            exportToSdif();
                        })

, mPropertyToJsonFrame("Frame", "Select the frame signature to decode from the SDIF file", "", {}, [&](size_t index)
                       {
                           juce::ignoreUnused(index);
                           selectedFrameUpdated();
                       })
, mPropertyToJsonMatrix("Matrix", "Select the matrix signature to decode from the SDIF file", "", {}, [&](size_t index)
                        {
                            juce::ignoreUnused(index);
                            selectedMatrixUpdated();
                        })
, mPropertyToJsonRow("Row", "Select the row to decode from the SDIF file", "", {}, nullptr)
, mPropertyToJsonColumn("Column", "Select the colum, to decode from the SDIF file", "", {}, nullptr)
, mPropertyToJsonExport("Convert to JSON", "Convert the SDIF file to a JSON file", [&]()
                        {
                            exportToJson();
                        })
{
    mPropertyName.entry.setEnabled(false);
    addAndMakeVisible(mPropertyOpen);
    addAndMakeVisible(mPropertyName);

    addChildComponent(mPropertyToSdifFrame);
    addChildComponent(mPropertyToSdifMatrix);
    addChildComponent(mPropertyToSdifExport);
    mPropertyToSdifFrame.entry.onEditorShow = [this]()
    {
        if(auto* textEditor = mPropertyToSdifFrame.entry.getCurrentTextEditor())
        {
            textEditor->setInputRestrictions(4);
            textEditor->setJustification(juce::Justification::right);
        }
    };
    mPropertyToSdifFrame.entry.onTextChange = [this]()
    {
        auto const text = mPropertyToSdifFrame.entry.getText().toUpperCase();
        mPropertyToSdifFrame.entry.setText(text, juce::NotificationType::dontSendNotification);
        mPropertyToSdifExport.setEnabled(text.isNotEmpty() && mPropertyToSdifMatrix.entry.getText().isNotEmpty());
    };
    mPropertyToSdifFrame.entry.setText("????", juce::NotificationType::dontSendNotification);
    mPropertyToSdifMatrix.entry.onEditorShow = [this]()
    {
        if(auto* textEditor = mPropertyToSdifMatrix.entry.getCurrentTextEditor())
        {
            textEditor->setInputRestrictions(4);
            textEditor->setJustification(juce::Justification::right);
        }
    };
    mPropertyToSdifMatrix.entry.onTextChange = [this]()
    {
        auto const text = mPropertyToSdifMatrix.entry.getText().toUpperCase();
        mPropertyToSdifMatrix.entry.setText(text, juce::NotificationType::dontSendNotification);
        mPropertyToSdifExport.setEnabled(text.isNotEmpty() && mPropertyToSdifFrame.entry.getText().isNotEmpty());
    };
    mPropertyToSdifMatrix.entry.setText("????", juce::NotificationType::dontSendNotification);

    addChildComponent(mPropertyToJsonFrame);
    addChildComponent(mPropertyToJsonMatrix);
    addChildComponent(mPropertyToJsonRow);
    addChildComponent(mPropertyToJsonColumn);
    addChildComponent(mPropertyToJsonExport);
    mPropertyToJsonFrame.entry.setTextWhenNoChoicesAvailable(juce::translate("No frame available"));
    mPropertyToJsonMatrix.entry.setTextWhenNoChoicesAvailable(juce::translate("No matrix available"));
    mPropertyToJsonRow.entry.setTextWhenNoChoicesAvailable(juce::translate("No row available"));
    mPropertyToJsonColumn.entry.setTextWhenNoChoicesAvailable(juce::translate("No column available"));

    mInfos.setSize(300, 72);
    mInfos.setText(juce::translate("Select or drag and drop a SDIF file to convert to a JSON file or a JSON file to convert to a SDIF file."));
    mInfos.setMultiLine(true);
    mInfos.setReadOnly(true);
    addAndMakeVisible(mInfos);

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

    setBounds(mPropertyToSdifFrame);
    setBounds(mPropertyToSdifMatrix);
    setBounds(mPropertyToSdifExport);

    setBounds(mPropertyToJsonFrame);
    setBounds(mPropertyToJsonMatrix);
    setBounds(mPropertyToJsonRow);
    setBounds(mPropertyToJsonColumn);
    setBounds(mPropertyToJsonExport);

    setBounds(mInfos);

    setSize(300, std::max(bounds.getY(), 120) + 2);
}

void SdifConverter::Panel::paintOverChildren(juce::Graphics& g)
{
    if(mFileIsDragging)
    {
        g.fillAll(findColour(juce::TextButton::ColourIds::buttonColourId).withAlpha(0.5f));
    }
}

void SdifConverter::Panel::lookAndFeelChanged()
{
    auto const text = mInfos.getText();
    mInfos.clear();
    mInfos.setText(text);
}

void SdifConverter::Panel::parentHierarchyChanged()
{
    lookAndFeelChanged();
}

bool SdifConverter::Panel::isInterestedInFileDrag(juce::StringArray const& files)
{
    return std::any_of(files.begin(), files.end(), [](auto const& path)
                       {
                           juce::File const file(path);
                           return file.hasFileExtension("sdif") || file.hasFileExtension("json");
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
        if(file.hasFileExtension("sdif") || file.hasFileExtension("json"))
        {
            setFile(file);
            return;
        }
    }
}

void SdifConverter::Panel::setFile(juce::File const& file)
{
    mFile = file;
    if(file.hasFileExtension("json"))
    {
        mInfos.setVisible(false);
        mPropertyName.entry.setText(file.getFileName(), juce::NotificationType::dontSendNotification);

        mPropertyToSdifFrame.setVisible(true);
        mPropertyToSdifMatrix.setVisible(true);
        mPropertyToSdifExport.setVisible(true);

        mPropertyToJsonFrame.setVisible(false);
        mPropertyToJsonMatrix.setVisible(false);
        mPropertyToJsonRow.setVisible(false);
        mPropertyToJsonColumn.setVisible(false);
        mPropertyToJsonExport.setVisible(false);
    }
    else if(file.hasFileExtension("sdif"))
    {
        mInfos.setVisible(false);
        mPropertyName.entry.setText(file.getFileName(), juce::NotificationType::dontSendNotification);

        mPropertyToSdifFrame.setVisible(false);
        mPropertyToSdifMatrix.setVisible(false);
        mPropertyToSdifExport.setVisible(false);

        mPropertyToJsonFrame.setVisible(true);
        mPropertyToJsonMatrix.setVisible(true);
        mPropertyToJsonRow.setVisible(true);
        mPropertyToJsonColumn.setVisible(true);
        mPropertyToJsonExport.setVisible(true);

        mEntries = getEntries(file);

        // Update of the frames
        mPropertyToJsonFrame.entry.clear(juce::NotificationType::dontSendNotification);
        mFrameSigLinks.clear();

        for(auto const& frame : mEntries)
        {
            auto const frameSignature = frame.first;
            mFrameSigLinks.push_back(frameSignature);
            mPropertyToJsonFrame.entry.addItem(SdifSignatureToString(frameSignature), static_cast<int>(mFrameSigLinks.size()));
        }
        mPropertyToJsonFrame.entry.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
        mPropertyToJsonFrame.entry.setEnabled(mPropertyToJsonFrame.entry.getNumItems() > 0);

        selectedFrameUpdated();
    }
    else
    {
        mInfos.setVisible(true);
        mPropertyName.entry.setText(file == juce::File{} ? juce::translate("No file selected") : juce::translate("FLNAME: File extension not supported").replace("FLNAME", file.getFileName()), juce::NotificationType::dontSendNotification);

        mPropertyToSdifFrame.setVisible(false);
        mPropertyToSdifMatrix.setVisible(false);
        mPropertyToSdifExport.setVisible(false);

        mPropertyToJsonFrame.setVisible(false);
        mPropertyToJsonMatrix.setVisible(false);
        mPropertyToJsonRow.setVisible(false);
        mPropertyToJsonColumn.setVisible(false);
        mPropertyToJsonExport.setVisible(false);
    }
    resized();
}

void SdifConverter::Panel::selectedFrameUpdated()
{
    mPropertyToJsonMatrix.entry.clear(juce::NotificationType::dontSendNotification);
    mMatrixSigLinks.clear();
    auto const selectedFrameIndex = mPropertyToJsonFrame.entry.getSelectedItemIndex();
    if(selectedFrameIndex < 0 || static_cast<size_t>(selectedFrameIndex) > mFrameSigLinks.size())
    {
        mPropertyToSdifFrame.entry.setText("????", juce::NotificationType::dontSendNotification);
        mPropertyToJsonMatrix.entry.setEnabled(false);
        mPropertyToJsonRow.entry.setEnabled(false);
        selectedMatrixUpdated();
        return;
    }
    auto const frameIdentifier = mFrameSigLinks[static_cast<size_t>(selectedFrameIndex)];
    if(mEntries.count(frameIdentifier) == 0_z)
    {
        mPropertyToSdifFrame.entry.setText("????", juce::NotificationType::dontSendNotification);
        mPropertyToJsonMatrix.entry.setEnabled(false);
        mPropertyToJsonRow.entry.setEnabled(false);
        selectedMatrixUpdated();
        return;
    }

    mPropertyToSdifFrame.entry.setText(SdifSignatureToString(frameIdentifier), juce::NotificationType::dontSendNotification);
    auto const& matrixEntries = mEntries.at(frameIdentifier);
    for(auto const& matrix : matrixEntries)
    {
        auto const matrixSignature = matrix.first;
        mMatrixSigLinks.push_back(matrixSignature);
        mPropertyToJsonMatrix.entry.addItem(SdifSignatureToString(matrixSignature), static_cast<int>(mMatrixSigLinks.size()));
    }
    mPropertyToJsonMatrix.entry.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
    mPropertyToJsonMatrix.entry.setEnabled(mPropertyToJsonMatrix.entry.getNumItems() > 0);

    selectedMatrixUpdated();
}

void SdifConverter::Panel::selectedMatrixUpdated()
{
    mPropertyToJsonRow.entry.clear(juce::NotificationType::dontSendNotification);
    mPropertyToJsonColumn.entry.clear(juce::NotificationType::dontSendNotification);

    auto const selectedFrameIndex = mPropertyToJsonFrame.entry.getSelectedItemIndex();
    auto const selectedMatrixIndex = mPropertyToJsonMatrix.entry.getSelectedItemIndex();
    if(selectedFrameIndex < 0 || static_cast<size_t>(selectedFrameIndex) > mFrameSigLinks.size() || selectedMatrixIndex < 0 || static_cast<size_t>(selectedMatrixIndex) > mMatrixSigLinks.size())
    {
        mPropertyToSdifMatrix.entry.setText("????", juce::NotificationType::dontSendNotification);
        mPropertyToJsonRow.entry.setEnabled(false);
        mPropertyToJsonColumn.entry.setEnabled(false);
        mPropertyToJsonExport.entry.setEnabled(false);
        return;
    }

    auto const frameIdentifier = mFrameSigLinks[static_cast<size_t>(selectedFrameIndex)];
    auto const matrixIdentifier = mMatrixSigLinks[static_cast<size_t>(selectedMatrixIndex)];
    if(mEntries.count(frameIdentifier) == 0_z || mEntries.at(frameIdentifier).count(matrixIdentifier) == 0_z)
    {
        mPropertyToSdifMatrix.entry.setText("????", juce::NotificationType::dontSendNotification);
        mPropertyToJsonRow.entry.setEnabled(false);
        mPropertyToJsonColumn.entry.setEnabled(false);
        mPropertyToJsonExport.entry.setEnabled(false);
        return;
    }

    mPropertyToSdifMatrix.entry.setText(SdifSignatureToString(matrixIdentifier), juce::NotificationType::dontSendNotification);
    auto const matrixSize = mEntries.at(frameIdentifier).at(matrixIdentifier);
    for(auto row = 0_z; row < matrixSize.first; ++row)
    {
        mPropertyToJsonRow.entry.addItem(juce::String(row), static_cast<int>(row + 1));
    }
    mPropertyToJsonRow.entry.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
    mPropertyToJsonRow.entry.setEnabled(mPropertyToJsonRow.entry.getNumItems() > 0);

    if(matrixSize.second > 1_z)
    {
        mPropertyToJsonColumn.entry.addItem("All", 1);
    }
    for(auto column = 0_z; column < matrixSize.second; ++column)
    {
        mPropertyToJsonColumn.entry.addItem(juce::String(column), static_cast<int>(column + 2));
    }
    mPropertyToJsonColumn.entry.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
    mPropertyToJsonColumn.entry.setEnabled(mPropertyToJsonColumn.entry.getNumItems() > 0);

    mPropertyToJsonExport.entry.setEnabled(mPropertyToJsonRow.entry.getNumItems() > 0 && mPropertyToJsonColumn.entry.getNumItems() > 0);
}

void SdifConverter::Panel::exportToSdif()
{
    auto const frameName = mPropertyToSdifFrame.entry.getText();
    auto const matrixName = mPropertyToSdifMatrix.entry.getText();
    if(frameName.isEmpty() || matrixName.isEmpty())
    {
        anlWeakAssert(false);
        return;
    }

    if(frameName.contains("?") || matrixName.contains("?") || frameName.length() != 4 || matrixName.length() != 4)
    {
        AlertWindow::showMessage(AlertWindow::MessageType::warning, juce::translate("Failed to convert SDIF file..."), juce::translate("The frame and matrix signatures must contain 4 characters."));
        return;
    }

    auto const frameIdentifier = getSignature(frameName);
    auto const matrixIdentifier = getSignature(matrixName);
    juce::FileChooser fc("Select a SDIF file", mFile.withFileExtension("sdif"), "*.sdif");
    if(!fc.browseForFileToSave(true))
    {
        return;
    }
    auto const sdifFile = fc.getResult();
    enterModalState();
    auto const result = fromJson(mFile, sdifFile, frameIdentifier, matrixIdentifier);
    exitModalState(0);
    if(result.wasOk())
    {
        AlertWindow::showMessage(AlertWindow::MessageType::info, juce::translate("Convert succeeded"), juce::translate("The JSON file 'JSONFLNM' has been successfully converted to the SDIF file 'SDIFFLNM'.").replace("JSONFLNM", mFile.getFullPathName()).replace("SDIFFLNM", sdifFile.getFullPathName()) + "\n\n" + result.getErrorMessage());
    }
    else
    {
        AlertWindow::showMessage(AlertWindow::MessageType::warning, juce::translate("Failed to convert JSON file..."), juce::translate("There was an error while trying to convert the SDIF file 'JSONFLNM' to the SDIF file 'SDIFFLNM'.").replace("JSONFLNM", mFile.getFullPathName()).replace("SDIFFLNM", sdifFile.getFullPathName()) + "\n\n" + result.getErrorMessage());
    }
}

void SdifConverter::Panel::exportToJson()
{
    auto const selectedFrameIndex = mPropertyToJsonFrame.entry.getSelectedItemIndex();
    auto const selectedMatrixIndex = mPropertyToJsonMatrix.entry.getSelectedItemIndex();
    if(selectedFrameIndex < 0 || static_cast<size_t>(selectedFrameIndex) > mFrameSigLinks.size() || selectedMatrixIndex < 0 || static_cast<size_t>(selectedMatrixIndex) > mMatrixSigLinks.size())
    {
        anlWeakAssert(false);
        return;
    }

    auto const frameIdentifier = mFrameSigLinks[static_cast<size_t>(selectedFrameIndex)];
    auto const matrixIdentifier = mMatrixSigLinks[static_cast<size_t>(selectedMatrixIndex)];
    if(mEntries.count(frameIdentifier) == 0_z || mEntries.at(frameIdentifier).count(matrixIdentifier) == 0_z)
    {
        anlWeakAssert(false);
        return;
    }

    auto const row = mPropertyToJsonRow.entry.getSelectedId() - 1;
    auto const column = mPropertyToJsonColumn.entry.getSelectedId() - 1;
    if(row < 0 || column < 0)
    {
        anlWeakAssert(false);
        return;
    }

    juce::FileChooser fc("Select a JSON file", mFile.withFileExtension("json"), "*.json");
    if(!fc.browseForFileToSave(true))
    {
        return;
    }
    auto const jsonFile = fc.getResult();
    enterModalState();
    auto const result = toJson(mFile, jsonFile, frameIdentifier, matrixIdentifier, static_cast<size_t>(row), column == 0 ? std::optional<size_t>{} : static_cast<size_t>(column - 1));
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
