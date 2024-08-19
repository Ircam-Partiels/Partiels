#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sdif.h>
#include "utils.h"

int writeFileHeader(SdifFileT *fileToWrite)
{
  size_t nbBytesWritten = 0;
  fprintf(stderr,"Write General Header (SdifFWriteGeneralHeader) \n");
  nbBytesWritten += SdifFWriteGeneralHeader(fileToWrite);
  fprintf(stderr,"Number of bytes written: %u\n",(unsigned int)nbBytesWritten);
  return nbBytesWritten; 
}

int writeFileASCIIChunks(SdifFileT *fileToWrite)
{
  size_t nbBytesWritten = 0;
  fprintf(stderr,"Write ASCII Chunks (SdifFWriteAllASCIIChunks)\n");
  nbBytesWritten += SdifFWriteAllASCIIChunks(fileToWrite);
  fprintf(stderr,"Number of bytes written: %u\n",(unsigned int)nbBytesWritten);  
  return nbBytesWritten;
}

int writeFrameHeader(SdifFileT *fileToWrite)
{
  size_t nbBytesWritten = 0;
  fprintf(stderr,"Write frame header (SdifFWriteFrameHeader)\n");
  nbBytesWritten += SdifFWriteFrameHeader(fileToWrite);
  fprintf(stderr,"Number of bytes written: %u\n",(unsigned int)nbBytesWritten);
  return nbBytesWritten;
}

int readFileHeader(SdifFileT *fileToRead)
{
  size_t nbBytesRead = 0;

  fprintf(stderr, "Read General Header (SdifFReadGeneralHeader) --> ");
  nbBytesRead += SdifFReadGeneralHeader(fileToRead);
  fprintf(stderr, "%s\nNumber of bytes read: %u\n", 
	  SdifSignatureToString(fileToRead->CurrSignature), 
	  (unsigned int)nbBytesRead);

  return nbBytesRead;
}

int readFileASCIIChunks(SdifFileT *fileToRead)
{
  size_t nbBytesRead = 0;
  fprintf(stderr,"Read ASCII Chunks (SdifFReadAllASCIIChunks) \n");
  nbBytesRead += SdifFReadAllASCIIChunks(fileToRead);
  fprintf(stderr,"Number of bytes read: %u\n",(unsigned int)nbBytesRead);
  return nbBytesRead;   
}


