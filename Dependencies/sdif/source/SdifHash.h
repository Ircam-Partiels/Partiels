/* $Id: SdifHash.h,v 3.5 2002-05-24 19:37:52 ftissera Exp $
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
 * SdifHash.h
 *
 * SdifHash is coded like a small independant library.
 * Every HashTable have its own size and data type.
 * Every data type must have a function to free memory : 
 *     void <KillerName>(<type>* ptr).
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.4  2001/05/02 09:34:44  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.3  2000/11/21 14:51:49  schwarz
 * - sdif.h is now included by all sdif/Sdif*.c files.
 * - Removed all public typedefs, enums, structs, and defines from the
 *   individual sdif/Sdif*.h files, because they were duplicated in sdif.h.
 * - Todo: Do the same for the function prototypes, decide which types and
 *   prototypes really need to be exported.
 * - Removed SdifFileStruct.h.
 * - Preliminary new version of SdiffGetPos, SdiffSetPos.  They used the
 *   type fpos_t, which is no longer a long on RedHat 7 Linux.
 *
 * Revision 3.2  2000/10/27 20:03:35  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.1.2.1  2000/08/21  21:35:28  tisseran
 * *** empty log message ***
 *
 * Revision 3.1  1999/03/14  10:57:02  virolle
 * SdifStdErr add
 *
 * Revision 2.1  1998/12/21  18:27:27  schwarz
 * Inserted copyright message.
 *
 * Revision 2.0  1998/11/29  11:41:53  virolle
 * - New management of interpretation errors.
 * - Alignement of frames with CNMAT (execpt specials Chunk 1NVT, 1TYP, 1IDS).
 * _ Sdif Header File has a Sdif format version.
 * - Matrices order in frames is not important now. (only one occurence of
 *   a Matrix Type in a Frame Type declaration )
 * - Hard coded predefined types more dynamic management.
 * - Standart streams (stdin, stdout, stderr) set as binary for Windows32 to
 *   have exactly the same result on each plateforme.
 *
 * Revision 1.5  1998/11/10  15:31:50  schwarz
 * Removed all 'extern' keywords for prototypes, since this is redundant
 * (function prototypes are automatically linked extern), and it
 * prohibits cocoon from generating an entry in the HTML documentation
 * for this function.
 *
 * Revision 1.4  1998/07/23  17:02:55  virolle
 * *** empty log message ***
 *
 * Revision 1.3  1998/05/04  15:44:18  schwarz
 * Replaced constant char* args by const char* (also some void*).
 *
 */

#ifndef _SdifHash_
#define _SdifHash_

#include <sdif.h>


void SdifMakeEmptyHashTable (SdifHashTableT* HTable);
void SdifKillHashTable      (SdifHashTableT* HTable);



/******************  eHashChar ****************/

unsigned int SdifHashChar(const char* s, unsigned int nchar, unsigned int HashSize);

void*           SdifHashTableSearchChar(SdifHashTableT* HTable, const char *s, unsigned int nchar);
SdifHashTableT* SdifHashTablePutChar   (SdifHashTableT* HTable, const char *s, unsigned int nchar, void* Data);


/***************** eHashInt4 **********************/

unsigned int SdifHashInt4(unsigned int i, unsigned int HashSize);

void*           SdifHashTableSearchInt4(SdifHashTableT* HTable, unsigned int i);
SdifHashTableT* SdifHashTablePutInt4   (SdifHashTableT* HTable, const unsigned int i, void* Data);


/*************************** for all ***********************/

void*           SdifHashTableSearch (SdifHashTableT* HTable, void *ptr, unsigned int nobj);
SdifHashTableT* SdifHashTablePut    (SdifHashTableT* HTable, const void *ptr, unsigned int nobj, void* Data);

#endif /* _SdifHash_ */
