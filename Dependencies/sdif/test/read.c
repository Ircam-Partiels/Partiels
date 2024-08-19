#include <sdif.h>

int main(int argc, char *argv[])
{
    if(argc < 1)
    {
        printf("Missing argument for file input\n");
        return 1;
    }
    
    // Initialize SDIF library
    SdifGenInit(NULL);
    
    // Open the SDIF file
    SdifFileT* file = SdifFOpen(argv[1], eReadFile);
    if(file == NULL)
    {
        printf("Can't open input file '%s'.\n", file->Name);
        SdifGenKill();
        return 1;
    }
    printf("Read file '%s'\n", file->Name);
    
    {
        //Get list of all name value table list
        SdifListT* nvtList   = SdifNameValueTableList(SdifFNameValueList(file));
        printf("Number of name value tables %d\n", SdifListGetNbData(nvtList));
        
        // For each name value table
        SdifHashTableIteratorT* nvtIter = SdifCreateHashTableIterator(NULL);
        SdifListInitLoop(nvtList);
        while(SdifListIsNext(nvtList))
        {
            SdifNameValueTableT* currentTable = (SdifNameValueTableT *)SdifListGetNext(nvtList);
            SdifHashTableT* hashTable = SdifNameValueTableGetHashTable(currentTable);
            
            int numTables = SdifNameValueTableGetNumTable(currentTable);
            printf("NameValueTable Number %d Stream %d Size %d:\n", numTables, SdifNameValueTableGetStreamID(currentTable), SdifHashTableGetNbData(hashTable));
            
            SdifHashTableIteratorInitLoop(nvtIter, hashTable);
            while(SdifHashTableIteratorIsNext(nvtIter))
            {
                SdifNameValueT *nv = (SdifNameValueT *)SdifHashTableIteratorGetNext(nvtIter);
                printf("%s\t%s;\n", SdifNameValueGetName(nv), SdifNameValueGetValue(nv));
            }
        }
        
        SdifKillHashTableIterator(nvtIter);
    }
    
    int endOfFile = 0;
    size_t bytesRead = 0;
    
    // Read file's header
    {
        size_t numBytes = SdifFReadGeneralHeader(file);
        if(numBytes == 0)
        {
            printf("Can't read header info of input file.\n");
            SdifFClose(file);
            SdifGenKill();
            return 1;
        }
        bytesRead += numBytes;
        printf("%s\n", SdifSignatureToString(eSDIF));
    }
    {
        size_t numBytes = SdifFReadAllASCIIChunks(file);
        if(numBytes == 0)
        {
            printf("Can't read ASCII header info of input file.\n");
            SdifFClose(file);
            SdifGenKill();
            return 1;
        }
        bytesRead += numBytes;
        
        if (SdifNameValuesLIsNotEmpty(file->NameValues))
        {
            SdifListInitLoop(file->NameValues->NVTList);
            while(SdifListIsNext(file->NameValues->NVTList))
            {
                file->NameValues->CurrNVT = (SdifNameValueTableT *)SdifListGetNext(file->NameValues->NVTList);
                printf("%s\n", SdifSignatureToString(e1NVT));
                size_t          SizeW = 0;
                SdifUInt4       iNV;
                SdifHashNT     *pNV;
                SdifHashTableT *HTable;
                
                HTable = file->NameValues->CurrNVT->NVHT;
                
                printf("{\n");
                for(iNV=0; iNV<HTable->HashSize; iNV++)
                {
                    for (pNV = HTable->Table[iNV]; pNV; pNV = pNV->Next)
                    {
                        printf("%s\t", ((SdifNameValueT *)pNV->Data)->Name);
                        printf("%s;\n", ((SdifNameValueT *)pNV->Data)->Value);
                    }
                }
                printf("}");
                printf("\n");
            }
        }
        
        if ((SdifExistUserMatrixType(file->MatrixTypesTable)) || (SdifExistUserFrameType(file->FrameTypesTable)))
        {
            printf("Unsupported user-defined values");
            printf("\n");
        }
        
        if (SdifStreamIDTableGetNbData(file->StreamIDsTable) > 0)
        {
            printf("\n");
        }
    }
    
    if(SdifFCurrSignature(file) == eEmptySignature)
    {
        printf("Signature: %s\n", SdifSignatureToString(file));
        SdifFClose(file);
        return 0;
    }
    
//    fprintf(SdifF->TextStream, "\n%s\n", SdifSignatureToString(eSDFC));
//    SizeR += SdifFConvToTextAllFrame(SdifF);
//    fprintf(SdifF->TextStream, "\n%s\n", SdifSignatureToString(eENDC));
//    
//    
//    
    
    SdifSignature signature  = SdifSignatureConst('1', 'F', 'Q', 'B');
    char* signatureStr = SdifSignatureToString(signature);
    
    while(!endOfFile  &&  SdifFLastError(file) == NULL)
    {
        bytesRead += SdifFReadFrameHeader(file);
        
        // Search for a frame we're interested in */
        while(!SdifFCurrFrameIsSelected(file))// || SdifFCurrSignature(file) != signature)
        {
            SdifFSkipFrameData(file);
            endOfFile = SdifFGetSignature(file, &bytesRead);
            if(endOfFile == eEof)
            {
                break;
            }
            bytesRead += SdifFReadFrameHeader(file);
        }
        
        if(!endOfFile)
        {    /* Access frame header information */
            SdifFloat8 time = SdifFCurrTime(file);
            SdifSignature sig = SdifFCurrFrameSignature(file);
            SdifUInt4 streamid = SdifFCurrID(file);
            SdifUInt4 nmatrix = SdifFCurrNbMatrix (file);
            int        m;
            printf("Frame Signature %s\n", SdifSignatureToString(sig));
            printf("Num matrix %i\n", (int)nmatrix);
            /* Read all matrices in this frame matching the selection. */
            for (m = 0; m < nmatrix; m++)
            {
                bytesRead += SdifFReadMatrixHeader(file);
                printf("Matrix %i\n", (int)m);
                if(SdifFCurrMatrixIsSelected (file))
                {    /* Access matrix header information */
                    SdifSignature    matsig   = SdifFCurrMatrixSignature (file);
                    SdifInt4 nrows = SdifFCurrNbRow (file);
                    SdifInt4 ncols = SdifFCurrNbCol (file);
                    SdifDataTypeET    type  = SdifFCurrDataType (file);
                    
                    int        row, col;
                    SdifFloat8    value;
                    printf("Signature %s\n", SdifSignatureToString(matsig));
                    printf("Nuw Rows %i and Cols %i\n", (int)nrows, (int)ncols);
                    for (row = 0; row < nrows; row++)
                    {
                        bytesRead += SdifFReadOneRow (file);
                        
                        for (col = 1; col <= ncols; col++)
                        {
                            /* Access value by value... */
                            value = SdifFCurrOneRowCol (file, col);
                            printf("%f ", (float)value);
                            /*  Do something with the data... */
                        }
                    }
                    printf("\n");
                }
                else
                {
                    bytesRead += SdifFSkipMatrixData(file);
                }
                
                bytesRead += SdifFReadPadding(file, SdifFPaddingCalculate(file->Stream, bytesRead));
            }
            
            /* read next signature */
            endOfFile = SdifFGetSignature(file, &bytesRead) == eEof;
        }
    }
    
    if(SdifFLastError(file))
    {
        return 1;
    }

    
    SdifFClose(file);
    return 0;
}
