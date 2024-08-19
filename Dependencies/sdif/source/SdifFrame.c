/* $Id: SdifFrame.c,v 3.9 2005-10-21 14:32:29 schwarz Exp $
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
 * Frame Header, Frame Data structures management
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.8  2004/09/09 17:42:13  schwarz
 * moved SdifSizeOf* functions to SdifGlobals.c
 *
 * Revision 3.7  2004/07/22 14:47:56  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.6  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.5  2001/05/02 09:34:43  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.4  2000/11/15 14:53:29  lefevre
 * no message
 *
 * Revision 3.3  2000/10/27  20:03:32  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.2.2.1  2000/08/21  21:35:18  tisseran
 * *** empty log message ***
 *
 * Revision 3.2  1999/09/28  13:08:59  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.1  1999/03/14  10:56:52  virolle
 * SdifStdErr add
 *
 */


#include "sdif_portability.h"

#include "SdifFrame.h"
#include "SdifFrameType.h"
#include <stdlib.h>


size_t
SdifSizeOfFrameHeader (void)
{
    return (sizeof(SdifFloat8) + 2 * sizeof(SdifUInt4));
}




SdifFrameHeaderT*
SdifCreateFrameHeader(SdifSignature Signature,
		      SdifUInt4 Size,
		      SdifUInt4 NbMatrix,
		      SdifUInt4 NumID,
		      SdifFloat8 Time)
{
  SdifFrameHeaderT *NewFrameHeader = NULL;
  
  NewFrameHeader = SdifMalloc(SdifFrameHeaderT);
  if (NewFrameHeader)
    {
      NewFrameHeader->Signature = Signature;
      NewFrameHeader->Size      = Size;
      NewFrameHeader->NbMatrix  = NbMatrix;
      NewFrameHeader->NumID     = NumID;
      NewFrameHeader->Time      = Time;
     return NewFrameHeader;
    }
  else
    {
      _SdifError(eAllocFail, "FrameHeader allocation");
      return NULL;
    }
}


SdifFrameHeaderT*
SdifCreateFrameHeaderEmpty(SdifSignature Signature)
{
  return SdifCreateFrameHeader(Signature, _SdifFrameHeaderSize, 0, 0, 0.);
}


void
SdifKillFrameHeader(SdifFrameHeaderT *FrameHeader)
{
  if (FrameHeader)
    SdifFree(FrameHeader);
  else
    _SdifError(eFreeNull, "FrameHeader free");
}


SdifFrameDataT*
SdifCreateFrameData(SdifHashTableT *FrameTypesTable,
		    SdifSignature FrameSignature,
		    SdifUInt4 NumID,
		    SdifFloat8 Time)
{
  SdifFrameDataT *NewFrameData = NULL;
  SdifFrameTypeT *FrameType = NULL;
  SdifUInt4 iMtrxD;

  FrameType = SdifGetFrameType(FrameTypesTable, FrameSignature);
  if (! FrameType)
    {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess), "Frame Type '%s'",
	      SdifSignatureToString(FrameSignature));
      _SdifError(eNotFound, errorMess);
      return NULL;
    }
  
  NewFrameData = SdifMalloc(SdifFrameDataT);
  if (NewFrameData)
    {
      NewFrameData->Header = SdifCreateFrameHeader(FrameSignature,
						   _SdifFrameHeaderSize,
						   0,
						   NumID,
						   Time);
      NewFrameData->Matrix_s = SdifCalloc(SdifMatrixDataT*, FrameType->NbComponent);
      for (iMtrxD=0; iMtrxD<FrameType->NbComponent; iMtrxD++)
	NewFrameData->Matrix_s[iMtrxD] = NULL;
      return NewFrameData;
    }
  else
    {
      _SdifError(eAllocFail, "FrameData allocation");
      return NULL;
    }
}






void
SdifKillFrameData(SdifHashTableT *FrameTypesTable, SdifFrameDataT *FrameData)
{
  SdifFrameTypeT *FrameType;
  unsigned int iMtrx;  
  
  FrameType = SdifGetFrameType(FrameTypesTable, FrameData->Header->Signature);
  
  if(FrameData)
    {
      if(FrameData->Matrix_s)
	{
	  for (iMtrx=0; iMtrx<FrameType->NbComponent; iMtrx++)
	    if (FrameData->Matrix_s[iMtrx] != NULL)
	      SdifKillMatrixData(FrameData->Matrix_s[iMtrx]);
	  
	  SdifFree(FrameData->Matrix_s);
	}
      else
	_SdifError(eFreeNull, "FrameData->Matrix_s free");
 
      SdifKillFrameHeader(FrameData->Header);
      SdifFree(FrameData);
    }
  else
    _SdifError(eFreeNull, "FrameData free");
}











SdifFrameDataT*
SdifFrameDataPutNthMatrixData(SdifFrameDataT *FrameData,
			      unsigned int NthMatrix,
			      SdifMatrixDataT *MatrixData)
{
  if (FrameData->Header->NbMatrix == (NthMatrix-1))
    {
      FrameData->Matrix_s[NthMatrix-1] = MatrixData;
      FrameData->Header->NbMatrix = NthMatrix;
      FrameData->Header->Size += MatrixData->Size;
    }
  else
    {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess),
	      "in FrameData '%s' ID:%u T:%g with Last:%d YourNth:%d",
	      SdifSignatureToString(FrameData->Header->Signature),
	      FrameData->Header->NumID,
	      FrameData->Header->Time,
	      FrameData->Header->NbMatrix,
	      NthMatrix);
      _SdifError(eAffectationOrder, errorMess);
      return NULL;
    }
  return FrameData;
}













SdifFrameDataT*
SdifFrameDataPutComponentMatrixData(SdifHashTableT *FrameTypesTable,
				    SdifFrameDataT *FrameData,
				    char *CompoName,
				    SdifMatrixDataT *MatrixData)
{
  SdifFrameTypeT *FrameType;
  SdifComponentT *Component;

  FrameType = SdifGetFrameType(FrameTypesTable, FrameData->Header->Signature);
  if (!FrameType)
    {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess),
	      "Frame Type '%s'",
	      SdifSignatureToString(FrameData->Header->Signature));
      _SdifError(eNotFound, errorMess);
      return NULL;
    }
  
  Component = SdifFrameTypeGetComponent(FrameType, CompoName);
  if ( ! Component)
    {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess),
	      "'%s' not a component of '%s' Frame Type",
	      CompoName, SdifSignatureToString(FrameData->Header->Signature));
      _SdifError(eNotFound, errorMess);
      return NULL;
    }
  
  if (Component->MtrxS == MatrixData->Header->Signature)
    return SdifFrameDataPutNthMatrixData(FrameData, Component->Num,MatrixData);
  else
    {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess),
	      "F:%s ID:%u T:%g\n   '%s' is not matrix type of %s\n   '%s' waiting",
	      SdifSignatureToString(FrameData->Header->Signature),
	      FrameData->Header->NumID,
	      FrameData->Header->Time,
	      SdifSignatureToString(MatrixData->Header->Signature),
	      CompoName,
	      SdifSignatureToString(Component->MtrxS));
      _SdifError(eAffectationOrder, errorMess);
      return NULL;
    }
}







SdifMatrixDataT*
SdifFrameDataGetNthMatrixData(SdifFrameDataT *FrameData,
			      unsigned int NthMatrix)
{
  if (FrameData->Header->NbMatrix >= NthMatrix)
    return FrameData->Matrix_s[NthMatrix-1];
  else
    return NULL;
}








SdifMatrixDataT*
SdifFrameDataGetComponentMatrixData(SdifHashTableT *FrameTypesTable,
				    SdifFrameDataT *FrameData,
				    char *CompoName)
{
  SdifFrameTypeT *FrameType;
  SdifComponentT *Component;

  FrameType = SdifGetFrameType(FrameTypesTable, FrameData->Header->Signature);
  if (!FrameType)
    {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess),
	      "'%s' Frame Type",
	      SdifSignatureToString(FrameData->Header->Signature));
      _SdifError(eNotFound, errorMess);
      return NULL;
    }
  
  Component = SdifFrameTypeGetComponent(FrameType, CompoName);
  if ( ! Component)
    {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess),
	      "'%s' is not a component of '%s' Frame Type",
	      CompoName, SdifSignatureToString(FrameData->Header->Signature));
      _SdifError(eNotFound, errorMess);
      return NULL;
    }
  
  return SdifFrameDataGetNthMatrixData(FrameData, Component->Num);
}


