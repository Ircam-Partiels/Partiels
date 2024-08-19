/* $Id: SdifFGet.h,v 3.13 2005-05-24 09:34:32 roebel Exp $
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

LIBRARY
 * SdifFGet.h
 *
 * F : SdifFileT*, Get : ascii frames reading,
 * common to read a sdif file and a pseudo-sdif text file
 * 'verbose' allows to choise if the file is a sdif file or a pseudo-sdif text file
 *
 * author: Dominique Virolle 1997
 *

DESCRIPTION 

   LECTURE

   <strong>Remarque</strong>: En lecture, on a toujours une
   avance sur la signature des chunks ou des frames. Ceci permet
   d'orientée la lecture suivant le type de données: chunk ou frame.

LOG
 * $Log: not supported by cvs2svn $
 * Revision 3.12  2005/05/23 19:17:53  schwarz
 * - Sdiffread/Sdiffwrite functions with SdifFileT instead of FILE *
 *   -> eof error reporting makes more sense
 * - more cleanup of sdif.h, above functions are private in SdifRWLowLevel.h
 * - eEof becomes error 4 to be distinguishable from ascii chars
 * - SdifFScanNameValueLCurrNVT reimplemented for ascii only
 *
 * Revision 3.11  2005/04/07 15:56:46  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.10  2003/08/06 15:11:45  schwarz
 * Finally removed obsolete functions (like SdifSkip...).
 *
 * Revision 3.9  2002/06/18 13:55:54  ftissera
 * Move SdifFGetAllTypefromSdifString declaration from SdifFGet.h to sdif.h
 *
 * Revision 3.8  2001/05/02 09:34:41  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.7  2000/11/21 14:51:48  schwarz
 * - sdif.h is now included by all sdif/Sdif*.c files.
 * - Removed all public typedefs, enums, structs, and defines from the
 *   individual sdif/Sdif*.h files, because they were duplicated in sdif.h.
 * - Todo: Do the same for the function prototypes, decide which types and
 *   prototypes really need to be exported.
 * - Removed SdifFileStruct.h.
 * - Preliminary new version of SdiffGetPos, SdiffSetPos.  They used the
 *   type fpos_t, which is no longer a long on RedHat 7 Linux.
 *
 * Revision 3.6  2000/10/27 20:03:27  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.5.2.2  2000/08/21  21:35:04  tisseran
 * *** empty log message ***
 *
 * Revision 3.5.2.1  2000/08/21  14:04:07  tisseran
 * *** empty log message ***
 *
 * Revision 3.5  2000/07/18  15:08:29  tisseran
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
 * Revision 3.4  2000/04/11  14:31:20  schwarz
 * Read/write NVT as frame with 1 text matrix, conforming to SDIF spec.
 *
 * Revision 3.3  1999/09/28  13:08:52  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.2  1999/08/25  18:32:34  schwarz
 * Added cocoon-able comments with sentinel "DOC:" (on a single line).
 *
 * Revision 3.1  1999/03/14  10:56:38  virolle
 * SdifStdErr add
 *
 * Revision 2.3  1999/01/23  15:55:39  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:23  virolle
 * General Lists, and special chunk preparation to become frames
 *
 * Revision 2.1  1998/12/21  18:27:06  schwarz
 * Inserted copyright message.
 *
 * Revision 2.0  1998/11/29  11:41:29  virolle
 * - New management of interpretation errors.
 * - Alignement of frames with CNMAT (execpt specials Chunk 1NVT, 1TYP, 1IDS).
 * _ Sdif Header File has a Sdif format version.
 * - Matrices order in frames is not important now. (only one occurence of
 *   a Matrix Type in a Frame Type declaration )
 * - Hard coded predefined types more dynamic management.
 * - Standart streams (stdin, stdout, stderr) set as binary for Windows32 to
 *   have exactly the same result on each plateforme.
 *
 * Revision 1.2  1998/11/10  15:31:40  schwarz
 * Removed all 'extern' keywords for prototypes, since this is redundant
 * (function prototypes are automatically linked extern), and it
 * prohibits cocoon from generating an entry in the HTML documentation
 * for this function.
 */


#ifndef _SDIFFGET_H
#define _SDIFFGET_H

#include <stdio.h>
#include <sdif.h>
#include "SdifGlobals.h"



int    SdifFGetOneNameValue     (SdifFileT *SdifF, int Verbose, size_t *NbCharRead);
size_t SdifFGetNameValueLCurrNVT(SdifFileT *SdifF);
int    SdifFNameValueLCurrNVTfromString (SdifFileT *SdifF, char *str);

/*DOC:
  Remark:
         This function implements the old SDIF Specification (before June 1999)
  Get the current matrix type from a file
*/
size_t SdifFGetOneMatrixType    (SdifFileT *SdifF, int Verbose);


/*DOC:
  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
  Return the current matrix type from a SdifStringT
*/
size_t SdifFGetOneMatrixTypefromSdifString(SdifFileT *SdifF, SdifStringT *SdifString);

/*DOC:
  Remark:
         This function implements the old SDIF Specification (before June 1999)
  Get the current component from a file
*/
int    SdifFGetOneComponent     (SdifFileT *SdifF, int Verbose,
				 SdifSignature *MatrixSignature,
				 char *ComponentName,
				 size_t *NbCharRead);

/*DOC:
  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
  Return the current component from a SdifStringT
*/
int    SdifFGetOneComponentfromSdifString(SdifFileT *SdifF,
					  SdifStringT *SdifString,
					  SdifSignature *MatrixSignature,
					  char *ComponentName);

/*DOC:
  Remark:
         This function implements the old SDIF Specification (before June 1999)
  Get the current Frame type from a file
*/
size_t SdifFGetOneFrameType     (SdifFileT *SdifF, int Verbose);

/*DOC:
  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
  Return the current frame type from a SdifStringT
*/
size_t SdifFGetOneFrameTypefromSdifString(SdifFileT *SdifF, SdifStringT *SdifString);

/*DOC:
  Remark:
         This function implements the old SDIF Specification (before June 1999)
  Get all types from a file
*/
size_t SdifFGetAllType          (SdifFileT *SdifF, int Verbose);


/*DOC:
  Remark:
         This function implements the old SDIF Specification (before June 1999)
  Get the current Stream ID from a file
*/
int    SdifFGetOneStreamID      (SdifFileT *SdifF, int Verbose, size_t *NbBytesRead);

/*DOC:
  Remark:
         This function is implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
  Get the current Stream ID from a SdifStringT
*/
int SdifFgetOneStreamIDfromSdifString(SdifFileT *SdifF, SdifStringT *SdifString);

/*DOC:
  Remark:
         This function implements the old SDIF Specification (before June 1999)
  Get all Stream ID from a file
*/
size_t SdifFGetAllStreamID      (SdifFileT *SdifF, int Verbose);

/*DOC:
  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
  Get the current Stream ID from a SdifStringT
*/
size_t SdifFGetAllStreamIDfromSdifString(SdifFileT *SdifF, SdifStringT *SdifString);

#endif /* _SdifFGet_ */
