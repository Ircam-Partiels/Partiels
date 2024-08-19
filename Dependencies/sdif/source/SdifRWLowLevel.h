/* $Id: SdifRWLowLevel.h,v 3.15 2005-10-21 14:32:30 schwarz Exp $
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
 * SdifRWLowLevel.h
 *
 *
 * Read Write Low Level. Machine sex, little-big endian control
 *
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.14  2005/05/24 09:36:09  roebel
 *
 * Fixed last checkin comment which turned out to be the start of
 * a c-comment.
 *
 * Revision 3.13  2005/05/23 19:14:54  schwarz
 * - Sdiffread* / Sdiffwrite* functions with SdifFileT instead of FILE *
 *   -> eof error reporting makes more sense
 * - more cleanup of sdif.h, above functions are private in SdifRWLowLevel.h
 * - eEof becomes error 4 to be distinguishable from ascii chars
 * - SdifFScanNameValueLCurrNVT reimplemented for ascii only
 *
 * Revision 3.12  2005/04/07 15:56:48  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.11  2003/05/01 18:50:52  roebel
 * Removed declarations that are now in sdif.h
 *
 * Revision 3.10  2001/05/02 09:34:47  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.9  2000/10/27 20:03:42  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.8.2.3  2000/08/21  21:35:45  tisseran
 * *** empty log message ***
 *
 * Revision 3.8.2.2  2000/08/21  18:34:13  tisseran
 * Add SdifSkipASCIIUntilfromSdifString function (same as SdifSkipASCIIUntil).
 * Add SdifFSkip for SdifSkip for (functions SdifSkip doesn't respect function nomemclature => obsolete).
 *
 * Revision 3.8.2.1  2000/08/21  14:04:21  tisseran
 * *** empty log message ***
 *
 * Revision 3.8  2000/07/18  15:08:39  tisseran
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
 *
 * Revision 3.7  2000/05/15  16:23:11  schwarz
 * Avoided avoidable warnings.
 *
 * Revision 3.6  1999/10/15  12:23:48  schwarz
 * Added SdifStringToNV.
 *
 * Revision 3.5  1999/10/13  16:05:57  schwarz
 * Changed data type codes (SdifDataTypeET) to SDIF format version 3, as
 * decided with Matt Wright June 1999, added integer data types.
 * Added writing of 1NVT with real frame header (but data is still not in
 * matrices).
 * The data type handling makes heavy use of code-generating macros,
 * called for all data types with the sdif_foralltypes macro, thus
 * adding new data types is easy.
 *
 * Revision 3.4  1999/09/28  13:09:11  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.3  1999/09/20  13:23:35  schwarz
 * Optimized SdifStringToSignature.
 *
 * Revision 3.2  1999/08/31  10:05:48  schwarz
 * Extracted function SdifStringToSignature from SdiffGetSignature.
 *
 * Revision 3.1  1999/03/14  10:57:19  virolle
 * SdifStdErr add
 *
 * Revision 2.1  1998/12/21  18:27:38  schwarz
 * Inserted copyright message.
 *
 * Revision 2.0  1998/11/29  11:42:05  virolle
 * - New management of interpretation errors.
 * - Alignement of frames with CNMAT (execpt specials Chunk 1NVT, 1TYP, 1IDS).
 * _ Sdif Header File has a Sdif format version.
 * - Matrices order in frames is not important now. (only one occurence of
 *   a Matrix Type in a Frame Type declaration )
 * - Hard coded predefined types more dynamic management.
 * - Standart streams (stdin, stdout, stderr) set as binary for Windows32 to
 *   have exactly the same result on each plateforme.
 *
 * Revision 1.5  1998/11/10  15:31:55  schwarz
 * Removed all 'extern' keywords for prototypes, since this is redundant
 * (function prototypes are automatically linked extern), and it
 * prohibits cocoon from generating an entry in the HTML documentation
 * for this function.
 */

#ifndef _SdifRWLowLevel_
#define _SdifRWLowLevel_


#include "SdifGlobals.h"
#include <stdio.h>
#include "sdif.h"


#define _SdifPaddingChar  '\0'
#define _SdifReservedChars  ",;{}:"


size_t Sdiffread  (void *ptr, size_t size, size_t nobj, SdifFileT *file);
size_t Sdiffwrite (void *ptr, size_t size, size_t nobj, SdifFileT *file);

/* Read, return the number of objects */

size_t SdiffReadChar   (SdifChar   *ptr, size_t nobj, SdifFileT *file);
size_t SdiffReadInt1   (SdifInt1   *ptr, size_t nobj, SdifFileT *file);
size_t SdiffReadInt2   (SdifInt2   *ptr, size_t nobj, SdifFileT *file);
size_t SdiffReadUInt1  (SdifUInt1  *ptr, size_t nobj, SdifFileT *file);
size_t SdiffReadUInt2  (SdifUInt2  *ptr, size_t nobj, SdifFileT *file);
size_t SdiffReadInt4   (SdifInt4   *ptr, size_t nobj, SdifFileT *file);
size_t SdiffReadUInt4  (SdifUInt4  *ptr, size_t nobj, SdifFileT *file);
size_t SdiffReadFloat4 (SdifFloat4 *ptr, size_t nobj, SdifFileT *file);
size_t SdiffReadFloat8 (SdifFloat8 *ptr, size_t nobj, SdifFileT *file);
SdifErrorTagET SdiffReadSignature (SdifSignature *Signature,  SdifFileT *file, size_t *n);



/* Write, return the number of objects */

size_t SdiffWriteChar   (SdifChar   *ptr, size_t nobj, SdifFileT *file);
size_t SdiffWriteInt1   (SdifInt1   *ptr, size_t nobj, SdifFileT *file);
size_t SdiffWriteInt2   (SdifInt2   *ptr, size_t nobj, SdifFileT *file);
size_t SdiffWriteUInt1  (SdifUInt1  *ptr, size_t nobj, SdifFileT *file);
size_t SdiffWriteUInt2  (SdifUInt2  *ptr, size_t nobj, SdifFileT *file);
size_t SdiffWriteInt4   (SdifInt4   *ptr, size_t nobj, SdifFileT *file);
size_t SdiffWriteUInt4  (SdifUInt4  *ptr, size_t nobj, SdifFileT *file);
size_t SdiffWriteFloat4 (SdifFloat4 *ptr, size_t nobj, SdifFileT *file);
size_t SdiffWriteFloat8 (SdifFloat8 *ptr, size_t nobj, SdifFileT *file);

size_t SdiffWriteSignature (SdifSignature *Signature, SdifFileT *file);
size_t SdiffWriteString (char* ptr, SdifFileT *file);

/*
 *size_t SdiffReadUInt8  (SdifUInt8  *ptr, size_t nobj, SdifFileT *file);
 *size_t SdiffWriteUInt8  (SdifUInt8  *ptr, size_t nobj, SdifFileT *file);
 */

/**Ascii**/
/* fGet --> return the last char
 */
size_t SdiffReadSpace   (FILE* fr);

size_t SdiffReadSpacefromSdifString(SdifStringT *SdifString);


#endif /* _SdifRWLowLevel_ */
