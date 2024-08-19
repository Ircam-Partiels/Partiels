/*
 * $Log: not supported by cvs2svn $
 * Revision 1.5  2004/10/07 15:37:30  roebel
 * Fixed for new SdifStringToNV.
 *
 * Revision 1.4  2002/05/24 19:41:34  ftissera
 * Change code to be compatible with C++
 *
 * Revision 1.3  2001/05/04 18:08:01  schwarz
 * Added test for SdifNameValuesLPutCurrNVTTranslate.
 *
 * Revision 1.2  2000/10/27 20:04:02  roebel
 * autoconf merged back to main trunk
 *
 * Revision 1.1.2.3  2000/08/21  18:34:26  tisseran
 * *** empty log message ***
 *
 * Revision 1.1.2.2  2000/08/21  17:00:52  tisseran
 * *** empty log message ***
 *
 * Revision 1.1.2.1  2000/08/21  13:55:43  tisseran
 * *** empty log message ***
 *
 *
 * This program test the matrix and frame type declaration in SDIF file.
 * 
 *
 * $Id: test2.c,v 1.6 2007-11-27 12:19:16 roebel Exp $
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h> /* To use time (Date in 1NVT) */

#include <sdif.h>
#include "utils.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#define lengthString 2000

#define FILE_NAME "test_file2.sdif"
#define FIELD1 "TableName"
#define FIELD2 "StreamID"
#define FIELD3 "WrittenBy"
#define FIELD4 "Date"


int
main(void)
{
  SdifFileT *MySdifFileToWrite;
  SdifFileT *MySdifFileToRead;

  char num[10];
  time_t now = time(NULL); /* For Date in 1NVT */
  int nbuser;
  int index;
  char *types = NULL;
  SdifNameValueT   *nv;

  size_t bytesWritten = 0;
  size_t bytesRead = 0;

#if HAVE_UNISTD_H
  sleep(1); /* Wait one second */
#endif

  fprintf(stderr,"\n");
  fprintf(stderr,"*************************************\n");
  fprintf(stderr,"***   Test on Name Value Table:   ***\n");
  fprintf(stderr,"***  Output file: %s ***\n",FILE_NAME);
  fprintf(stderr,"*************************************\n");

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

  fprintf(stderr,"\nWRITE SEQUENCE\n");

  fprintf(stderr,"Open file %s in write mode (SdifFileT * SdifFOpen(char *,eWriteFile)\n"
	  ,FILE_NAME);
  MySdifFileToWrite = SdifFOpen(FILE_NAME,eWriteFile);

  /* Index incrementation */
  index = SdifFAddUserData(MySdifFileToWrite, (void *)12);

  /* */
  nbuser = MySdifFileToWrite->NbUserData;  
  
  /* Information Table Chunk Creation */
  SdifNameValuesLNewTable(MySdifFileToWrite->NameValues, _SdifNVTStreamID);

  sprintf (num, "%u", (SdifUInt4) SdifFGetUserData (MySdifFileToWrite, index));

  SdifNameValuesLPutCurrNVT(MySdifFileToWrite->NameValues,FIELD1,"prefere");
  SdifNameValuesLPutCurrNVT(MySdifFileToWrite->NameValues,FIELD2,num);
  SdifNameValuesLPutCurrNVTTranslate(MySdifFileToWrite->NameValues,FIELD3,"test 2,;\n\t{}:x");
  SdifNameValuesLPutCurrNVTTranslate(MySdifFileToWrite->NameValues,FIELD4,ctime(&now));

  bytesWritten += writeFileHeader(MySdifFileToWrite);

  bytesWritten += writeFileASCIIChunks(MySdifFileToWrite);

  fprintf(stderr,"Number of bytes written: %u\n",(unsigned int)bytesWritten);
  fprintf(stderr,"File closing... (SdifFClose(SdifFileT *)\n");
  SdifFClose(MySdifFileToWrite);

/************************************/
  fprintf(stderr,"\nREAD SEQUENCE\n");
  fprintf(stderr,"Open file %s in readMode (SdifFileT * SdifFOpen(SdifFileT *, eReadFile)\n"
	  ,FILE_NAME);
  MySdifFileToRead = SdifFOpen(FILE_NAME,eReadFile);

  bytesRead += readFileHeader(MySdifFileToRead);
  
  bytesRead += readFileASCIIChunks(MySdifFileToRead);

  fprintf(stderr,"\nCHECK NVT: ");
  nv = SdifNameValuesLGet(SdifFNameValueList(MySdifFileToRead), FIELD3);
  if (strcmp(nv->Value, "test 2,;  {}:x") != 0)
  {
      fprintf(stderr,"Difference in NVT: %s unexpected\n", nv->Value);
      SdifGenKill();
      fprintf(stderr,"\n");
      return 1;
  }
  else
      fprintf(stderr, "OK\n\n");

  fprintf(stderr,"Total number of bytes read: %u \n",(unsigned int)bytesRead);
  fprintf(stderr,"File closing...\n");
  SdifFClose(MySdifFileToRead);

/************************************/  
  fprintf(stderr,"\nCHECK DIFFERENCIES: ");
  if (bytesWritten == bytesRead)
    fprintf(stderr,"NUMBER OF BYTES WRITTEN AND READ ARE THE SAME\n");
  else
    {
      fprintf(stderr,"Difference (written-read): %u\n",(unsigned int)(bytesWritten-bytesRead));
      SdifGenKill();
      fprintf(stderr,"\n");
      return 1;
    }
  
  fprintf(stderr,"\nDeinitialisation de la librairie\n");
  SdifGenKill();
  fprintf(stderr,"\n");

  return 0;
}
