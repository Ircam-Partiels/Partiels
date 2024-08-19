/* $Id: SdifHard_OS.h,v 3.13 2005-10-21 14:32:29 schwarz Exp $
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
 * SdifHard_OS.h
 *
 *
 *
 *
 * author: Dominique Virolle 1998
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.12  2005/05/24 09:36:36  roebel
 *
 * Fixed last checkin comment which turned out to be the start of
 * a c-comment.
 *
 * Revision 3.11  2005/05/23 19:17:53  schwarz
 * - Sdiffread/Sdiffwrite functions with SdifFileT instead of FILE *
 *   -> eof error reporting makes more sense
 * - more cleanup of sdif.h, above functions are private in SdifRWLowLevel.h
 * - eEof becomes error 4 to be distinguishable from ascii chars
 * - SdifFScanNameValueLCurrNVT reimplemented for ascii only
 *
 * Revision 3.10  2005/04/07 15:20:23  schwarz
 * removed duplicated declarations that belong to the external API in sdif.h
 *
 * Revision 3.9  2001/05/02 09:34:44  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.8  2000/11/21 14:51:49  schwarz
 * - sdif.h is now included by all sdif/Sdif*.c files.
 * - Removed all public typedefs, enums, structs, and defines from the
 *   individual sdif/Sdif*.h files, because they were duplicated in sdif.h.
 * - Todo: Do the same for the function prototypes, decide which types and
 *   prototypes really need to be exported.
 * - Removed SdifFileStruct.h.
 * - Preliminary new version of SdiffGetPos, SdiffSetPos.  They used the
 *   type fpos_t, which is no longer a long on RedHat 7 Linux.
 *
 * Revision 3.7  2000/11/21 09:00:45  roebel
 * replaced fgetpos/fsetpos by ftell/fseek for Unix
 * because fpos_t is no longer an int for some systems
 *
 * Revision 3.6  2000/10/27 20:03:34  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.5.2.2  2000/08/21  21:35:26  tisseran
 * *** empty log message ***
 *
 * Revision 3.5.2.1  2000/08/21  14:04:20  tisseran
 * *** empty log message ***
 *
 * Revision 3.5  2000/03/01  11:17:38  schwarz
 * Factored out SdiffIsFile from get/setpos macros.
 *
 * Revision 3.4  1999/10/13  16:05:50  schwarz
 * Changed data type codes (SdifDataTypeET) to SDIF format version 3, as
 * decided with Matt Wright June 1999, added integer data types.
 * Added writing of 1NVT with real frame header (but data is still not in
 * matrices).
 * The data type handling makes heavy use of code-generating macros,
 * called for all data types with the sdif_foralltypes macro, thus
 * adding new data types is easy.
 *
 * Revision 3.3  1999/10/07  15:12:25  schwarz
 * Added isSeekable flag in SdifFileT struct.  This allows to simplify the
 * many tests for stdio on opening the stream.
 * Added SdifStrEq utility function.
 *
 * Revision 3.2  1999/06/18  16:23:58  schwarz
 * SdifSignatureCmpNoVersion dropped LAST byte on alpha, because the mask
 * 0x00ffffff was not byteswapped.  Introduced gSdifSignatureVersionMask,
 * which is initialised in SdifInitMachineType to appropriate value.
 *
 * Revision 3.1  1999/03/14  10:57:00  virolle
 * SdifStdErr add
 *
 * Revision 2.3  1999/01/23  15:55:52  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:37  virolle
 * General Lists, and special chunk preparation to become frames
 *
 * Revision 2.1  1998/12/21  18:27:25  schwarz
 * Inserted copyright message.
 *
 * Revision 2.0  1998/11/29  11:41:50  virolle
 * - New management of interpretation errors.
 * - Alignement of frames with CNMAT (execpt specials Chunk 1NVT, 1TYP, 1IDS).
 * _ Sdif Header File has a Sdif format version.
 * - Matrices order in frames is not important now. (only one occurence of
 *   a Matrix Type in a Frame Type declaration )
 * - Hard coded predefined types more dynamic management.
 * - Standart streams (stdin, stdout, stderr) set as binary for Windows32 to
 *   have exactly the same result on each plateforme.
 *
 * Revision 1.2  1998/11/10  15:31:49  schwarz
 * Removed all 'extern' keywords for prototypes, since this is redundant
 * (function prototypes are automatically linked extern), and it
 * prohibits cocoon from generating an entry in the HTML documentation
 * for this function.
 */

#ifndef _SdifHard_OS_
#define _SdifHard_OS_


#include <stdio.h>
#include <float.h>
#include <sdif.h>	/* SdifMachineET */


SdifMachineET SdifGetMachineType(void);
extern SdifMachineET gSdifMachineType;
SdifMachineET SdifInitMachineType(void);
extern SdifUInt4 gSdifSignatureVersionMask;


/** 2 byte array swapping in place. */
void SdifSwap2 (void *ptr, size_t num);

/** 2 byte array swapping with copy. */
void SdifSwap2Copy (void *src, void *dest, size_t num);

/** 4 byte array swapping in place. */
void SdifSwap4 (void *ptr, size_t num);

/** 4 byte array swapping with copy */
void SdifSwap4Copy (void *src, void *dest, size_t num);

/** 8 byte array swapping in place. */
void SdifSwap8 (void *ptr, size_t num);

/** 8 byte array swapping with copy. */
void SdifSwap8Copy (void *src, void *dest, size_t num);


void SdifLittleToBig(void *BigPtr, void *LittlePtr, size_t size);
void SdifBigToLittle(void *InOutPtr, size_t size);

#endif /* _SdifHard_OS_ */
