/* $Id: SdifString.c,v 3.13 2011-04-15 17:19:06 roebel Exp $
 *
 * IRCAM SDIF Library (http://www.ircam.fr/sdif)
 *
 * Copyright (C) 1998, 1999, 2000 by IRCAM-Centre Georges Pompidou, Paris, France.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * See file COPYING for further informations on licensing terms.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *  For any information regarding this and other IRCAM software, please
 *  send email to:
 *                            sdif@ircam.fr
 *
 *
 * String management
 *
 * author : Patrice Tisserand 2000
 *
 *
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.12  2005/05/23 17:52:53  schwarz
 * Unified error handling:
 * - SdifErrorEnum (global errors) integrated into SdifErrorTagET (file errors)
 * - no more SdifError.[ch], everything done by SdifErrMess.[ch]
 *
 * Revision 3.11  2005/04/07 15:56:48  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.10  2004/09/09 17:49:51  schwarz
 * Fixed low-level tokenising errors that were part of the parsing bug
 * that the type definition string had to end with whitespace.
 *
 * At the next bug with the type language parser and tokeniser, it should
 * be rewritten using flex/bison!
 *
 * Revision 3.9  2002/08/27 10:52:52  schwarz
 * String append from const char *
 * Threw out redundant function prototypes from SdifString.h,
 * because their place is sdif.h!
 *
 * Revision 3.8  2002/06/18 13:55:27  ftissera
 * Change malloc to calloc in SdifStringNew to get an empty string
 *
 * Revision 3.7  2001/05/02 09:34:48  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.6  2001/02/08 15:26:56  tisseran
 * Add a test on memory allocation in SdifStringNew (call perror if malloc return a NULL pointer).
 * Note on sdifextract.c:
 * 	- Rechange SdifSkipFrameData to SdifFSkipFrameData (change by axel have disappeared)
 *
 * Revision 3.5  2000/11/15 14:53:36  lefevre
 * no message
 *
 * Revision 3.4  2000/10/27  20:03:44  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.3.2.2  2000/08/21  21:35:52  tisseran
 * *** empty log message ***
 *
 * Revision 3.3.2.1  2000/08/21  14:04:23  tisseran
 * *** empty log message ***
 *
 * Revision 3.1  2000/07/18  15:08:40  tisseran
 * This release implements the New SDIF Specification (june 1999):
 * - Name Values Table are written in a 1NVT frame which contains a 1NVT matrix
 * - Frame and matrix type declaration are written in a 1TYP frame which contains a 1TYP matrix.
 * - Stream ID are written in a 1IDS frame which contains a 1IDS matrix.
 *
 * Read function accept the previous version of the specification (read a text frame without matrix) to be compatible with older SDIF files.
 *
 * SdifString.h and SdifString.c implements some string mangement (creation, destruction, append, test of end of string, getc, ungetc).
 *
 * WATCH OUT:
 *      We don't care about the old SDIF Specification (_SdifFormatVersion < 3)
 * To use _SdifFormatVersion < 3, get the previous release.
 */


#include <string.h>

#include "sdif_portability.h"
#include "sdif.h"
#include "SdifGlobals.h"


/* Function declaration */
/*DOC:
  Make a memory allocation for a SdifStringT structure.
  The size for the string is defined in SdifString.h; define _SdifStringGranule 128.
*/
SdifStringT * SdifStringNew(void)
{
  SdifStringT *SdifString;
  size_t SizeW = 0;
  size_t TotalSize = 0;

  SdifString = (SdifStringT *)malloc(sizeof(SdifStringT));
  TotalSize = _SdifStringGranule;
  
  SdifString->str = (char *)calloc(TotalSize, sizeof(char));
  if (SdifString->str == NULL) /* Check result of memory allocation */
  {
      /* Error with memory allocation */
      perror("SdifStringNew memory allocation: ");
      /* Call SdifError checker (not yet...) */
      _SdifError(eAllocFail,"SdifString Memory Allocation");
  }
  SdifString->SizeW = SizeW;
  SdifString->TotalSize = TotalSize;
  SdifString->NbCharRead = 0;

  return(SdifString);
}


/*DOC:
  Free memory allocated for SdifString.
*/
void SdifStringFree(SdifStringT *SdifString)
{
  free(SdifString->str);
  free(SdifString);

  return;
}


/*DOC:
  Append a string to another one.
  Manage memory reallocation.
  Return a boolean for the succes of the function's call.
*/
int SdifStringAppend(SdifStringT * SdifString, const char *strToAppend)
{
  char   *tmpStr; /* Temporary string for memory allocation */
  char   *str = SdifString->str;
  size_t SizeW = SdifString->SizeW;
  size_t TotalSize = SdifString->TotalSize;
  size_t strToAppendLen = strlen(strToAppend);
  size_t memoryNeeded;
  int    memoryQuantuum;

  /* enough space to append (including terminating zero)? */
  if ((TotalSize - SizeW) < strToAppendLen + 1) 
    {
      /* Need a memory reallocation */
      memoryQuantuum =(int) ((SizeW - TotalSize + strToAppendLen) / _SdifStringGranule) ;
      memoryNeeded = (memoryQuantuum + 1) * _SdifStringGranule; 
      tmpStr = (char *)realloc(str, TotalSize + memoryNeeded);
      if (!tmpStr)
	{
	  fprintf(stderr,"No more memory available!!!\n");
	  return(0);
	}
      str = tmpStr;
      TotalSize += memoryNeeded;
    }

  /* actually append string */
  strcpy(str + SizeW, strToAppend);
  SizeW += strToAppendLen;

  /* Variable reaffectation */
  SdifString->str = str;
  SdifString->SizeW = SizeW;
  SdifString->TotalSize = TotalSize;
  
  return(1);
}

/*DOC:
  Read the current char (= fgetc).

  TODO: return eEof when string is used up, rewrite parser more simply!
  (read whole 1TYP string matrix, parse from mem only)
*/
int SdifStringGetC(SdifStringT *SdifString)
{
  int NbCharRead = SdifString->NbCharRead;
  int chint;

  chint = (int) SdifString->str[NbCharRead];

  SdifString->NbCharRead++;

  return chint;
}

/*DOC:
  Equivalent of ungetc: put one character back into string, clear EOS condition
*/
int SdifStringUngetC(SdifStringT *SdifString)
{
  if (SdifString->NbCharRead == 0)
      return eFalse;
  else
  {
      SdifString->NbCharRead--;
      return eTrue;
  }
}


/*DOC:
  Test the end of the string (= feof)
*/
int SdifStringIsEOS(SdifStringT *SdifString)
{
  int NbCharRead = SdifString->NbCharRead;
  int SizeW = SdifString->SizeW;
  
  if (NbCharRead >= SizeW)
    return eTrue;
  else
    return eFalse;
}
