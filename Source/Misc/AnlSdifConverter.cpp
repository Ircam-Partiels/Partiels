#include "AnlSdifConverter.h"
#include <sdif.h>

ANALYSE_FILE_BEGIN

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

    nlohmann::json json;

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
            return juce::Result::fail("Can't fint frame signature");
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
    stream << json << std::endl;
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

ANALYSE_FILE_END
