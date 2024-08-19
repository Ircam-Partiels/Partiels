/* $Id: SdifFPrint.c,v 3.16 2005-10-21 14:32:29 schwarz Exp $
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
 * F : SdifFileT* SdifF, Print : write only into the pseudo-sdif text file, SdifF->TextStream.
 *
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.15  2005/05/24 09:35:00  roebel
 * Fixed last checkin comment which turned out to be the start of
 * a c-comment.
 *
 * Removed all old versions of ASCII Chunk writing functions that were
 * still based on the Text/Binary output distinction via
 * function argument. Now the write functions are used for binary
 * and the Put functions used for ASCII files.
 * The writing of the pre 1999 SDIF
 * ASCII chunks in binary files is no longer possible.
 *
 * Revision 3.14  2005/04/07 15:56:46  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.13  2004/07/22 14:47:56  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.12  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.11  2003/08/06 15:11:45  schwarz
 * Finally removed obsolete functions (like SdifSkip...).
 *
 * Revision 3.10  2003/07/21 09:59:08  roebel
 * Added support for intreger types to SdifFPrintOneRow
 *
 * Revision 3.9  2003/05/30 14:33:44  schwarz
 * Added text matrix case to SdifFPrintOneRow for sdiftotext conversion
 * (called from SdifConvToText.c)
 *
 * Revision 3.8  2002/05/24 19:37:52  ftissera
 * Change code to be compatible with C++
 * Cast pointers to correct type.
 *
 * Revision 3.7  2001/05/02 09:34:41  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.6  2000/11/15 14:53:26  lefevre
 * no message
 *
 * Revision 3.5  2000/10/27  20:03:27  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.4.2.1  2000/08/21  21:35:05  tisseran
 * *** empty log message ***
 *
 * Revision 3.4  2000/04/11  13:24:38  schwarz
 * Fixed ancient bug in ...AllAsciiChunks: SdifExistUserFrameType worked
 * on SdifF->MatrixTypesTable, not SdifF->FrameTypesTable.
 *
 * Revision 3.3  1999/10/13  16:05:40  schwarz
 * Changed data type codes (SdifDataTypeET) to SDIF format version 3, as
 * decided with Matt Wright June 1999, added integer data types.
 * Added writing of 1NVT with real frame header (but data is still not in
 * matrices).
 * The data type handling makes heavy use of code-generating macros,
 * called for all data types with the sdif_foralltypes macro, thus
 * adding new data types is easy.
 *
 * Revision 3.2  1999/09/28  13:08:52  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.1  1999/03/14  10:56:39  virolle
 * SdifStdErr add
 */


#include "sdif_portability.h"

#include <sdif.h>
#include "SdifTest.h"
#include "SdifFile.h"
#include "SdifHash.h"
#include "SdifMatrixType.h"
#include "SdifFrameType.h"
#include "SdifStreamID.h"
#include "SdifFPut.h"
#include "SdifErrMess.h"






size_t
SdifFPrintGeneralHeader(SdifFileT *SdifF)
{
  size_t SizeW = 0;
  
  SizeW += fprintf(SdifF->TextStream, "%s\n\n", SdifSignatureToString(eSDIF));
  return SizeW;
}




size_t
SdifFPrintNameValueLCurrNVT(SdifFileT *SdifF)
{
  size_t SizeW = 0;

  SizeW += fprintf(SdifF->TextStream, "%s\n", SdifSignatureToString(e1NVT));
  SizeW += SdifFPutNameValueLCurrNVT (SdifF);
  SizeW += fprintf(SdifF->TextStream, "\n");
  return SizeW;
}




size_t
SdifFPrintAllNameValueNVT(SdifFileT *SdifF)
{
    size_t SizeW = 0;

    SdifListInitLoop(SdifF->NameValues->NVTList);
    while (SdifListIsNext(SdifF->NameValues->NVTList))
    {
        SdifF->NameValues->CurrNVT = (SdifNameValueTableT *)
	    SdifListGetNext(SdifF->NameValues->NVTList);
        SizeW += SdifFPrintNameValueLCurrNVT (SdifF);
    }
    return SizeW;
}



size_t
SdifFPrintMatrixType(SdifFileT *SdifF, SdifMatrixTypeT *MatrixType)
{
  return SdifFPutOneMatrixType(SdifF,  MatrixType);
}




size_t
SdifFPrintFrameType(SdifFileT *SdifF, SdifFrameTypeT *FrameType)
{
  return SdifFPutOneFrameType(SdifF, FrameType);
}




size_t
SdifFPrintAllType(SdifFileT *SdifF)
{
  size_t SizeW = 0;
  
  if ((SdifF->TypeDefPass == eNotPass) || (SdifF->TypeDefPass == eReadPass))
    {      
      SizeW += fprintf(SdifF->TextStream, "%s\n", SdifSignatureToString(e1TYP));
      SizeW += SdifFPutAllType(SdifF);
      SizeW += fprintf(SdifF->TextStream, "\n");

      SdifF->TypeDefPass = eWritePass;
    }
  else
    _SdifFError(SdifF, eOnlyOneChunkOf, SdifSignatureToString(e1TYP));

  return SizeW;
}





size_t
SdifFPrintAllStreamID(SdifFileT *SdifF)
{
  size_t SizeW = 0;

  if ((SdifF->StreamIDPass == eNotPass) || (SdifF->StreamIDPass == eReadPass))
    {      
      SizeW += fprintf(SdifF->TextStream, "%s\n", SdifSignatureToString(e1IDS));
      SizeW += SdifFPutAllStreamID(SdifF);
      SizeW += fprintf(SdifF->TextStream, "\n");

      SdifF->StreamIDPass = eWritePass;
    }
  else
    _SdifFError(SdifF, eOnlyOneChunkOf, SdifSignatureToString(e1IDS));
      
  return SizeW;
}





size_t
SdifFPrintAllASCIIChunks(SdifFileT *SdifF)
{
  size_t SizeW = 0;

  if (SdifNameValuesLIsNotEmpty(SdifF->NameValues))
    {
      SizeW += SdifFPrintAllNameValueNVT(SdifF);
      SizeW += fprintf(SdifF->TextStream, "\n");
    }

  if (   (SdifExistUserMatrixType(SdifF->MatrixTypesTable))
      || (SdifExistUserFrameType(SdifF->FrameTypesTable)) )
    {
      SizeW += SdifFPrintAllType (SdifF);
      SizeW += fprintf(SdifF->TextStream, "\n");
    }

  if (SdifStreamIDTableGetNbData  (SdifF->StreamIDsTable) > 0)
    {
      SizeW += SdifFPrintAllStreamID (SdifF);
      SizeW += fprintf(SdifF->TextStream, "\n");
    }

  return SizeW;
}





size_t
SdifFPrintMatrixHeader(SdifFileT *SdifF)
{
  size_t
    SizeW = 0;

  SizeW += fprintf(SdifF->TextStream, "  %s\t0x%04x\t%u\t%u\n",
		   SdifSignatureToString(SdifF->CurrMtrxH->Signature),
		   SdifF->CurrMtrxH->DataType,
		   SdifF->CurrMtrxH->NbRow,
		   SdifF->CurrMtrxH->NbCol);

  return SizeW;
}






size_t
SdifFPrintOneRow(SdifFileT *SdifF)
{
  unsigned int iCol;
  size_t SizeW = 0;
  
  switch (SdifF->CurrOneRow->DataType)
  {
    case eFloat8:
	for (iCol = 0; iCol < SdifF->CurrOneRow->NbData; iCol++)
	    SizeW += fprintf(SdifF->TextStream, "\t%g", 
			     SdifF->CurrOneRow->Data.Float8[iCol]);
    break;

    case eFloat4:
	for (iCol = 0; iCol < SdifF->CurrOneRow->NbData; iCol++)
	    SizeW += fprintf(SdifF->TextStream, "\t%g", 
			     SdifF->CurrOneRow->Data.Float4[iCol]);
    break;

    case eInt1:
	for (iCol = 0; iCol < SdifF->CurrOneRow->NbData; iCol++)
	    SizeW += fprintf(SdifF->TextStream, "\t%d", 
			     SdifF->CurrOneRow->Data.Int1[iCol]);
    break;

    case eInt2:
	for (iCol = 0; iCol < SdifF->CurrOneRow->NbData; iCol++)
	    SizeW += fprintf(SdifF->TextStream, "\t%d", 
			     SdifF->CurrOneRow->Data.Int2[iCol]);
    break;

    case eInt4:
	for (iCol = 0; iCol < SdifF->CurrOneRow->NbData; iCol++)
	    SizeW += fprintf(SdifF->TextStream, "\t%d", 
			     SdifF->CurrOneRow->Data.Int4[iCol]);
    break;

    case eUInt1:
	for (iCol = 0; iCol < SdifF->CurrOneRow->NbData; iCol++)
	    SizeW += fprintf(SdifF->TextStream, "\t%u", 
			     SdifF->CurrOneRow->Data.UInt1[iCol]);
    break;


    case eUInt2:
	for (iCol = 0; iCol < SdifF->CurrOneRow->NbData; iCol++)
	    SizeW += fprintf(SdifF->TextStream, "\t%u", 
			     SdifF->CurrOneRow->Data.UInt2[iCol]);
    break;


    case eUInt4:
	for (iCol = 0; iCol < SdifF->CurrOneRow->NbData; iCol++)
	    SizeW += fprintf(SdifF->TextStream, "\t%u", 
			     SdifF->CurrOneRow->Data.UInt4[iCol]);
    break;

    case eText:
	for (iCol = 0; iCol < SdifF->CurrOneRow->NbData; iCol++)
	    SizeW += fprintf(SdifF->TextStream, "\t'%c'", 
			     SdifF->CurrOneRow->Data.Char[iCol]);
    break;

    default:
    {
	char errorMess[_SdifStringLen];
  
	snprintf(errorMess, sizeof(errorMess), 
		 "OneRow 0x%04x, then Float4 used", 
		 SdifF->CurrOneRow->DataType);
	_SdifFError(SdifF, eTypeDataNotSupported, errorMess);
	/* then values are considered as Float4 */
	for (iCol = 0; iCol < SdifF->CurrOneRow->NbData; iCol++)
	    SizeW += fprintf(SdifF->TextStream, "\t%g", 
			     SdifF->CurrOneRow->Data.Float4[iCol]);
    }
    break;
  }
  SizeW += fprintf(SdifF->TextStream, "\n");

  return SizeW;
}






size_t
SdifFPrintFrameHeader(SdifFileT *SdifF)
{
  size_t
    SizeW = 0;
  
  SizeW += fprintf(SdifF->TextStream,
		   "%s\t%u\t%u\t%g\n",
		   SdifSignatureToString(SdifF->CurrFramH->Signature),  
		   SdifF->CurrFramH->NbMatrix,
		   SdifF->CurrFramH->NumID,
		   SdifF->CurrFramH->Time);

  return SizeW;
}
