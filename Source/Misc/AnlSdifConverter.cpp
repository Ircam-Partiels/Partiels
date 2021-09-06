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

class SdifScopedFile
{
public:
    SdifScopedFile(juce::File const& f, bool readOnly)
    {
        SdifGenInit(nullptr);
        SdifSetExitFunc([]()
                        {
                        });
        auto print = [](SdifErrorTagET tag, SdifErrorLevelET level, char* message, SdifFileT*, SdifErrorT* error, char*, int)
        {
            std::cout << std::string(magic_enum::enum_name(tag).substr()) << " ";
            std::cout << "(" << std::string(magic_enum::enum_name(level).substr()) << ")";
            std::cout << ": " << message << " ";
            if(error != nullptr && error->UserMess != nullptr)
            {
                std::cout << " - " << error->UserMess;
            }
            std::cout << "\n";
        };
        SdifSetWarningFunc(print);
        SdifSetErrorFunc(print);
        file = SdifFOpen(f.getFullPathName().toRawUTF8(), readOnly ? eReadFile : eWriteFile);
    }

    ~SdifScopedFile()
    {
        if(file != nullptr)
        {
            SdifFClose(file);
        }
        SdifGenKill();
    }

    SdifFileT* file = nullptr;
};

std::map<uint32_t, std::map<uint32_t, SdifConverter::matrix_size_t>> SdifConverter::getEntries(juce::File const& inputFile)
{
    if(!inputFile.existsAsFile())
    {
        return {};
    }

    SdifScopedFile scopedFile(inputFile, true);
    if(scopedFile.file == nullptr)
    {
        return {};
    }
    auto* file = scopedFile.file;

    int endOfFile = 0;
    size_t bytesRead = 0;

    {
        auto const numBytes = SdifFReadGeneralHeader(file);
        if(numBytes == 0_z)
        {
            return {};
        }
        bytesRead += numBytes;
    }
    {
        auto const numBytes = SdifFReadAllASCIIChunks(file);
        if(numBytes == 0)
        {
            return {};
        }
        bytesRead += numBytes;
    }

    std::map<uint32_t, std::map<uint32_t, matrix_size_t>> entries;
    while(!endOfFile && SdifFLastError(file) == nullptr)
    {
        bytesRead += SdifFReadFrameHeader(file);
        while(!SdifFCurrFrameIsSelected(file))
        {
            SdifFSkipFrameData(file);
            endOfFile = SdifFGetSignature(file, &bytesRead) == eEof;
            if(endOfFile)
            {
                break;
            }
            bytesRead += SdifFReadFrameHeader(file);
        }

        auto const frameSignature = SdifFCurrFrameSignature(file);
        auto& frameEntry = entries[frameSignature];
        if(!endOfFile)
        {
            auto const numMatrix = SdifFCurrNbMatrix(file);
            for(SdifUInt4 m = 0; m < numMatrix; m++)
            {
                bytesRead += SdifFReadMatrixHeader(file);
                if(SdifFCurrMatrixIsSelected(file))
                {
                    auto const maxtrixSignature = SdifFCurrMatrixSignature(file);
                    auto* matrixType = SdifTestMatrixType(scopedFile.file, maxtrixSignature);
                    auto const newMatrix = frameEntry.count(maxtrixSignature);
                    auto& matrixEntry = frameEntry[SdifFCurrMatrixSignature(file)];

                    auto const numRows = static_cast<size_t>(SdifFCurrNbRow(file));
                    matrixEntry.first = std::max(numRows, newMatrix ? 0_z : matrixEntry.first);

                    auto const numColumns = SdifFCurrDataType(file) == SdifDataTypeE::eChar ? 1_z : static_cast<size_t>(SdifFCurrNbCol(file));

                    for(size_t colIndex = matrixEntry.second.size(); colIndex < numColumns; ++colIndex)
                    {
                        auto const* name = matrixType == nullptr ? nullptr : SdifMatrixTypeGetColumnName(matrixType, static_cast<int>(colIndex + 1));
                        matrixEntry.second.push_back(name == nullptr ? std::to_string(colIndex + 1) : name);
                    }
                }
                bytesRead += SdifFSkipMatrixData(file);
                bytesRead += SdifFReadPadding(file, SdifFPaddingCalculate(file->Stream, bytesRead));
            }

            endOfFile = SdifFGetSignature(file, &bytesRead) == eEof;
        }
    }
    return entries;
}

juce::Result SdifConverter::toJson(juce::File const& inputFile, juce::File const& outputFile, uint32_t frameId, uint32_t matrixId, std::optional<size_t> row, std::optional<size_t> column, std::optional<nlohmann::json> const& extraInfo)
{
    auto container = extraInfo.has_value() ? *extraInfo : nlohmann::json::object();
    auto& json = container["results"];

    {
        SdifScopedFile scopedFile(inputFile, true);
        if(scopedFile.file == nullptr)
        {
            return juce::Result::fail("Can't open input file");
        }
        auto* file = scopedFile.file;

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

        if(SdifTestFrameType(file, frameId) == nullptr)
        {
            return juce::Result::fail("Frame type undefined");
        }

        if(SdifTestMatrixType(file, matrixId) == nullptr)
        {
            return juce::Result::fail("Matrix type undefined");
        }

        while(!endOfFile && SdifFLastError(file) == nullptr)
        {
            bytesRead += SdifFReadFrameHeader(file);
            while(!SdifFCurrFrameIsSelected(file) || SdifFCurrSignature(file) != frameId)
            {
                SdifFSkipFrameData(file);
                endOfFile = SdifFGetSignature(file, &bytesRead) == eEof;
                if(endOfFile)
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
                        auto const dataType = SdifFCurrDataType(file);
                        if(dataType == SdifDataTypeET::eChar && column.has_value() && *column != 0_z)
                        {
                            return juce::Result::fail("Column index cannot be specified with text data type");
                        }
                        if(row.has_value() || numRows == static_cast<SdifUInt4>(1))
                        {
                            auto const selRow = row.has_value() ? *row : static_cast<SdifUInt4>(0);
                            for(auto rowIndex = 0_z; rowIndex < numRows; ++rowIndex)
                            {
                                bytesRead += SdifFReadOneRow(file);
                                if(rowIndex == selRow)
                                {
                                    auto& cjson = json[channelIndex];
                                    nlohmann::json vjson;
                                    vjson["time"] = time;

                                    if(dataType == SdifDataTypeET::eChar)
                                    {
                                        std::string label;
                                        for(SdifUInt4 col = 0; col < numColumns; col++)
                                        {
                                            label += file->CurrOneRow->Data.Char[col];
                                        }
                                        vjson["label"] = label;
                                    }
                                    else if(numColumns == 1 || column.has_value())
                                    {
                                        auto const columnIndex = column.has_value() ? static_cast<SdifUInt4>(*column) + static_cast<SdifUInt4>(1) : static_cast<SdifUInt4>(1);
                                        if(columnIndex <= numColumns)
                                        {
                                            vjson["value"] = SdifFCurrOneRowCol(file, columnIndex);
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
                            }
                        }
                        else
                        {
                            if(numColumns != static_cast<SdifUInt4>(1) && !column.has_value())
                            {
                                return juce::Result::fail("Can't convert all rows and all columns (either one row or one column must be selected)");
                            }

                            nlohmann::json vjson;
                            vjson["time"] = time;
                            for(auto rowIndex = 0_z; rowIndex < numRows; ++rowIndex)
                            {
                                bytesRead += SdifFReadOneRow(file);
                                auto const columnIndex = column.has_value() ? static_cast<SdifUInt4>(*column + 1_z) : static_cast<SdifUInt4>(1);
                                if(columnIndex <= numColumns)
                                {
                                    auto const value = SdifFCurrOneRowCol(file, columnIndex);
                                    vjson["values"].push_back(value);
                                }
                            }
                            json[channelIndex].push_back(std::move(vjson));
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

juce::Result SdifConverter::fromJson(juce::File const& inputFile, juce::File const& outputFile, uint32_t frameId, uint32_t matrixId, std::optional<juce::String> columnName)
{
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
    if(temp.getFile().create().failed())
    {
        return juce::Result::fail("Can't create temporary input file");
    }

    auto const& json = container.count("results") ? container.at("results") : container;

    {
        SdifScopedFile scopedFile(temp.getFile(), false);
        if(scopedFile.file == nullptr)
        {
            return juce::Result::fail("Can't open input file");
        }
        auto* file = scopedFile.file;

        if(SdifGetMatrixType(gSdifPredefinedTypes->MatrixTypesTable, matrixId) == nullptr)
        {
            auto* newMatrixType = SdifCreateMatrixType(matrixId, nullptr);
            if(newMatrixType == nullptr)
            {
                return juce::Result::fail(juce::translate("Can't create new matrix type"));
            }
            if(columnName.has_value())
            {
                SdifMatrixTypeInsertTailColumnDef(newMatrixType, columnName->getCharPointer());
            }
            else if(!json.empty() && !json[0_z].empty())
            {
                if(json[0_z][0_z].count("label"))
                {
                    SdifMatrixTypeInsertTailColumnDef(newMatrixType, "label");
                }
                else if(json[0_z][0_z].count("value"))
                {
                    SdifMatrixTypeInsertTailColumnDef(newMatrixType, "value");
                }
                else
                {
                    SdifMatrixTypeInsertTailColumnDef(newMatrixType, "values");
                }
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
                    std::copy_n(label.c_str(), data.size(), data.data());
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
                            SdifFWriteFrameAndOneMatrix(file, frameId, static_cast<SdifUInt4>(channelIndex), static_cast<SdifFloat8>(time), matrixId, SdifDataTypeET::eFloat8, static_cast<SdifUInt4>(values.size()), static_cast<SdifUInt4>(1), reinterpret_cast<void*>(values.data()));
                        }
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
: FloatingWindowContainer("SDIF Converter", *this)
, mPropertyOpen("Open", "Select a SDIF or a JSON file to convert", [&]()
                {
                    mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Load a SDIF or a JSON file"), juce::File{}, "*.sdif;*.json");
                    if(mFileChooser == nullptr)
                    {
                        return;
                    }
                    using Flags = juce::FileBrowserComponent::FileChooserFlags;
                    mFileChooser->launchAsync(Flags::openMode | Flags::canSelectFiles, [this](juce::FileChooser const& fileChooser)
                                              {
                                                  auto const results = fileChooser.getResults();
                                                  if(results.isEmpty())
                                                  {
                                                      return;
                                                  }
                                                  setFile(results.getFirst());
                                              });
                })
, mPropertyName("File", "The SDIF file to convert", nullptr)

, mPropertyToSdifFrame("Frame", "Define the frame signature to encode the results in the SDIF file", nullptr)
, mPropertyToSdifMatrix("Matrix", "Define the matrix signature to encode the results in the SDIF file", nullptr)
, mPropertyToSdifColName("Column Name", "Define the name of the column to encode the results in the SDIF file", nullptr)
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
, mPropertyToJsonRow("Row", "Select the row(s) to decode from the SDIF file", "", {}, [&](size_t index)
                     {
                         juce::ignoreUnused(index);
                         selectedRowColumnUpdated();
                     })
, mPropertyToJsonColumn("Column", "Select the colum(s) to decode from the SDIF file", "", {}, [&](size_t index)
                        {
                            juce::ignoreUnused(index);
                            selectedRowColumnUpdated();
                        })
, mPropertyToJsonUnit("Unit", "Define the unit of the results", [&](juce::String const& text)
                      {
                          mPropertyToJsonMinValue.entry.setTextValueSuffix(text);
                          mPropertyToJsonMaxValue.entry.setTextValueSuffix(text);
                      })
, mPropertyToJsonMinValue("Value Range Min.", "Define the minimum value of the results.", "", {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max()}, 0.0f, [&](float value)
                          {
                              auto const max = std::max(static_cast<float>(mPropertyToJsonMaxValue.entry.getValue()), value);
                              mPropertyToJsonMaxValue.entry.setValue(max, juce::NotificationType::dontSendNotification);
                          })
, mPropertyToJsonMaxValue("Value Range Max.", "Define the maximum value of the results.", "", {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max()}, 0.0f, [&](float value)
                          {
                              auto const min = std::min(static_cast<float>(mPropertyToJsonMinValue.entry.getValue()), value);
                              mPropertyToJsonMinValue.entry.setValue(min, juce::NotificationType::dontSendNotification);
                          })
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
    addChildComponent(mPropertyToSdifColName);
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
    addChildComponent(mPropertyToJsonUnit);
    addChildComponent(mPropertyToJsonMinValue);
    addChildComponent(mPropertyToJsonMaxValue);
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
    setBounds(mPropertyToSdifColName);
    setBounds(mPropertyToSdifExport);

    setBounds(mPropertyToJsonFrame);
    setBounds(mPropertyToJsonMatrix);
    setBounds(mPropertyToJsonRow);
    setBounds(mPropertyToJsonColumn);
    setBounds(mPropertyToJsonUnit);
    setBounds(mPropertyToJsonMinValue);
    setBounds(mPropertyToJsonMaxValue);
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
        mPropertyToSdifColName.setVisible(true);
        mPropertyToSdifExport.setVisible(true);

        mPropertyToJsonFrame.setVisible(false);
        mPropertyToJsonMatrix.setVisible(false);
        mPropertyToJsonRow.setVisible(false);
        mPropertyToJsonColumn.setVisible(false);
        mPropertyToJsonExport.setVisible(false);
        mPropertyToJsonUnit.setVisible(false);
        mPropertyToJsonMinValue.setVisible(false);
        mPropertyToJsonMaxValue.setVisible(false);
    }
    else if(file.hasFileExtension("sdif"))
    {
        mInfos.setVisible(false);
        mPropertyName.entry.setText(file.getFileName(), juce::NotificationType::dontSendNotification);

        mPropertyToSdifFrame.setVisible(false);
        mPropertyToSdifMatrix.setVisible(false);
        mPropertyToSdifColName.setVisible(false);
        mPropertyToSdifExport.setVisible(false);

        mPropertyToJsonFrame.setVisible(true);
        mPropertyToJsonMatrix.setVisible(true);
        mPropertyToJsonRow.setVisible(true);
        mPropertyToJsonColumn.setVisible(true);
        mPropertyToJsonUnit.setVisible(true);
        mPropertyToJsonMinValue.setVisible(true);
        mPropertyToJsonMaxValue.setVisible(true);
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
        mPropertyToSdifColName.setVisible(false);
        mPropertyToSdifExport.setVisible(false);

        mPropertyToJsonFrame.setVisible(false);
        mPropertyToJsonMatrix.setVisible(false);
        mPropertyToJsonRow.setVisible(false);
        mPropertyToJsonColumn.setVisible(false);
        mPropertyToJsonExport.setVisible(false);
        mPropertyToJsonUnit.setVisible(false);
        mPropertyToJsonMinValue.setVisible(false);
        mPropertyToJsonMaxValue.setVisible(false);
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

    if(matrixSize.first > 1_z)
    {
        mPropertyToJsonRow.entry.addItem("All", 1);
    }
    for(auto row = 0_z; row < matrixSize.first; ++row)
    {
        mPropertyToJsonRow.entry.addItem(juce::String(row), static_cast<int>(row + 2));
    }
    mPropertyToJsonRow.entry.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
    mPropertyToJsonRow.entry.setEnabled(mPropertyToJsonRow.entry.getNumItems() > 0);

    if(matrixSize.second.size() > 1_z)
    {
        mPropertyToJsonColumn.entry.addItem("All", 1);
    }
    for(auto column = 0_z; column < matrixSize.second.size(); ++column)
    {
        mPropertyToJsonColumn.entry.addItem(matrixSize.second[column], static_cast<int>(column + 2));
    }
    mPropertyToJsonColumn.entry.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
    mPropertyToJsonColumn.entry.setEnabled(mPropertyToJsonColumn.entry.getNumItems() > 0);

    mPropertyToJsonExport.entry.setEnabled(mPropertyToJsonRow.entry.getNumItems() > 0 && mPropertyToJsonColumn.entry.getNumItems() > 0);
    selectedRowColumnUpdated();
}

void SdifConverter::Panel::selectedRowColumnUpdated()
{
    auto const row = mPropertyToJsonRow.entry.getSelectedId() - 1;
    auto const column = mPropertyToJsonColumn.entry.getSelectedId() - 1;
    if(row < 0 || column < 0)
    {
        anlWeakAssert(false);
        return;
    }
    mPropertyToJsonColumn.entry.setItemEnabled(1, row != 0);
    if(row == 0 && column == 0)
    {
        mPropertyToJsonColumn.entry.setSelectedId(2, juce::NotificationType::dontSendNotification);
    }
}

void SdifConverter::Panel::exportToSdif()
{
    auto const frameName = mPropertyToSdifFrame.entry.getText();
    auto const matrixName = mPropertyToSdifMatrix.entry.getText();
    auto const colName = mPropertyToSdifColName.entry.getText();
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

    mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Select a SDIF file"), mFile.withFileExtension("sdif"), "*.sdif");
    if(mFileChooser == nullptr)
    {
        return;
    }
    using Flags = juce::FileBrowserComponent::FileChooserFlags;
    mFileChooser->launchAsync(Flags::saveMode | Flags::canSelectFiles | Flags::warnAboutOverwriting, [=, this](juce::FileChooser const& fileChooser)
                              {
                                  auto const results = fileChooser.getResults();
                                  if(results.isEmpty())
                                  {
                                      return;
                                  }
                                  auto const sdifFile = results.getFirst();
                                  enterModalState();
                                  auto const result = fromJson(mFile, sdifFile, frameIdentifier, matrixIdentifier, colName.isEmpty() ? std::optional<juce::String>() : colName);
                                  exitModalState(0);
                                  if(result.wasOk())
                                  {
                                      AlertWindow::showMessage(AlertWindow::MessageType::info, juce::translate("Convert succeeded"), juce::translate("The JSON file 'JSONFLNM' has been successfully converted to the SDIF file 'SDIFFLNM'.").replace("JSONFLNM", mFile.getFullPathName()).replace("SDIFFLNM", sdifFile.getFullPathName()) + "\n\n" + result.getErrorMessage());
                                  }
                                  else
                                  {
                                      AlertWindow::showMessage(AlertWindow::MessageType::warning, juce::translate("Failed to convert JSON file..."), juce::translate("There was an error while trying to convert the SDIF file 'JSONFLNM' to the SDIF file 'SDIFFLNM'.").replace("JSONFLNM", mFile.getFullPathName()).replace("SDIFFLNM", sdifFile.getFullPathName()) + "\n\n" + result.getErrorMessage());
                                  }
                              });
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

    auto const unit = mPropertyToJsonUnit.entry.getText();
    juce::Range<double> const range{mPropertyToJsonMinValue.entry.getValue(), mPropertyToJsonMaxValue.entry.getValue()};
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

    mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Select a JSON file"), mFile.withFileExtension("json"), "*.json");
    if(mFileChooser == nullptr)
    {
        return;
    }
    using Flags = juce::FileBrowserComponent::FileChooserFlags;
    mFileChooser->launchAsync(Flags::saveMode | Flags::canSelectFiles | Flags::warnAboutOverwriting, [=, this](juce::FileChooser const& fileChooser)
                              {
                                  auto const results = fileChooser.getResults();
                                  if(results.isEmpty())
                                  {
                                      return;
                                  }
                                  auto const jsonFile = results.getFirst();
                                  enterModalState();
                                  auto const result = toJson(mFile, jsonFile, frameIdentifier, matrixIdentifier, row == 0 ? std::optional<size_t>{} : static_cast<size_t>(row - 1), column == 0 ? std::optional<size_t>{} : static_cast<size_t>(column - 1), extra);
                                  exitModalState(0);
                                  if(result.wasOk())
                                  {
                                      AlertWindow::showMessage(AlertWindow::MessageType::info, juce::translate("Convert succeeded"), juce::translate("The SDIF file 'SDIFFLNM' has been successfully converted to the JSON file 'JSONFLNM'.").replace("SDIFFLNM", mFile.getFullPathName()).replace("JSONFLNM", jsonFile.getFullPathName()) + "\n\n" + result.getErrorMessage());
                                  }
                                  else
                                  {
                                      AlertWindow::showMessage(AlertWindow::MessageType::warning, juce::translate("Failed to convert SDIF file..."), juce::translate("There was an error while trying to convert the SDIF file 'SDIFFLNM' to the JSON file 'JSONFLNM'.").replace("SDIFFLNM", mFile.getFullPathName()).replace("JSONFLNM", jsonFile.getFullPathName()) + "\n\n" + result.getErrorMessage());
                                  }
                              });
}

ANALYSE_FILE_END
