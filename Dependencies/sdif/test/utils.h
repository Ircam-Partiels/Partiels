#ifndef _UTILS_H_
#define _UTILS_H_ 1

#include "sdif.h"

int writeFileHeader(SdifFileT *fileToWrite);
int writeFileASCIIChunks(SdifFileT *fileToWrite);

int readFileHeader(SdifFileT *fileToRead);
int readFileASCIIChunks(SdifFileT *fileToRead);


#endif /* _UTILS_H_ */
