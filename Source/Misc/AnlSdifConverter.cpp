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

juce::String SdifConverter::getString(uint32_t signature)
{
    return juce::String(SdifSignatureToString(signature));
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

ANALYSE_FILE_END
