/* $Id: SdifHighLevel.h,v 3.7 2004-09-09 17:53:07 schwarz Exp $
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
LIBRARY
 *
 * SdifHighLevel.h	8.12.1999	Diemo Schwarz
 *
LOG
 * $Log: not supported by cvs2svn $
 * Revision 3.6  2001/05/02 09:34:44  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.5  2000/11/21 14:51:50  schwarz
 * - sdif.h is now included by all sdif/Sdif*.c files.
 * - Removed all public typedefs, enums, structs, and defines from the
 *   individual sdif/Sdif*.h files, because they were duplicated in sdif.h.
 * - Todo: Do the same for the function prototypes, decide which types and
 *   prototypes really need to be exported.
 * - Removed SdifFileStruct.h.
 * - Preliminary new version of SdiffGetPos, SdiffSetPos.  They used the
 *   type fpos_t, which is no longer a long on RedHat 7 Linux.
 *
 * Revision 3.4  2000/10/27 20:03:36  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.3  2000/08/22  13:38:34  schwarz
 * First alpha version of SdifFReadNextSelectedFrameHeader.
 *
 * Revision 3.2.2.1  2000/08/21  21:35:30  tisseran
 * *** empty log message ***
 *
 * Revision 3.2  2000/04/11  14:44:28  schwarz
 * SdifFWriteTextMatrix, more docs.
 *
 * Revision 3.1  2000/03/01  11:20:21  schwarz
 * Added preliminary sketch of SdifHighLevel
 *
 */


#ifndef _SdifHighLevel_H_
#define _SdifHighLevel_H_


#include "SdifGlobals.h"


#if 0	/* TBI */

/*
//FUNCTION GROUP: to be implemented / TBI
*/


/*DOC: 
  Write whole matrix, given as separate columns in array "columns" of
  pointer to "DataType".  Each columns [i], i = 0..NbCol-1, points to 
  NbRow * SdifSizeofDataType (DataType) bytes.  
  TBI 
*/
SdifFWriteMatrixColumns (SdifFileT     *file,
			 SdifSignature  Signature,
			 SdifDataTypeET DataType,
			 SdifUInt4      NbRow,
			 SdifUInt4      NbCol,
			 void	       *columns []);


SdifFWriteTextMatrix (SdifFileT f, SdifSignature matrixsig, const char *str)
{
    /* convert to UTF-8 */
    SdifConvertToUTF8 (str, utfstr);
    nbytes = strlen (utfstr);
    SdifFSetCurrMatrixHeader (f, matrixsig, eText, nbytes, 1);
    SdifFWriteMatrixHeader (f);

    /* write text data and padding */
}


/* see SdifFRead */

SdifFReadMatrixAs_TYPE_ ();

#if 0
/* Alternative: extend SdifDataTypeET by */
  eTranslate = 0x1000, /* flag to be or'ed with data type: Translate on read */
  eTypeMask  = 0x0fff  /* mask to eliminate flags */
#endif


/*DOC: 
  Reads matrix header and data into memory allocated by the library,
  accessible by SdifFCurrMatrixData (). */
int SdifFReadMatrix (SdifFileT *file);

/* with type conversion */
int SdifFReadMatrixAs (SdifFileT *file, SdifDataTypeET as_type);

/*? text special: return allocated, decoded c-string, to be free'd by caller */
char *SdifFReadTextMatrix (SdifFileT *file);

/*DOC: 
  Reads matrix data into memory pointed to by target, which must point
  to at least nbrow * nbcol * size of datatype bytes of memory.  If
  target is NULL, the library will allocate enough space for the data
  of one matrix, accessible by SdifFCurrMatrixData ().

  [Precondition:] 
  Matrix header must have been read with SdifFReadMatrixHeader.  */

/* with type conversion */
int SdifFReadMatrixDataAs (SdifFileT *file, void *target,
			   SdifDataTypeET as_type);


void *SdifGetColumn ();



// Error handling


int /*bool*/ SdifFCheckStatus (SdifFileT *file)
{
  return (SdifLastError (file->ErrorList)) == NULL);
}


int /*bool*/ SdifFCheckStatusPrint (SdifFileT *file)
{
  SdifError err = SdifLastError (file->ErrorList));
  if (err != eNoError)
     print (SdifFsPrintFirstError (..., file, ...);
  return err == NULL;
}


/* --> test in SdifFReadGeneralHeader  (file) + SdifFReadAllASCIIChunks (file)
   if (!SdifFCheckStatus (file))
      SdifWarningAdd ("Followup error");
*/


#endif /* TBI */

#endif /* _SdifHighLevel_H_ */
