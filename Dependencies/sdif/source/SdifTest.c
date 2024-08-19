/* $Id: SdifTest.c,v 3.19 2008-01-09 16:13:48 bogaards Exp $
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
 * SdifTest.c
 *
 * Tests with message on errors apply on a SdifFileT*
 * exit(1) is exceptional
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.18  2007/12/10 10:46:12  roebel
 * Use const char* for read only function arguments.
 *
 * Revision 3.17  2005/10/21 14:32:30  schwarz
 * protect all static buffers from overflow by using snprintf instead of sprintf
 * move big errorMess buffers into error branch to avoid too large stack allocation
 *
 * Revision 3.16  2005/05/24 09:36:02  roebel
 *
 * Fixed last checkin comment which turned out to be the start of
 * a c-comment.
 *
 * Revision 3.15  2005/05/23 19:17:53  schwarz
 * - Sdiffread/Sdiffwrite functions with SdifFileT instead of FILE *
 *   -> eof error reporting makes more sense
 * - more cleanup of sdif.h, above functions are private in SdifRWLowLevel.h
 * - eEof becomes error 4 to be distinguishable from ascii chars
 * - SdifFScanNameValueLCurrNVT reimplemented for ascii only
 *
 * Revision 3.14  2005/05/13 15:28:41  schwarz
 * print signatures in hex also
 *
 * Revision 3.13  2005/04/07 15:56:48  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.12  2004/09/09 17:50:40  schwarz
 * Better syntax error messages and code reformatted to help finding
 * parsing bug that the type definition string has to end with whitespace.
 *
 * At the next bug with the type language parser and tokeniser, it should
 * be rewritten using flex/bison!
 *
 * Revision 3.11  2004/07/22 14:47:56  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.10  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.9  2001/07/12 14:15:31  roebel
 * Removed warning due to more columns then expected from the list
 * possible warnings. In fact this is allowed by the standard and should
 * not generate a warning.
 *
 * Revision 3.8  2001/05/02 09:34:48  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.7  2000/11/15 14:53:36  lefevre
 * no message
 *
 * Revision 3.6  2000/10/27  20:03:45  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.5  2000/08/22  13:38:41  schwarz
 * SdifFTestMatrixWithFrameHeader warns about additional undeclared
 * matrix in frame but do not signal an error, because we allow any
 * additional matrices in frames.
 * Replaced short by int, because that's faster and less error-prone.
 *
 * Revision 3.4.2.1  2000/08/21  21:35:53  tisseran
 * *** empty log message ***
 *
 * Revision 3.4  2000/04/11  14:32:09  schwarz
 * Don't return error, if more columns in matrix than known,
 * since then the wrong number of columns is read.
 *
 * Revision 3.3  1999/10/13  16:05:59  schwarz
 * Changed data type codes (SdifDataTypeET) to SDIF format version 3, as
 * decided with Matt Wright June 1999, added integer data types.
 * Added writing of 1NVT with real frame header (but data is still not in
 * matrices).
 * The data type handling makes heavy use of code-generating macros,
 * called for all data types with the sdif_foralltypes macro, thus
 * adding new data types is easy.
 *
 * Revision 3.2  1999/09/28  13:09:14  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.1  1999/03/14  10:57:23  virolle
 * SdifStdErr add
 *
 * Revision 2.3  1999/01/23  15:56:01  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:48  virolle
 * General Lists, and special chunk preparation to become frames
 */


#include "sdif_portability.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <sdif.h>
#include "SdifTest.h"
#include "SdifFile.h"
#include "SdifErrMess.h"


#define _NbOfWarningMax 19


SdifMatrixTypeT*
SdifTestMatrixType(SdifFileT *SdifF, SdifSignature Signature)
{
  SdifMatrixTypeT* MtrxT;
  SdifMatrixTypeT* PredefinedMtrxT;

	if(!Signature){
		_SdifFError(SdifF, eUnDefined, "Empty Signature for Matrix Type");
        return NULL;
	}

  MtrxT = SdifGetMatrixType(SdifF->MatrixTypesTable, Signature);
  if (MtrxT)
      return MtrxT;
  else
  {
      PredefinedMtrxT = SdifGetMatrixType(gSdifPredefinedTypes->MatrixTypesTable, Signature);
      if (PredefinedMtrxT)
      {
	  MtrxT = SdifCreateMatrixType(Signature, PredefinedMtrxT);
	  SdifPutMatrixType(SdifF->MatrixTypesTable, MtrxT);
	  return MtrxT;
      }
      else
      {
	  char errorMess[_SdifStringLen];

          snprintf(errorMess, sizeof(errorMess), "Matrix Type : '%s' (0x%08x)", 
		   SdifSignatureToString(Signature), (unsigned) Signature);
          _SdifFError(SdifF, eUnDefined, errorMess);
          return NULL;
      }
  }
}


int
SdifFTestDataType(SdifFileT* SdifF)
{
#if (_SdifFormatVersion >= 3)
    if (SdifDataTypeKnown (SdifF->CurrMtrxH->DataType))
        return (eTrue);
    else
    {
	char errorMess[_SdifStringLen];

        snprintf(errorMess, sizeof(errorMess), " 0x%04x, then Float4 used", 
		 SdifF->CurrMtrxH->DataType);
	_SdifFError (SdifF, eTypeDataNotSupported, errorMess);
	SdifF->CurrMtrxH->DataType = eFloat4;
	return eFalse;
    }
#else

  switch (SdifF->CurrMtrxH->DataType)
    {
    case eFloat4:
    case eFloat8:
      return eTrue;
    default:
    {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess), 
	       " 0x%04x, then Float4 used", SdifF->CurrMtrxH->DataType);
      SdifF->CurrMtrxH->DataType = eFloat4;
      _SdifFError(SdifF, eTypeDataNotSupported, errorMess);
      return eFalse;
    }

#endif
}


int SdifFTestNbColumns(SdifFileT* SdifF)
{
#if 0
  /* Due to user complaints we shut down the warning
   * regarding the wrong number of columns which 
   * is supported in the standard anyway */
  if (SdifF->CurrMtrxT->NbColumnDef < SdifF->CurrMtrxH->NbCol)
    {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess), "%d maximum, %d in the header",
        SdifF->CurrMtrxT->NbColumnDef,
        SdifF->CurrMtrxH->NbCol);
      _SdifFError(SdifF, eBadNbData, gSdifErrorMess);
      return eTrue; /* don't return eFalse, since then the wrong number
                      of columns is read */
    }
  else
#endif
    return eTrue;
}


int SdifFTestMatrixWithFrameHeader(SdifFileT* SdifF)
{
    SdifComponentT *CD;

    CD = SdifFrameTypeGetComponent_MtrxS (SdifF->CurrFramT,
					  SdifF->CurrMtrxH->Signature);

    if (CD)
	if (! SdifFIsInMtrxUsed (SdifF, SdifF->CurrMtrxH->Signature))
	    return eTrue;
	else
	    _SdifFError(SdifF, eMtrxUsedYet,
			SdifSignatureToString(SdifF->CurrMtrxH->Signature));
    else
    {   /* warn about additional undeclared matrix in frame */
	_SdifFError(SdifF, eMtrxNotInFrame,
		    SdifSignatureToString(SdifF->CurrMtrxH->Signature));
	return eTrue; /* ...but do not signal an error, because we
                         allow any additional matrices in frames */
    }
    
    return eFalse;
}


int
SdifFTestNotEmptyMatrix(SdifFileT* SdifF)
{
  if ((SdifF->CurrMtrxH->NbCol <= 0) && (SdifF->CurrMtrxH->NbRow <= 0)) 
    return eFalse;
  else
    return eTrue;
} 


int
SdifFTestMatrixHeader(SdifFileT* SdifF)
{
  SdifF->CurrMtrxT = SdifTestMatrixType(SdifF, SdifF->CurrMtrxH->Signature);

  if (! SdifF->CurrMtrxT )
    return eFalse;

  if (SdifF->CurrFramT)
    if(! SdifFTestMatrixWithFrameHeader(SdifF) )
      return eFalse;

  if (! SdifFTestDataType (SdifF) )
    return eFalse;

  if (! SdifFTestNbColumns (SdifF) )
    return eFalse;

  return eTrue;
}


SdifColumnDefT*
SdifTestColumnDef(SdifFileT *SdifF, SdifMatrixTypeT *MtrxT, const char *NameCD)
{
  SdifColumnDefT* CD;

  CD = SdifMatrixTypeGetColumnDef(MtrxT, NameCD);
  if (CD)
    return CD;
  else
    {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess),
	      "In Matrix Type '%s', Column : %s",
	      SdifSignatureToString(MtrxT->Signature),
	      NameCD);
      _SdifFError(SdifF, eUnDefined, errorMess);
      return NULL;
    }
}


SdifFrameTypeT*
SdifTestFrameType(SdifFileT *SdifF, SdifSignature Signature)
{
  SdifFrameTypeT
    *FrameType,
    *PredefinedFrameType;
  
  FrameType = SdifGetFrameType(SdifF->FrameTypesTable, Signature);
  if (FrameType)
    return FrameType;
  else
    {
      PredefinedFrameType = SdifGetFrameType(gSdifPredefinedTypes->FrameTypesTable, Signature);
      if (PredefinedFrameType)
	    {
	      FrameType = SdifCreateFrameType(Signature, PredefinedFrameType);
	      SdifPutFrameType(SdifF->FrameTypesTable, FrameType);
	      return FrameType;
	    }
      else
	    {
	      char errorMess[_SdifStringLen];
	
	      snprintf(errorMess, sizeof(errorMess), "Frame Type '%s' (0x%08x)",
		      SdifSignatureToString(Signature), (unsigned) Signature);
	      _SdifFError(SdifF, eUnDefined, errorMess);
	      return NULL;
	    }
    }
}


SdifComponentT*
SdifTestComponent(SdifFileT* SdifF, SdifFrameTypeT *FramT, const char *NameCD)
{
  SdifComponentT* CD;

  CD = SdifFrameTypeGetComponent(FramT, NameCD);
  if (CD)
    return CD;
  else
    {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess),
	      "In Frame Type '%s', Component : %s",
	      SdifSignatureToString(FramT->Signature),
	      NameCD);
      _SdifFError(SdifF, eUnDefined, errorMess);
      return NULL;
    }
}


int
SdifTestSignature(SdifFileT *SdifF, int CharEnd, SdifSignature Signature, const char *Mess)
{
	if(!Signature){
		_SdifFError(SdifF, eNameLength, "Empty Signature (probably there are two consecutive reserved characters ,;{}: )");
		return eFalse;
	}else if (SdifIsAReservedChar(CharEnd)  ||  isspace(CharEnd)){
		char errorMess[_SdifStringLen];

		snprintf(errorMess, sizeof(errorMess), "%s Invalid character in signature : '%s'. Last char read : '%c' (hex: %x)",
			Mess, SdifSignatureToString(Signature),	(char) CharEnd, CharEnd);		
		_SdifFError(SdifF, eNameLength, errorMess);
		return eFalse;
	}else{
		return eTrue;
	}
}


int
SdifTestCharEnd(SdifFileT *SdifF, int CharEnd, char MustBe, char *StringRead,
		int ErrCondition, const char *Mess)
{
    if ((unsigned) CharEnd != (unsigned) MustBe  ||  ErrCondition)
    {
	char errorMess[_SdifStringLen];

	snprintf(errorMess, sizeof(errorMess), 
		"In %s, Attempt to read '%c' (%d): actually read '%s%c' (%d) ",
		Mess, MustBe, MustBe, StringRead, CharEnd, CharEnd);
	_SdifFError (SdifF, eSyntax, errorMess);
	return  eFalse;
    }
    else
	return eTrue;
}


int
SdifTestMatrixTypeModifMode(SdifFileT *SdifF, SdifMatrixTypeT *MatrixType)
{  
  if (eCanModif == MatrixType->ModifMode)
    return eTrue;
  else
    {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess), "Matrix Type '%s' ",
	      SdifSignatureToString(MatrixType->Signature));
      _SdifFError (SdifF, eUserDefInFileYet, errorMess);
      return  eFalse;
    }
}


int
SdifTestFrameTypeModifMode(SdifFileT *SdifF, SdifFrameTypeT *FrameType)
{
  if (eCanModif == FrameType->ModifMode)
    return eTrue;
  else
    {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess), "Frame Type '%s' ",
	      SdifSignatureToString(FrameType->Signature));
      _SdifFError (SdifF, eUserDefInFileYet, errorMess);
      return  eFalse;
    }
}
