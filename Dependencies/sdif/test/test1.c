#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sdif.h>
#include "utils.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#define FILE_NAME "test_file1.sdif"

int
main(void)
{
  char *types = NULL;
  
  SdifFileT *MySdifFileToWrite;
  SdifFileT *MySdifFileToRead;
  size_t bytesWritten = 0;
  size_t bytesRead = 0;

#if HAVE_UNISTD_H
  sleep(1); /* Wait one second */
#endif
  
  fprintf(stderr,"\n");
  fprintf(stderr,"**********************************************************\n");
  fprintf(stderr,"***  Test on write and read SDIF file General Header:  ***\n");
  fprintf(stderr,"***             Output File: %s           ***\n",FILE_NAME);
  fprintf(stderr,"**********************************************************\n");
  
  /* init */
  fprintf(stderr,"\nLibrary Initialisation (SdifGenInit)\n");
  if (!types)
  {
      char types2[2] = "";
      SdifGenInit (types2);
  }
  else
  {
      SdifGenInit (types);
  }

  /* write */
  fprintf(stderr,"\nWRITE SEQUENCE\n");
  fprintf(stderr,"Open file %s in write mode (SdifFOpen) \n",FILE_NAME);
  MySdifFileToWrite = SdifFOpen(FILE_NAME,eWriteFile);
  
  bytesWritten += writeFileHeader(MySdifFileToWrite);
  
  fprintf(stderr,"Close File (SdifFClose) \n");
  SdifFClose(MySdifFileToWrite);

  /* read */
  fprintf(stderr,"\nREAD SEQUENCE \n");
  fprintf(stderr,"Open file %s in read mode (SdifFOpen)\n",FILE_NAME);
  MySdifFileToRead = SdifFOpen(FILE_NAME,eReadFile);

  bytesRead += readFileHeader(MySdifFileToRead);
  
  fprintf(stderr,"Close File (SdifFClose)\n");
  SdifFClose(MySdifFileToRead);

  /* check */
  fprintf(stderr,"\nCHECK DIFFERENCIES: ");
  if (bytesWritten == bytesRead)
    fprintf(stderr,"NUMBER OF BYTES WRITTEN AND READ ARE THE SAME\n");
  else
    {
      fprintf(stderr,"Difference (written-read): %u\n",(unsigned int) (bytesWritten-bytesRead));
      SdifGenKill();
      fprintf(stderr,"\n");      
      return 1;
    }

  /* deinit */
  fprintf(stderr,"Librairy Deinitialisation\n");
  SdifGenKill();

  fprintf(stderr,"\n");
  return 0;
}

  
  
  
