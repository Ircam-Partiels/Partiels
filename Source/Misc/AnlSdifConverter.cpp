#include "AnlSdifConverter.h"
#ifdef JUCE_CLANG
#if !defined(__has_warning) || __has_warning("-Winvalid-utf8")
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-utf8"
#endif
#endif
#include <sdif.h>
#ifdef JUCE_CLANG
#if !defined(__has_warning) || __has_warning("-Winvalid-utf8")
#pragma clang diagnostic pop
#endif
#endif

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
    if(signature == uint32_t(0))
    {
        return "";
    }
    return juce::String(SdifSignatureToString(signature));
}

juce::Result SdifConverter::read(juce::File const& file, FrameCallbackFn frameCallbackFn, MatrixCallbackFn matrixCallbackFn, DataCallbackFn dataCallbackFn)
{
    if(!file.existsAsFile())
    {
        return juce::Result::fail("Error while parsing SDIF file");
    }
    SdifGenInit(nullptr);
    auto const* filepath = file.getFullPathName().toRawUTF8();

    struct CallbackFns
    {
        FrameCallbackFn frameCallbackFn;
        MatrixCallbackFn matrixCallbackFn;
        DataCallbackFn dataCallbackFn;
    };

    CallbackFns callbackFns;
    callbackFns.frameCallbackFn = frameCallbackFn;
    callbackFns.matrixCallbackFn = matrixCallbackFn;
    callbackFns.dataCallbackFn = dataCallbackFn;

    auto iFrameCallback = [](SdifFileT* f, void* userdata)
    {
        auto* cFns = reinterpret_cast<CallbackFns*>(userdata);
        MiscWeakAssert(cFns != nullptr);
        if(cFns == nullptr)
        {
            return 0;
        }
        auto const frameSignature = static_cast<uint32_t>(SdifFCurrFrameSignature(f));
        auto const frameIndex = static_cast<size_t>(SdifFCurrID(f));
        auto const time = static_cast<double>(SdifFCurrTime(f));
        auto const numMatrix = static_cast<size_t>(SdifFCurrNbMatrix(f));
        auto const* frameType = SdifTestFrameType(f, frameSignature);
        MiscWeakAssert(frameType != nullptr);
        juce::ignoreUnused(frameType);
        if(cFns->frameCallbackFn == nullptr)
        {
            return 1;
        }
        return cFns->frameCallbackFn(frameSignature, frameIndex, time, numMatrix) ? 1 : 0;
    };

    auto iMatrixCallback = [](SdifFileT* f, int nummatrix, void* userdata)
    {
        juce::ignoreUnused(nummatrix);
        auto* cFns = reinterpret_cast<CallbackFns*>(userdata);
        MiscWeakAssert(cFns != nullptr);
        if(cFns == nullptr)
        {
            return 0;
        }
        auto const matrixSignature = static_cast<uint32_t>(SdifFCurrMatrixSignature(f));
        auto const frameIndex = static_cast<size_t>(SdifFCurrID(f));
        auto const numRows = static_cast<size_t>(SdifFCurrNbRow(f));
        auto const dataType = SdifFCurrDataType(f);
        auto const numColumns = dataType == SdifDataTypeE::eChar ? 1_z : static_cast<size_t>(SdifFCurrNbCol(f));
        auto* matrixType = SdifTestMatrixType(f, matrixSignature);
        MiscWeakAssert(matrixType != nullptr);
        if(cFns->matrixCallbackFn == nullptr)
        {
            return 1;
        }

        auto getColumnName = [&](size_t columnIndex)
        {
            if(matrixType == nullptr)
            {
                return std::to_string(columnIndex + 1);
            }
            auto const* name = SdifMatrixTypeGetColumnName(matrixType, static_cast<int>(columnIndex + 1));
            if(name == nullptr)
            {
                return std::to_string(columnIndex + 1);
            }
            return std::string(name);
        };

        std::vector<std::string> columnNames;
        columnNames.reserve(numColumns);
        for(auto columnIndex = 0_z; columnIndex < numColumns; ++columnIndex)
        {
            columnNames.push_back(getColumnName(columnIndex));
        }

        auto const frameSignature = static_cast<uint32_t>(SdifFCurrFrameSignature(f));
        return cFns->matrixCallbackFn(frameSignature, frameIndex, matrixSignature, numRows, numColumns, std::move(columnNames)) ? 1 : 0;
    };

    auto iDataCallback = [](SdifFileT* f, int nummatrix, void* userdata)
    {
        juce::ignoreUnused(nummatrix);
        auto* cFns = reinterpret_cast<CallbackFns*>(userdata);
        MiscWeakAssert(cFns != nullptr);
        if(cFns == nullptr)
        {
            return 0;
        }
        if(cFns->dataCallbackFn == nullptr)
        {
            return 1;
        }

        auto const frameSignature = static_cast<uint32_t>(SdifFCurrFrameSignature(f));
        auto const frameIndex = static_cast<size_t>(SdifFCurrID(f));
        auto const time = static_cast<double>(SdifFCurrTime(f));
        auto const matrixSignature = static_cast<uint32_t>(SdifFCurrMatrixSignature(f));
        auto const numRows = static_cast<size_t>(SdifFCurrNbRow(f));
        auto const numColumns = static_cast<size_t>(SdifFCurrNbCol(f));
        auto const dataType = SdifFCurrDataType(f);

        auto* matrixData = SdifFCurrMatrixData(f);
        if(matrixData == nullptr)
        {
            return 1;
        }
        if(dataType == SdifDataTypeET::eChar)
        {
            std::vector<std::string> labels;
            for(auto rowIndex = 0_z; rowIndex < numRows; ++rowIndex)
            {
                std::string label;
                for(SdifUInt4 col = 0; col < static_cast<SdifUInt4>(numColumns); col++)
                {
                    label += matrixData->Data.Char[col];
                }
                labels.push_back(label);
            }
            return cFns->dataCallbackFn(frameSignature, frameIndex, matrixSignature, time, numRows, 1, {std::move(labels)}) ? 1 : 0;
        }
        std::vector<std::vector<double>> results;
        for(SdifUInt4 rowIndex = 1; rowIndex <= static_cast<SdifUInt4>(numRows); ++rowIndex)
        {
            std::vector<double> rowValues;
            rowValues.reserve(numColumns);
            for(SdifUInt4 columnIndex = 1; columnIndex <= static_cast<SdifUInt4>(numColumns); columnIndex++)
            {
                rowValues.push_back(SdifMatrixDataGetValue(matrixData, rowIndex, columnIndex));
            }
            results.push_back(std::move(rowValues));
        }
        return cFns->dataCallbackFn(frameSignature, frameIndex, matrixSignature, time, numRows, numColumns, {std::move(results)}) ? 1 : 0;
    };

    if(SdifReadFile(filepath, nullptr, iFrameCallback, iMatrixCallback, iDataCallback, nullptr, &callbackFns) == 0)
    {
        SdifGenKill();
        return juce::Result::fail("Error while parsing SDIF file");
    }
    SdifGenKill();
    return juce::Result::ok();
}

juce::Result SdifConverter::write(juce::File const& file, juce::Range<double> const& timeRange, uint32_t frameId, uint32_t matrixId, std::optional<juce::String> columnName, std::variant<Markers, Points, Columns> const& data, std::function<bool(void)> shouldContinue)
{
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

    SdifScopedFile scopedFile(file, false);
    if(scopedFile.file == nullptr)
    {
        return juce::Result::fail("Can't open input file");
    }
    auto* sfile = scopedFile.file;

    if(!shouldContinue())
    {
        return juce::Result::fail("Aborted");
    }

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
        if(data.index() == 0_z)
        {
            SdifMatrixTypeInsertTailColumnDef(newMatrixType, "label");
        }
        else if(data.index() == 1_z)
        {
            SdifMatrixTypeInsertTailColumnDef(newMatrixType, "value");
        }
        else if(data.index() == 2_z)
        {
            SdifMatrixTypeInsertTailColumnDef(newMatrixType, "values");
        }
        else
        {
            return juce::Result::fail("Unsupported format");
        }
        SdifPutMatrixType(sfile->MatrixTypesTable, newMatrixType);
    }

    if(SdifTestMatrixType(sfile, matrixId) == nullptr)
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
        SdifPutFrameType(sfile->FrameTypesTable, newFrameType);
        char name[] = "MAT0";
        newFrameType = SdifFrameTypePutComponent(newFrameType, matrixId, name);
    }

    if(SdifTestFrameType(sfile, frameId) == nullptr)
    {
        return juce::Result::fail("Frame type undefined");
    }

    SdifFWriteGeneralHeader(sfile);
    SdifFWriteAllASCIIChunks(sfile);

    if(!shouldContinue())
    {
        return juce::Result::fail("Aborted");
    }

    if(auto* markers = std::get_if<Markers>(&data))
    {
        for(size_t channelIndex = 0_z; channelIndex < markers->size(); ++channelIndex)
        {
            auto const& channelData = markers->at(channelIndex);
            for(size_t frameIndex = 0_z; frameIndex < channelData.size(); ++frameIndex)
            {
                auto const& frameData = channelData[frameIndex];
                auto const time = std::get<0>(frameData);
                if(timeRange.isEmpty() || timeRange.contains(time))
                {
                    auto const label = std::get<2>(frameData);
                    std::vector<SdifChar> text;
                    text.resize(label.length());
                    std::copy_n(label.c_str(), text.size(), text.data());
                    SdifFWriteFrameAndOneMatrix(sfile, frameId, static_cast<SdifUInt4>(channelIndex), static_cast<SdifFloat8>(time), matrixId, SdifDataTypeET::eChar, static_cast<SdifUInt4>(1), static_cast<SdifUInt4>(text.size()), reinterpret_cast<void*>(text.data()));
                }
                if(!shouldContinue())
                {
                    return juce::Result::fail("Aborted");
                }
            }
        }
    }
    else if(auto* points = std::get_if<Points>(&data))
    {
        for(size_t channelIndex = 0_z; channelIndex < points->size(); ++channelIndex)
        {
            auto const& channelData = points->at(channelIndex);
            for(size_t frameIndex = 0_z; frameIndex < channelData.size(); ++frameIndex)
            {
                auto const& frameData = channelData[frameIndex];
                auto const time = std::get<0>(frameData);
                if(timeRange.isEmpty() || timeRange.contains(time))
                {
                    if(std::get<2>(frameData).has_value())
                    {
                        auto value = static_cast<SdifFloat8>(*std::get<2>(frameData));
                        SdifFWriteFrameAndOneMatrix(sfile, frameId, static_cast<SdifUInt4>(channelIndex), static_cast<SdifFloat8>(time), matrixId, SdifDataTypeET::eFloat8, static_cast<SdifUInt4>(1), static_cast<SdifUInt4>(1), reinterpret_cast<void*>(&value));
                    }
                    else
                    {
                        SdifFSetCurrFrameHeader(sfile, frameId, static_cast<SdifUInt4>(SdifSizeOfFrameHeader()), 0, static_cast<SdifUInt4>(channelIndex), static_cast<SdifFloat8>(time));
                        SdifFWriteFrameHeader(sfile);
                    }
                }
                if(!shouldContinue())
                {
                    return juce::Result::fail("Aborted");
                }
            }
        }
    }
    else if(auto* columns = std::get_if<Columns>(&data))
    {
        for(size_t channelIndex = 0_z; channelIndex < columns->size(); ++channelIndex)
        {
            auto const& channelData = columns->at(channelIndex);
            for(size_t frameIndex = 0_z; frameIndex < channelData.size(); ++frameIndex)
            {
                auto const& frameData = channelData[frameIndex];
                auto const time = std::get<0>(frameData);
                if(timeRange.isEmpty() || timeRange.contains(time))
                {
                    std::vector<SdifFloat8> values;
                    values.resize(std::get<2>(frameData).size());
                    for(size_t binIndex = 0_z; binIndex < std::get<2>(frameData).size(); ++binIndex)
                    {
                        values[binIndex] = std::get<2>(frameData).at(binIndex);
                    }
                    SdifFWriteFrameAndOneMatrix(sfile, frameId, static_cast<SdifUInt4>(channelIndex), static_cast<SdifFloat8>(time), matrixId, SdifDataTypeET::eFloat8, static_cast<SdifUInt4>(values.size()), static_cast<SdifUInt4>(1), reinterpret_cast<void*>(values.data()));
                }
            }

            if(!shouldContinue())
            {
                return juce::Result::fail("Aborted");
            }
        }
    }
    return juce::Result::ok();
}

std::map<uint32_t, std::map<uint32_t, SdifConverter::matrix_size_t>> SdifConverter::getEntries(juce::File const& file)
{
    std::map<uint32_t, std::map<uint32_t, matrix_size_t>> entries;
    read(
        file, nullptr, [&](uint32_t frameSignature, size_t frameIndex, uint32_t matrixSignature, size_t numRows, size_t numColumns, std::vector<std::string> names)
        {
            juce::ignoreUnused(frameIndex, numColumns);
            auto const newMatrix = entries[frameSignature].count(matrixSignature) == 0_z;
            auto& entry = entries[frameSignature][matrixSignature];
            entry.first = std::max(numRows, newMatrix ? 0_z : entry.first);
            for(size_t index = entry.second.size(); index < names.size(); ++index)
            {
                entry.second.push_back(std::move(names[index]));
            }
            return false;
        },
        nullptr);
    return entries;
}

juce::Result SdifConverter::toJson(juce::File const& inputFile, juce::File const& outputFile, uint32_t frameId, uint32_t matrixId, std::optional<size_t> row, std::optional<size_t> column, std::optional<nlohmann::json> const& extraInfo)
{
    auto container = extraInfo.has_value() ? *extraInfo : nlohmann::json::object();
    auto& json = container["results"];
    auto result = juce::Result::ok();
    read(
        inputFile, [&](uint32_t frameSignature, size_t frameIndex, double time, size_t numMarix)
        {
            juce::ignoreUnused(frameIndex, time, numMarix);
            return result.wasOk() && frameSignature == frameId;
        },
        [&](uint32_t frameSignature, size_t frameIndex, uint32_t matrixSignature, size_t numRows, size_t numColumns, std::vector<std::string> names)
        {
            juce::ignoreUnused(frameSignature, frameIndex, numRows, numColumns, names);
            return result.wasOk() && matrixSignature == matrixId;
        },
        [&](uint32_t frameSignature, size_t frameIndex, uint32_t matrixSignature, double time, size_t numRows, size_t numColumns, std::variant<std::vector<std::string>, std::vector<std::vector<double>>> data)
        {
            juce::ignoreUnused(frameSignature, matrixSignature);
            if(result.failed())
            {
                return false;
            }
            if(data.index() == 0_z && column.has_value() && *column != 0_z)
            {
                result = juce::Result::fail("Column index cannot be specified with text data type");
                return false;
            }
            if(numRows != 1_z && !row.has_value() && numColumns != 1_z && !column.has_value())
            {
                result = juce::Result::fail("Can't convert all rows and all columns (either one row or one column must be selected)");
                return false;
            }

            auto const* labels = std::get_if<0_z>(&data);
            auto const* values = std::get_if<1_z>(&data);

            nlohmann::json vjson;
            vjson["time"] = time;

            if(row.has_value() || numRows == 1_z)
            {
                auto const rowIndex = row.has_value() ? *row : 0_z;
                if(labels != nullptr && labels->size() > rowIndex)
                {
                    vjson["label"] = labels->at(rowIndex);
                }
                else if(values != nullptr && values->size() > rowIndex)
                {
                    if(column.has_value() || numColumns == 1)
                    {
                        auto const columnIndex = column.has_value() ? *column : 0_z;
                        if(values->at(rowIndex).size() > columnIndex)
                        {
                            vjson["value"] = values->at(rowIndex).at(columnIndex);
                        }
                    }
                    else
                    {
                        vjson["values"] = values->at(rowIndex);
                    }
                }
            }
            else if(values != nullptr)
            {
                auto const columnIndex = column.has_value() ? *column : 0_z;
                for(auto rowIndex = 0_z; rowIndex < values->size(); ++rowIndex)
                {
                    if(columnIndex < values->at(rowIndex).size())
                    {
                        vjson["values"].push_back(values->at(rowIndex).at(columnIndex));
                    }
                }
            }
            json[frameIndex].push_back(std::move(vjson));
            return true;
        });

    if(result.failed())
    {
        return result;
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
: mPropertyToSdifFrame("Frame", "Define the frame signature to encode the results in the SDIF file", [this](juce::String)
                       {
                           notify();
                       })
, mPropertyToSdifMatrix("Matrix", "Define the matrix signature to encode the results in the SDIF file", [this](juce::String)
                        {
                            notify();
                        })
, mPropertyToSdifColName("Column Name", "Define the name of the column to encode the results in the SDIF file", [this](juce::String)
                         {
                             notify();
                         })

, mPropertyToJsonFrame("Frame", "Select the frame signature to decode from the SDIF file", "", {}, [&]([[maybe_unused]] size_t index)
                       {
                           selectedFrameUpdated();
                       })
, mPropertyToJsonMatrix("Matrix", "Select the matrix signature to decode from the SDIF file", "", {}, [&]([[maybe_unused]] size_t index)
                        {
                            selectedMatrixUpdated();
                        })
, mPropertyToJsonRow("Row", "Select the row(s) to decode from the SDIF file", "", {}, [&]([[maybe_unused]] size_t index)
                     {
                         selectedRowColumnUpdated();
                     })
, mPropertyToJsonColumn("Column", "Select the colum(s) to decode from the SDIF file", "", {}, [&]([[maybe_unused]] size_t index)
                        {
                            selectedRowColumnUpdated();
                        })
{
    addChildComponent(mPropertyToSdifFrame);
    addChildComponent(mPropertyToSdifMatrix);
    addChildComponent(mPropertyToSdifColName);
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
    };
    mPropertyToSdifMatrix.entry.setText("????", juce::NotificationType::dontSendNotification);

    addChildComponent(mPropertyToJsonFrame);
    addChildComponent(mPropertyToJsonMatrix);
    addChildComponent(mPropertyToJsonRow);
    addChildComponent(mPropertyToJsonColumn);

    mPropertyToJsonFrame.entry.setTextWhenNoChoicesAvailable(juce::translate("No frame available"));
    mPropertyToJsonMatrix.entry.setTextWhenNoChoicesAvailable(juce::translate("No matrix available"));
    mPropertyToJsonRow.entry.setTextWhenNoChoicesAvailable(juce::translate("No row available"));
    mPropertyToJsonColumn.entry.setTextWhenNoChoicesAvailable(juce::translate("No column available"));

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
    setBounds(mPropertyToSdifFrame);
    setBounds(mPropertyToSdifMatrix);
    setBounds(mPropertyToSdifColName);

    setBounds(mPropertyToJsonFrame);
    setBounds(mPropertyToJsonMatrix);
    setBounds(mPropertyToJsonRow);
    setBounds(mPropertyToJsonColumn);

    setSize(getWidth(), std::max(bounds.getY(), 24));
}

std::tuple<uint32_t, uint32_t, std::optional<juce::String>> SdifConverter::Panel::getToSdifFormat() const
{
    auto const frameName = mPropertyToSdifFrame.entry.getText();
    auto const matrixName = mPropertyToSdifMatrix.entry.getText();
    auto const colName = mPropertyToSdifColName.entry.getText();
    if((!frameName.isEmpty() && (frameName.contains("?") || frameName.length() != 4)) || (!matrixName.isEmpty() && (matrixName.contains("?") || matrixName.length() != 4)))
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::WarningIcon)
                                 .withTitle(juce::translate("Invalid SDIF Format!"))
                                 .withMessage(juce::translate("The frame and matrix signatures must contain 4 characters."))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
        return {uint32_t(0), uint32_t(0), std::optional<juce::String>{}};
    }

    auto const frameIdentifier = SdifConverter::getSignature(frameName);
    auto const matrixIdentifier = SdifConverter::getSignature(matrixName);
    return {frameIdentifier, matrixIdentifier, colName.isEmpty() ? std::optional<juce::String>{} : std::optional<juce::String>(colName)};
}

std::tuple<uint32_t, uint32_t, std::optional<size_t>, std::optional<size_t>> SdifConverter::Panel::getFromSdifFormat() const
{
    auto const selectedFrameIndex = mPropertyToJsonFrame.entry.getSelectedItemIndex();
    auto const selectedMatrixIndex = mPropertyToJsonMatrix.entry.getSelectedItemIndex();
    if(selectedFrameIndex < 0 || static_cast<size_t>(selectedFrameIndex) > mFrameSigLinks.size() || selectedMatrixIndex < 0 || static_cast<size_t>(selectedMatrixIndex) > mMatrixSigLinks.size())
    {
        return {uint32_t(0), uint32_t(0), std::optional<size_t>{}, std::optional<size_t>{}};
    }

    auto const frameIdentifier = mFrameSigLinks[static_cast<size_t>(selectedFrameIndex)];
    auto const matrixIdentifier = mMatrixSigLinks[static_cast<size_t>(selectedMatrixIndex)];
    if(mEntries.count(frameIdentifier) == 0_z || mEntries.at(frameIdentifier).count(matrixIdentifier) == 0_z)
    {
        return {uint32_t(0), uint32_t(0), std::optional<size_t>{}, std::optional<size_t>{}};
    }

    auto const row = std::max(mPropertyToJsonRow.entry.getSelectedId() - 1, 0);
    auto const column = std::max(mPropertyToJsonColumn.entry.getSelectedId() - 1, 0);
    return {frameIdentifier, matrixIdentifier, row == 0 ? std::optional<size_t>{} : static_cast<size_t>(row - 1), column == 0 ? std::optional<size_t>{} : static_cast<size_t>(column - 1)};
}

void SdifConverter::Panel::setFile(juce::File const& file)
{
    mFile = file;
    mPropertyToSdifFrame.setVisible(false);
    mPropertyToSdifMatrix.setVisible(false);
    mPropertyToSdifColName.setVisible(false);

    mPropertyToJsonFrame.setVisible(false);
    mPropertyToJsonMatrix.setVisible(false);
    mPropertyToJsonRow.setVisible(false);
    mPropertyToJsonColumn.setVisible(false);

    if(file.hasFileExtension("json"))
    {
        mPropertyToSdifFrame.setVisible(true);
        mPropertyToSdifMatrix.setVisible(true);
        mPropertyToSdifColName.setVisible(true);
        notify();
    }
    else if(file.hasFileExtension("sdif"))
    {
        mPropertyToJsonFrame.setVisible(true);
        mPropertyToJsonMatrix.setVisible(true);
        mPropertyToJsonRow.setVisible(true);
        mPropertyToJsonColumn.setVisible(true);

        mEntries = SdifConverter::getEntries(file);

        // Update of the frames
        mPropertyToJsonFrame.entry.clear(juce::NotificationType::dontSendNotification);
        mFrameSigLinks.clear();

        for(auto const& frame : mEntries)
        {
            auto const frameSignature = frame.first;
            mFrameSigLinks.push_back(frameSignature);
            mPropertyToJsonFrame.entry.addItem(SdifConverter::getString(frameSignature), static_cast<int>(mFrameSigLinks.size()));
        }
        mPropertyToJsonFrame.entry.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
        mPropertyToJsonFrame.entry.setEnabled(mPropertyToJsonFrame.entry.getNumItems() > 1);

        selectedFrameUpdated();
    }
    else
    {
        notify();
    }
    resized();
}

bool SdifConverter::Panel::hasAnyChangeableOption() const
{
    if(mFile.hasFileExtension("sdif"))
    {
        return mPropertyToJsonFrame.entry.isEnabled() || mPropertyToJsonMatrix.entry.isEnabled() || mPropertyToJsonRow.entry.isEnabled() || mPropertyToJsonColumn.entry.isEnabled();
    }
    return true;
}

nlohmann::json SdifConverter::Panel::getExtraInfo(double sampleRate) const
{
    if(!mFile.hasFileExtension("sdif"))
    {
        return {};
    }
    auto const selectedMatrixIndex = mPropertyToJsonMatrix.entry.getSelectedItemIndex();
    if(selectedMatrixIndex < 0 || static_cast<size_t>(selectedMatrixIndex) > mMatrixSigLinks.size())
    {
        return {};
    }
    auto const matrixIdentifier = mMatrixSigLinks.at(static_cast<size_t>(selectedMatrixIndex));
    switch(static_cast<uint32_t>(matrixIdentifier))
    {
        case SignatureIds::i1FQ0:
        {
            auto const desc = juce::String("{\"description\":{\"category\":\"Pitch\",\"defaultState\":{},\"details\":\"The fundamental estimation.\",\"extraO"
                                           "utputs\":[{\"description\":\"The confidence of the estimated periodicity\",\"hasKnownExtents\":true,\"identi"
                                           "fier\":\"confidence\",\"isQuantized\":false,\"maxValue\":1.0,\"minValue\":0.0,\"name\":\"Confidence\",\"quantizeSt"
                                           "ep\":0.0,\"unit\":\"\"}, {\"description\":\"The score of the estimated periodicity\",\"hasKnownExtents\":true,\""
                                           "identifier\":\"score\",\"isQuantized\":false,\"maxValue\":1.0,\"minValue\":0.0,\"name\":\"Score\",\"quantizeStep\":"
                                           "0.0,\"unit\":\"\"}, {\"description\":\"The real amplitude\",\"hasKnownExtents\":true,\"identifier\":\"realamplitu"
                                           "de\",\"isQuantized\":false,\"maxValue\":1.0,\"minValue\":0.0,\"name\":\"Real Amplitude\",\"quantizeStep\":0.0,\"un"
                                           "it\":\"\"}],\"name\":\"Fundamental\",\"output\":{\"binCount\":1,\"binNames\":[\"\"],\"description\":\"Pitch estimated "
                                           "from the input signal\",\"hasDuration\":false,\"hasFixedBinCount\":true,\"hasKnownExtents\":true,\"identifie"
                                           "r\":\"fundamental\",\"isQuantized\":false,\"maxValue\":MAXFREQ,\"minValue\":0.0,\"name\":\"Pitch\",\"quantizeStep\""
                                           ":0.0,\"sampleRate\":0.0,\"sampleType\":2,\"unit\":\"Hz\"}}}")
                                  .replace("MAXFREQ", juce::String(sampleRate / 2.0));
            std::istringstream stream(desc.toStdString());
            return nlohmann::json::parse(stream);
        }
        default:
            return {};
    }
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

    mPropertyToSdifFrame.entry.setText(SdifConverter::getString(frameIdentifier), juce::NotificationType::dontSendNotification);
    auto const& matrixEntries = mEntries.at(frameIdentifier);
    for(auto const& matrix : matrixEntries)
    {
        auto const matrixSignature = matrix.first;
        mMatrixSigLinks.push_back(matrixSignature);
        mPropertyToJsonMatrix.entry.addItem(SdifConverter::getString(matrixSignature), static_cast<int>(mMatrixSigLinks.size()));
    }
    mPropertyToJsonMatrix.entry.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
    mPropertyToJsonMatrix.entry.setEnabled(mPropertyToJsonMatrix.entry.getNumItems() > 1);

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
        selectedRowColumnUpdated();
        return;
    }

    auto const frameIdentifier = mFrameSigLinks.at(static_cast<size_t>(selectedFrameIndex));
    auto const matrixIdentifier = mMatrixSigLinks.at(static_cast<size_t>(selectedMatrixIndex));
    if(mEntries.count(frameIdentifier) == 0_z || mEntries.at(frameIdentifier).count(matrixIdentifier) == 0_z)
    {
        mPropertyToSdifMatrix.entry.setText("????", juce::NotificationType::dontSendNotification);
        mPropertyToJsonRow.entry.setEnabled(false);
        mPropertyToJsonColumn.entry.setEnabled(false);
        selectedRowColumnUpdated();
        return;
    }

    mPropertyToSdifMatrix.entry.setText(SdifConverter::getString(matrixIdentifier), juce::NotificationType::dontSendNotification);
    switch(static_cast<uint32_t>(matrixIdentifier))
    {
        case SignatureIds::i1FQ0:
            mPropertyToJsonRow.entry.setEnabled(false);
            mPropertyToJsonRow.setVisible(false);
            mPropertyToJsonColumn.entry.setEnabled(false);
            mPropertyToJsonColumn.setVisible(false);
            resized();
            break;
        default:
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
            mPropertyToJsonRow.entry.setEnabled(mPropertyToJsonRow.entry.getNumItems() > 1);

            if(matrixSize.second.size() > 1_z)
            {
                mPropertyToJsonColumn.entry.addItem("All", 1);
            }
            for(auto column = 0_z; column < matrixSize.second.size(); ++column)
            {
                mPropertyToJsonColumn.entry.addItem(matrixSize.second[column], static_cast<int>(column + 2));
            }
            mPropertyToJsonColumn.entry.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
            mPropertyToJsonColumn.entry.setEnabled(mPropertyToJsonColumn.entry.getNumItems() > 1);
            selectedRowColumnUpdated();
            break;
    }
}

void SdifConverter::Panel::selectedRowColumnUpdated()
{
    auto const row = mPropertyToJsonRow.entry.getSelectedId() - 1;
    auto const column = mPropertyToJsonColumn.entry.getSelectedId() - 1;
    if(row < 0 || column < 0)
    {
        MiscWeakAssert(false);
        notify();
        resized();
        return;
    }
    mPropertyToJsonColumn.entry.setItemEnabled(1, row != 0);
    if(row == 0 && column == 0)
    {
        mPropertyToJsonColumn.entry.setSelectedId(2, juce::NotificationType::dontSendNotification);
    }
    notify();
    resized();
}

void SdifConverter::Panel::notify()
{
    if(onUpdated != nullptr)
    {
        onUpdated();
    }
}

ANALYSE_FILE_END
