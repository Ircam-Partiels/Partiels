/* $Id: SdifMatrix.h,v 3.6 2004-09-10 09:15:56 roebel Exp $
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
 * SdifMatrix.h
 *
 * Matrix Header, Data, Rows structure management.
 *
 * Memory allocation of SdifOneRowT* depend on size of one granule.
 * if nb rows or data type (float4 or float8),
 * then SdifReInitOneRow makes a realloc. Then think to use it before write.
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.5  2001/05/02 09:34:46  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.4  2000/11/21 14:51:50  schwarz
 * - sdif.h is now included by all sdif/Sdif*.c files.
 * - Removed all public typedefs, enums, structs, and defines from the
 *   individual sdif/Sdif*.h files, because they were duplicated in sdif.h.
 * - Todo: Do the same for the function prototypes, decide which types and
 *   prototypes really need to be exported.
 * - Removed SdifFileStruct.h.
 * - Preliminary new version of SdiffGetPos, SdiffSetPos.  They used the
 *   type fpos_t, which is no longer a long on RedHat 7 Linux.
 *
 * Revision 3.3  2000/10/27 20:03:38  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.2.2.1  2000/08/21  21:35:34  tisseran
 * *** empty log message ***
 *
 * Revision 3.2  1999/10/13  16:05:52  schwarz
 * Changed data type codes (SdifDataTypeET) to SDIF format version 3, as
 * decided with Matt Wright June 1999, added integer data types.
 * Added writing of 1NVT with real frame header (but data is still not in
 * matrices).
 * The data type handling makes heavy use of code-generating macros,
 * called for all data types with the sdif_foralltypes macro, thus
 * adding new data types is easy.
 *
 * Revision 3.1  1999/03/14  10:57:07  virolle
 * SdifStdErr add
 *
 * Revision 2.1  1998/12/21  18:27:29  schwarz
 * Inserted copyright message.
 *
 * Revision 2.0  1998/11/29  11:41:55  virolle
 * - New management of interpretation errors.
 * - Alignement of frames with CNMAT (execpt specials Chunk 1NVT, 1TYP, 1IDS).
 * _ Sdif Header File has a Sdif format version.
 * - Matrices order in frames is not important now. (only one occurence of
 *   a Matrix Type in a Frame Type declaration )
 * - Hard coded predefined types more dynamic management.
 * - Standart streams (stdin, stdout, stderr) set as binary for Windows32 to
 *   have exactly the same result on each plateforme.
 *
 * Revision 1.4  1998/11/10  15:31:51  schwarz
 * Removed all 'extern' keywords for prototypes, since this is redundant
 * (function prototypes are automatically linked extern), and it
 * prohibits cocoon from generating an entry in the HTML documentation
 * for this function.
 *
 */

#ifndef _SdifMatrix_
#define _SdifMatrix_

#include "SdifGlobals.h"
#include "SdifMatrixType.h"


SdifMatrixHeaderT* SdifCreateMatrixHeader    (SdifSignature Signature, 
					      SdifDataTypeET DataType,
					      SdifUInt4 NbRow, 
					      SdifUInt4 NbCol);

SdifMatrixHeaderT* SdifCreateMatrixHeaderEmpty (void);
void               SdifKillMatrixHeader        (SdifMatrixHeaderT *MatrixHeader);

SdifOneRowT*       SdifCreateOneRow          (SdifDataTypeET DataType, SdifUInt4  NbGranuleAlloc);
SdifOneRowT*       SdifReInitOneRow          (SdifOneRowT *OneRow, SdifDataTypeET DataType, SdifUInt4 NbData);
void               SdifKillOneRow            (SdifOneRowT *OneRow);
SdifOneRowT*       SdifOneRowPutValue        (SdifOneRowT *OneRow, SdifUInt4 numCol, SdifFloat8 Value);
SdifFloat8         SdifOneRowGetValue        (SdifOneRowT *OneRow, SdifUInt4 numCol);
SdifFloat8         SdifOneRowGetValueColName (SdifOneRowT *OneRow, SdifMatrixTypeT *MatrixType, char * NameCD);

SdifMatrixDataT*   SdifCreateMatrixData      (SdifSignature Signature, SdifDataTypeET DataType,
						     SdifUInt4 NbRow, SdifUInt4 NbCol);

void               SdifKillMatrixData        (SdifMatrixDataT *MatrixData);
SdifMatrixDataT*   SdifMatrixDataPutValue    (SdifMatrixDataT *MatrixData,
						     SdifUInt4  numRow, SdifUInt4  numCol, SdifFloat8 Value);

SdifFloat8         SdifMatrixDataGetValue    (SdifMatrixDataT *MatrixData,
						     SdifUInt4  numRow, SdifUInt4  numCol);
int SdifMatrixDataUpdateHeader (SdifMatrixDataT *Data, SdifMatrixHeaderT *NewH);
#endif /* _SdifMatrix_ */
