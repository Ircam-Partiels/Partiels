/* $Id: SdifGlobals.h,v 3.18 2007-03-21 19:44:15 roebel Exp $
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
 * SdifGlobals.h
 *
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.17  2005/05/23 17:52:53  schwarz
 * Unified error handling:
 * - SdifErrorEnum (global errors) integrated into SdifErrorTagET (file errors)
 * - no more SdifError.[ch], everything done by SdifErrMess.[ch]
 *
 * Revision 3.16  2005/04/07 15:56:47  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.15  2005/04/07 15:20:23  schwarz
 * removed duplicated declarations that belong to the external API in sdif.h
 *
 * Revision 3.14  2004/07/22 14:47:56  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.13  2001/05/02 09:34:44  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.12  2000/11/21 16:34:49  roebel
 * New SdifSignatureConst builds integer signature according to
 * endianess of machine. Multicharacter constants are no longer
 * supported by the library. Cleaned up sdif.h/SdifGlobals.h a bit.
 * Test for Multicharacter conversion is removed from configure.in.
 *
 * Revision 3.11  2000/11/21 14:51:49  schwarz
 * - sdif.h is now included by all sdif/Sdif*.c files.
 * - Removed all public typedefs, enums, structs, and defines from the
 *   individual sdif/Sdif*.h files, because they were duplicated in sdif.h.
 * - Todo: Do the same for the function prototypes, decide which types and
 *   prototypes really need to be exported.
 * - Removed SdifFileStruct.h.
 * - Preliminary new version of SdiffGetPos, SdiffSetPos.  They used the
 *   type fpos_t, which is no longer a long on RedHat 7 Linux.
 *
 * Revision 3.10  2000/11/15 14:53:30  lefevre
 * no message
 *
 * Revision 3.9  2000/10/27  20:03:34  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.8.2.2  2000/08/21  21:35:24  tisseran
 * *** empty log message ***
 *
 * Revision 3.8.2.1  2000/08/21  14:04:18  tisseran
 * *** empty log message ***
 *
 * Revision 3.8  2000/04/11  14:44:11  schwarz
 * Use Char for Text in SDIF types.
 *
 * Revision 3.7  2000/03/01  11:17:37  schwarz
 * Backwards compatibility of data types for reading text.
 *
 * Revision 3.6  1999/11/03  16:42:35  schwarz
 * Use _SdifNVTStreamID for stream ID of 1NVT frames because of CNMAT
 * restriction of only one frame type per stream.
 * (See SdifNameValuesLNewTable)
 *
 * Revision 3.5  1999/10/15  12:27:08  schwarz
 * Changed min/max to upper case.
 *
 * Revision 3.4  1999/10/13  16:05:49  schwarz
 * Changed data type codes (SdifDataTypeET) to SDIF format version 3, as
 * decided with Matt Wright June 1999, added integer data types.
 * Added writing of 1NVT with real frame header (but data is still not in
 * matrices).
 * The data type handling makes heavy use of code-generating macros,
 * called for all data types with the sdif_foralltypes macro, thus
 * adding new data types is easy.
 *
 * Revision 3.3  1999/09/28  10:39:26  schwarz
 * Introduced SdifSignatureConst for signature constants across different
 * compilers/architectures.
 *
 * Revision 3.2  1999/09/20  13:28:05  schwarz
 * Introduced min/max macros.  (Why oh why is something so basic not part
 * of the standard? --- there is no macros.h on sgi!)
 *
 * Revision 3.1  1999/03/14  10:56:58  virolle
 * SdifStdErr add
 *
 * Revision 2.4  1999/02/28  12:16:46  virolle
 * memory report
 *
 * Revision 2.3  1999/01/23  15:55:50  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:35  virolle
 * General Lists, and special chunk preparation to become frames
 *
 * Revision 2.1  1998/12/21  18:27:23  schwarz
 * Inserted copyright message.
 *
 * Revision 2.0  1998/11/29  11:41:48  virolle
 * - New management of interpretation errors.
 * - Alignement of frames with CNMAT (execpt specials Chunk 1NVT, 1TYP, 1IDS).
 * _ Sdif Header File has a Sdif format version.
 * - Matrices order in frames is not important now. (only one occurence of
 *   a Matrix Type in a Frame Type declaration )
 * - Hard coded predefined types more dynamic management.
 * - Standart streams (stdin, stdout, stderr) set as binary for Windows32 to
 *   have exactly the same result on each plateforme.
 *
 * Revision 1.6  1998/11/10  15:31:48  schwarz
 * Removed all 'extern' keywords for prototypes, since this is redundant
 * (function prototypes are automatically linked extern), and it
 * prohibits cocoon from generating an entry in the HTML documentation
 * for this function.
 *
 * Revision 1.5  1998/07/23  17:02:53  virolle
 * *** empty log message ***
 *
 * Revision 1.4  1998/04/24  12:40:34  schwarz
 * Made char * arguments constant for SdifNameValuesLPut and functions called by it.
 */

#ifndef _SdifGlobals_
#define _SdifGlobals_


#include <stdio.h>
#include <float.h>

#include <sdif.h>
#include "sdif_portability.h"
#include "SdifErrMess.h"

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif


/* some constants for allocation sizes */

/* SdifString:		Default memory size allocated for string */
#define _SdifStringGranule 128 

/* SdifSignatureTab:	Growth steps for reallocation */
#define _SdifSignatureTabGranule 16


#ifdef HAVE_PTHREAD
void    SdifCreateThreadKeys() ;
#endif

struct SdifGlobals{
	char stringSignature[_SdifNbMaxPrintSignature][5];
	int currStringPosSignature;
	SdifListNStockT sdifListNodeStock;
};

struct SdifGlobals* GetSdifGlobals();
void FreeGlobals(void *);



#endif /* _SdifGlobals_ */
