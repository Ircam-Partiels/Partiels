/* $Id: SdifFScan.c,v 3.22 2006-06-21 15:40:00 schwarz Exp $
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
 * F : SdifFileT* SdifF, Scan : pseudo-sdif text file read (SdifF->TextStream)
 *
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.21  2005/11/10 17:51:21  schwarz
 * hoppla!
 *
 * Revision 3.20  2005/10/21 14:32:29  schwarz
 * protect all static buffers from overflow by using snprintf instead of sprintf
 * move big errorMess buffers into error branch to avoid too large stack allocation
 *
 * Revision 3.19  2005/05/24 09:35:29  roebel
 *
 * Fixed last checkin comment which turned out to be the start of
 * a c-comment.
 *
 * Revision 3.18  2005/05/23 19:17:53  schwarz
 * - Sdiffread/Sdiffwrite functions with SdifFileT instead of FILE *
 *   -> eof error reporting makes more sense
 * - more cleanup of sdif.h, above functions are private in SdifRWLowLevel.h
 * - eEof becomes error 4 to be distinguishable from ascii chars
 * - SdifFScanNameValueLCurrNVT reimplemented for ascii only
 *
 * Revision 3.17  2005/04/07 15:56:47  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.16  2004/07/22 14:47:56  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.15  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.14  2003/08/06 15:11:45  schwarz
 * Finally removed obsolete functions (like SdifSkip...).
 *
 * Revision 3.13  2002/05/24 19:37:07  ftissera
 * Add include "sdif.h" to be compatible with C++
 *
 * Revision 3.12  2001/05/02 09:34:42  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.11  2000/11/15 14:53:27  lefevre
 * no message
 *
 * Revision 3.10  2000/10/27  20:03:29  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.9.2.2  2000/08/21  21:35:11  tisseran
 * *** empty log message ***
 *
 * Revision 3.9.2.1  2000/08/21  14:04:12  tisseran
 * *** empty log message ***
 *
 * Revision 3.8  2000/05/15  16:25:55  schwarz
 * cc on alpha doesn't like empty cases.
 *
 * Revision 3.7  2000/05/15  16:23:08  schwarz
 * Avoided avoidable warnings.
 *
 * Revision 3.6  2000/03/01  11:17:35  schwarz
 * Backwards compatibility of data types for reading text.
 *
 * Revision 3.5  1999/11/03  16:42:33  schwarz
 * Use _SdifNVTStreamID for stream ID of 1NVT frames because of CNMAT
 * restriction of only one frame type per stream.
 * (See SdifNameValuesLNewTable)
 *
 * Revision 3.4  1999/10/15  12:27:51  schwarz
 * No time parameter for name value tables and stream ID tables, since
 * this decision is better left to the library.  (It uses the _SdifNoTime
 * constant, which happens to be _Sdif_MIN_DOUBLE_.)
 *
 * Revision 3.3  1999/10/13  16:05:42  schwarz
 * Changed data type codes (SdifDataTypeET) to SDIF format version 3, as
 * decided with Matt Wright June 1999, added integer data types.
 * Added writing of 1NVT with real frame header (but data is still not in
 * matrices).
 * The data type handling makes heavy use of code-generating macros,
 * called for all data types with the sdif_foralltypes macro, thus
 * adding new data types is easy.
 *
 * Revision 3.2  1999/09/28  13:08:55  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.1  1999/03/14  10:56:45  virolle
 * SdifStdErr add
 *
 * Revision 2.3  1999/01/23  15:55:45  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:28  virolle
 * General Lists, and special chunk preparation to become frames
 *
 *
 *
 */


#include "sdif_portability.h"

#include "sdif.h"

#include "SdifRWLowLevel.h"
#include "SdifFScan.h"
#include "SdifTest.h"
#include "SdifFile.h"
#include "SdifHash.h"
#include "SdifMatrixType.h"
#include "SdifFrameType.h"
#include "SdifStreamID.h"
#include "SdifErrMess.h"
#include "SdifFGet.h"
#include <stdlib.h>


size_t 
SdifFScanGeneralHeader(SdifFileT *SdifF)
{
  size_t SizeR = 0;
  
  SdiffGetSignature(SdifF->TextStream, &(SdifF->CurrSignature), &SizeR);
  if (SdifF->CurrSignature != eSDIF)
    {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess),
	      "'%s' not correctly read\t: '%s'.",
	       SdifSignatureToString(eSDIF),
	       SdifSignatureToString(SdifF->CurrSignature));
      /*_SdifFError(SdifF, eBadHeader, "SDIF not correctly read");
	  */
      _SdifFError(SdifF, eBadHeader, errorMess);
      return 0;
    }

  return SizeR;
}






size_t
SdifFScanNameValueLCurrNVT(SdifFileT *SdifF)
{
    size_t    SizeR = 0;
    int CharEnd;
    char sdifString[_SdifStringLen];
	
    /* ascii: intermediate format, read pure data in frame */
    CharEnd = SdiffGetStringUntil (SdifF->TextStream, sdifString, _SdifStringLen, 
				   &SizeR, _SdifReservedChars);
    if (SdifTestCharEnd (SdifF, CharEnd, '{', sdifString, 
			 SdifStrLen (sdifString) != 0,
			 "Begin of NameValue Table declarations") != eFalse)
    {
	while (SdifFGetOneNameValue(SdifF, 't', &SizeR) != (int) '}')
	    /*loop*/;
    }
  
    return SizeR;
}





size_t
SdifFScanMatrixType(SdifFileT *SdifF)
{
  return SdifFGetOneMatrixType(SdifF, 't');
}







size_t
SdifFScanFrameType(SdifFileT *SdifF)
{
  return SdifFGetOneFrameType(SdifF, 't');
}







/* SdifFScanAllType ne lit pas "1TYP" puisque l'on sera aiguillie sur cette fonction 
 * apres lecture de "1TYP"
 */
size_t
SdifFScanAllType(SdifFileT *SdifF)
{
  return SdifFGetAllType(SdifF, 't');
}



/* SdifFScanAllStreamID ne lit pas "1IDS" puisque l'on sera aiguillie sur cette fonction 
 * apres lecture de "1IDS"
 */
size_t
SdifFScanAllStreamID(SdifFileT *SdifF)
{
  return SdifFGetAllStreamID(SdifF, 't');
}



/* Read from the end of File Header until TO HAVE READ 'SDFC' */
size_t
SdifFScanAllASCIIChunks(SdifFileT *SdifF)
{
  size_t    SizeR = 0;
  
  while (   (SdiffGetSignature(SdifF->TextStream, &(SdifF->CurrSignature), &SizeR)) != eEof  )
  {
      switch (SdifF->CurrSignature)
      {
	case e1NVT :
	  SdifNameValuesLNewTable(SdifF->NameValues, _SdifNVTStreamID);
	  SizeR += SdifFScanNameValueLCurrNVT(SdifF);
	  break;

	case e1TYP :
	  SizeR += SdifFScanAllType(SdifF);
	  break;
	  
	case e1IDS :
	      SizeR += SdifFScanAllStreamID(SdifF);
	  break;
	  
	case eENDC :
	  _SdifFError(SdifF, eSyntax, "Attempt to read 'SDFC' failed");
	case eSDFC :
	case eENDF :
	  return SizeR;
	  
	default :
	{
	  char errorMess[_SdifStringLen];

	  snprintf(errorMess, sizeof(errorMess),
		  "It is not a chunk name : '%s'",
		  SdifSignatureToString(SdifF->CurrSignature));
	  _SdifFError(SdifF, eSyntax, errorMess);
	}
	break;
      }
  }
  return SizeR;
}



void
SdifFScanMatrixHeader(SdifFileT *SdifF)
{
  size_t SizeR = 0;
  SdifInt4 DataType;
  
  SdifFCreateCurrMtrxH(SdifF); /* create only if it's necessary */
  
  SdiffGetSignature(SdifF->TextStream, &(SdifF->CurrMtrxH->Signature), &SizeR);
  fscanf(SdifF->TextStream, "%i", &DataType);

  /* when DataType was 32 for Float4 at Ircam and others */
  switch ((SdifDataTypeET) (int) DataType)
  {
      case eFloat4a:
      case eFloat4b:
          DataType = eFloat4;
      break;

      case eFloat8a:
      case eFloat8b:
	  DataType = eFloat8;
      break;
      default: /* all is fine! */;
  }

  SdifF->CurrMtrxH->DataType = (SdifDataTypeET) (int) DataType;

  fscanf(SdifF->TextStream, "%u", &(SdifF->CurrMtrxH->NbRow));
  fscanf(SdifF->TextStream, "%u", &(SdifF->CurrMtrxH->NbCol));

  if ( SdifFTestMatrixHeader(SdifF) )
    {
      SdifReInitOneRow(SdifF->CurrOneRow,
                       SdifF->CurrMtrxH->DataType,
                       SdifF->CurrMtrxH->NbCol);
    }

}









void
SdifFScanOneRow(SdifFileT *SdifF)
{
    /* case template for type from SdifDataTypeET */
#   define scanrowcase(type)						  \
    case e##type:  							  \
        SdiffScan##type (SdifF->TextStream, SdifF->CurrOneRow->Data.type, \
			 SdifF->CurrOneRow->NbData);			  \
    break;

    switch (SdifF->CurrOneRow->DataType)
    {
        /* generate cases for all types */
	sdif_foralltypes (scanrowcase);

	default:
	{
	    char errorMess[_SdifStringLen];

	    snprintf(errorMess, sizeof(errorMess), "in text file, OneRow 0x%04x, then Float4 used", SdifF->CurrOneRow->DataType);
	    _SdifFError(SdifF, eTypeDataNotSupported, errorMess);
	    SdiffScanFloat4(SdifF->TextStream, SdifF->CurrOneRow->Data.Float4,
			    SdifF->CurrOneRow->NbData);
	}
	break;
    }
}







/* Frame signature read before and update */
void
SdifFScanFrameHeader(SdifFileT *SdifF)
{

  SdifFCreateCurrFramH(SdifF, SdifF->CurrSignature);
    
  SdifF->CurrFramH->Size = _SdifUnknownSize ;
  fscanf(SdifF->TextStream, "%u", &(SdifF->CurrFramH->NbMatrix));
  fscanf(SdifF->TextStream, "%u", &(SdifF->CurrFramH->NumID));
  fscanf(SdifF->TextStream, "%lg", &(SdifF->CurrFramH->Time));


  SdifF->CurrFramT = SdifTestFrameType(SdifF, SdifF->CurrFramH->Signature);
  if (SdifF->CurrFramT)
    SdifFReInitMtrxUsed(SdifF);
}
