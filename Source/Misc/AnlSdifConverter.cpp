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

ANALYSE_FILE_END
