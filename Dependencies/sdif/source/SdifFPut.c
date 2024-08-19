/* $Id: SdifFPut.c,v 3.16 2005-05-24 09:35:10 roebel Exp $
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
 * F : SdifFileT* SdifF, Put : write ascii frames into pseudo-sdif text file,
 * SdifF->TextStream or sdif file SdifF->Stream.
 * switch 'verbose'
 *
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.15  2005/04/07 15:56:47  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.14  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.13  2003/08/06 15:11:45  schwarz
 * Finally removed obsolete functions (like SdifSkip...).
 *
 * Revision 3.12  2002/05/24 19:37:52  ftissera
 * Change code to be compatible with C++
 * Cast pointers to correct type.
 *
 * Revision 3.11  2001/05/02 09:34:41  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.10  2000/11/15 14:53:26  lefevre
 * no message
 *
 * Revision 3.9  2000/10/27  20:03:28  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.8.2.2  2000/08/21  21:35:07  tisseran
 * *** empty log message ***
 *
 * Revision 3.8.2.1  2000/08/21  14:04:08  tisseran
 * *** empty log message ***
 *
 * Revision 3.7  2000/07/18  15:08:29  tisseran
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
 * Revision 3.6  2000/07/06  19:01:46  tisseran
 * Add function for frame and matrix type declaration
 * Remove string size limitation for NameValueTable
 * TODO: 1TYP and 1IDS frame must contain an 1NVT (text) matrix
 *       Actually, data are written with fprintf.
 *
 * Revision 3.5  2000/05/12  14:41:45  schwarz
 * On behalf of Adrien, synchronisation with Mac sources, with some slight
 * changes because of cross-platform issues:
 * - Mac only stuff: XpSetFileAttribute XpFileSize
 * - Cross platform wrapper: XpGetenv XpExit
 * - Dangerous: strings.h (and thus bzero, bcopy) is not ANSI and thus doesn't
 *   exist on Mac.  Use string.h and memset, memcpy.
 *
 * Revision 3.4  2000/05/04  15:03:58  schwarz
 * Avoid strlen warning.
 *
 * Revision 3.3  2000/04/11  14:31:21  schwarz
 * Read/write NVT as frame with 1 text matrix, conforming to SDIF spec.
 *
 * Revision 3.2  1999/09/28  13:08:53  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.1  1999/03/14  10:56:41  virolle
 * SdifStdErr add
 */


#include "sdif_portability.h"

#include <string.h>

#include "sdif.h"
#include "SdifHard_OS.h"
#include "SdifRWLowLevel.h"
#include "SdifFPut.h"
#include "SdifTest.h"
#include "SdifFile.h"
#include "SdifList.h"
#include "SdifHash.h"




size_t
SdifFPutOneNameValue(SdifFileT* SdifF, SdifNameValueT *NameValue)
{
  size_t  SizeW = 0;
  
  SizeW += fprintf(SdifF->TextStream, "%s\t", NameValue->Name);
  SizeW += fprintf(SdifF->TextStream, "%s;\n", NameValue->Value);

  return  SizeW;
}

size_t SdifFPutSignature  (SdifFileT *SdifF,SdifSignature *sig){

    SdifSignature SignW;

    switch (gSdifMachineType)
    {     
    case eLittleEndian :
    case eLittleEndian64 :
      SdifSwap4Copy(sig, &SignW, 1);
      break;
      
    default :
      SignW = *sig;
      break;
    }

    return fwrite(&SignW,1,sizeof(SignW),SdifF->TextStream);
}



size_t
SdifFPutNameValueLCurrNVT (SdifFileT *SdifF)
/* SdifFPutNameValueLCurrNVT doesn't write frame header and padding */
{
  size_t          SizeW = 0;
  SdifUInt4       iNV;
  SdifHashNT     *pNV;
  SdifHashTableT *HTable;

  HTable = SdifF->NameValues->CurrNVT->NVHT;
  
  SizeW += fprintf(SdifF->TextStream, "{\n");
  for(iNV=0; iNV<HTable->HashSize; iNV++)
    for (pNV = HTable->Table[iNV]; pNV; pNV = pNV->Next)
      SizeW += SdifFPutOneNameValue(SdifF, (SdifNameValueT *)pNV->Data);
  
  SizeW += fprintf(SdifF->TextStream, "}");  

  return SizeW;
}



/*DOC:
  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
  This function put NameValue to SdifString
*/
int
SdifFNameValueLCurrNVTtoSdifString (SdifFileT *SdifF, SdifStringT *SdifString)
{
  SdifUInt4       iNV;
  SdifHashNT     *pNV;
  SdifHashTableT *HTable;
  int result = 1;
  
  HTable = SdifF->NameValues->CurrNVT->NVHT;
  
  for(iNV=0; iNV<HTable->HashSize; iNV++)
    for (pNV = HTable->Table[iNV]; pNV; pNV = pNV->Next)
      {
	SdifNameValueT *NameValue = (SdifNameValueT *) pNV->Data;
	result = SdifStringAppend(SdifString, NameValue->Name);
	result *= SdifStringAppend(SdifString, "\t");
	result *= SdifStringAppend(SdifString, NameValue->Value);
	result *= SdifStringAppend(SdifString, "\n");	
      }

  return result;
}


//ASCII only
size_t
SdifFPutOneMatrixType(SdifFileT* SdifF,  SdifMatrixTypeT *MatrixType)
{
  SdifColumnDefT    *ColumnDef;
  size_t            SizeW = 0;
  FILE              *file=SdifF->TextStream;  
  
  if (! SdifListIsEmpty(MatrixType->ColumnUserList))
    {
      SizeW += fprintf(file, "  %s\t", SdifSignatureToString(e1MTD));
      SizeW += SdifFPutSignature(SdifF,&(MatrixType->Signature));
      SizeW += fprintf(file, "\t{");

      ColumnDef = (SdifColumnDefT    *)SdifListGetHead(MatrixType->ColumnUserList); /* Reinit GetNext */
	  SizeW += fprintf(file, "%s",ColumnDef->Name);

      while (SdifListIsNext(MatrixType->ColumnUserList))
      {
          ColumnDef = (SdifColumnDefT    *)SdifListGetNext(MatrixType->ColumnUserList);
	      SizeW += fprintf(file, ", %s",ColumnDef->Name);
      }
      SizeW += fprintf(file, "}\n");  
    }

  return SizeW;
}


/*DOC:
  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
  Append matrix type declaration in SdifString
*/
int SdifFOneMatrixTypeToSdifString(SdifMatrixTypeT *MatrixType, SdifStringT *SdifString)
{
  int success = 1;
  SdifColumnDefT    *ColumnDef;
  
    success *= SdifStringAppend(SdifString, "  ");
    success *= SdifStringAppend(SdifString,SdifSignatureToString(e1MTD));
    success *= SdifStringAppend(SdifString, "  ");
    success *= SdifStringAppend(SdifString,SdifSignatureToString(MatrixType->Signature));
    success *= SdifStringAppend(SdifString,"\t{");
  if (! SdifListIsEmpty(MatrixType->ColumnUserList))
    {
      ColumnDef = (SdifColumnDefT    *)SdifListGetHead(MatrixType->ColumnUserList); /* Reinit GetNext */

      success *= SdifStringAppend(SdifString,ColumnDef->Name);
      
      while (SdifListIsNext(MatrixType->ColumnUserList))
	{
	  ColumnDef = (SdifColumnDefT    *)SdifListGetNext(MatrixType->ColumnUserList);
	  success *= SdifStringAppend(SdifString,", ");
	  success *= SdifStringAppend(SdifString,ColumnDef->Name);
	}
      
    }
    success *= SdifStringAppend(SdifString,"}\n");
  return(success);
}



size_t
SdifFPutAllMatrixType(SdifFileT* SdifF)
{
  unsigned int   iName;
  SdifHashNT    *pName;
  size_t         SizeW = 0;

  for(iName=0; iName<SdifF->MatrixTypesTable->HashSize; iName++)
    for (pName = SdifF->MatrixTypesTable->Table[iName]; pName;  pName=pName->Next)
      SizeW += SdifFPutOneMatrixType(SdifF, (SdifMatrixTypeT *)pName->Data);
  
  return SizeW;
}



/*DOC:
  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
  Write all Matrix type in SdifString
*/
int SdifFAllMatrixTypeToSdifString(SdifFileT *SdifF, SdifStringT *SdifString)
{
  unsigned int iName;
  SdifHashNT *pName;
  int success = 1;
  
  for(iName=0;iName<SdifF->MatrixTypesTable->HashSize; iName++)
    for(pName=SdifF->MatrixTypesTable->Table[iName]; pName; 
	pName = pName->Next)
      {
	success *= SdifFOneMatrixTypeToSdifString((SdifMatrixTypeT *)pName->Data, SdifString);
      }
  return success;
}



size_t
SdifFPutOneComponent(SdifFileT *SdifF, SdifComponentT *Component)
{
  size_t   SizeW = 0;
  FILE    *file = SdifF->TextStream;

  SizeW += fprintf(file, "\t  ");
  SizeW += SdifFPutSignature(SdifF,&Component->MtrxS);
  SizeW += fprintf(file, "\t%s;\n", Component->Name);
  
  return SizeW;
}


/*DOC:
  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
  Append one Component to SdifString
*/
int
SdifFOneComponentToSdifString(SdifComponentT *Component, SdifStringT *SdifString)
{
  int success = 1;

  success *= SdifStringAppend(SdifString, "\t  ");
  success *= SdifStringAppend(SdifString, SdifSignatureToString(Component->MtrxS));
  success *= SdifStringAppend(SdifString, "\t");
  success *= SdifStringAppend(SdifString, Component->Name);
  success *= SdifStringAppend(SdifString, ";\n");
  
  return success;
}



size_t
SdifFPutOneFrameType(SdifFileT *SdifF,  SdifFrameTypeT *FrameType)
{
  SdifUInt4 iC;
  size_t    SizeW = 0;
  FILE      *file = SdifF->TextStream;

  if (FrameType->NbComponentUse > 0)
    {
      SizeW += fprintf(file, "  %s\t", SdifSignatureToString(e1FTD));
      SizeW += SdifFPutSignature(SdifF,&(FrameType->Signature));
      SizeW += fprintf(file, "\n\t{\n");
      for(iC = FrameType->NbComponent - FrameType->NbComponentUse + 1;
          iC<= FrameType->NbComponent;
          iC++)
	    {
	      SizeW += SdifFPutOneComponent(SdifF, 
                                            SdifFrameTypeGetNthComponent(FrameType, iC));
	    }
      SizeW += fprintf(file, "\t}\n");
    }

  return SizeW;
}


/*DOC:
  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
  Append one frame type to SdifString
*/
int
SdifFOneFrameTypeToSdifString(SdifFrameTypeT *FrameType, SdifStringT *SdifString)
{
  SdifUInt4 iC;
  int success = 1; /* success of SdifStringAppend */

  if (FrameType->NbComponentUse > 0)
    {
      success *= SdifStringAppend(SdifString, "  ");
      success *= SdifStringAppend(SdifString, SdifSignatureToString(e1FTD));
      success *= SdifStringAppend(SdifString, "  ");
      success *= SdifStringAppend(SdifString, SdifSignatureToString(FrameType->Signature));
      success *= SdifStringAppend(SdifString, "\n\t{\n");
      
      for(iC = FrameType->NbComponent - FrameType->NbComponentUse + 1;
          iC<= FrameType->NbComponent;
          iC++)
	    {
	      success *= SdifFOneComponentToSdifString(
			      SdifFrameTypeGetNthComponent(FrameType, iC),
			      SdifString);
	    }
      success *= SdifStringAppend(SdifString,"\t}\n");
    }

  return success;      
}



size_t
SdifFPutAllFrameType(SdifFileT *SdifF)
{
  unsigned int  iName;
  SdifHashNT   *pName;
  size_t        SizeW = 0;

  for(iName=0; iName<SdifF->FrameTypesTable->HashSize; iName++)
    for (pName = SdifF->FrameTypesTable->Table[iName]; pName;  pName=pName->Next)
      SizeW += SdifFPutOneFrameType(SdifF, (SdifFrameTypeT *) pName->Data);

  return SizeW;
}


/*DOC:
  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
  Write all frame type in SdifString
*/
int
SdifFAllFrameTypeToSdifString(SdifFileT *SdifF, SdifStringT *SdifString)
{
  unsigned int  iName;
  SdifHashNT   *pName;
  int           success = 1;

  for(iName=0; iName<SdifF->FrameTypesTable->HashSize; iName++)
    for (pName = SdifF->FrameTypesTable->Table[iName]; pName;  pName=pName->Next)
      success *= SdifFOneFrameTypeToSdifString((SdifFrameTypeT *)pName->Data, SdifString);

  return success;
}  



/* SdifFPutAllType writes no '1TYP', no chunk size and no padding too */
size_t
SdifFPutAllType(SdifFileT *SdifF)
{
  size_t  SizeW = 0;
  FILE   *file=SdifF->TextStream;

  SizeW += fprintf(file, "{\n");  
  SizeW += SdifFPutAllMatrixType(SdifF);
  SizeW += SdifFPutAllFrameType(SdifF);  
  SizeW += fprintf(file, "}");

  return SizeW;
}




size_t
SdifFPutOneStreamID(SdifFileT *SdifF, SdifStreamIDT *StreamID)
{
  size_t   SizeW = 0;
  FILE    *file =SdifF->TextStream;

  SizeW += fprintf(file, "  %u ", StreamID->NumID);
  SizeW += fprintf(file, "%s:", StreamID->Source);
  SizeW += fprintf(file, "%s;\n", StreamID->TreeWay);

  return  SizeW;
}

/*DOC:
  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
  Append one StreamID to SdifString
*/
int
SdifFOneStreamIDToSdifString(SdifStringT *SdifString,
				  SdifStreamIDT *StreamID)
{
  int success = 1;
  char *tmpStr;
  int SizeW = 0;
  
  success *= SdifStringAppend(SdifString," ");
  
  /* Add StreamID->NumID  (unsigned int) */
  /* We use a temporary string for conversion in unsigned int */
  /* Memory allocation */
  tmpStr = (char *) malloc(_SdifStringGranule*sizeof(char));
  SizeW = sprintf(tmpStr,"%u",StreamID->NumID);
  success *=  SdifStringAppend(SdifString,tmpStr);
  free(tmpStr);
  
  success *= SdifStringAppend(SdifString," ");
  success *= SdifStringAppend(SdifString,StreamID->Source);
  success *= SdifStringAppend(SdifString,":");
  success *= SdifStringAppend(SdifString,StreamID->TreeWay);
  success *= SdifStringAppend(SdifString,";");
  success *= SdifStringAppend(SdifString,"\n");

  return success;
}

/* SdifFPutAllStreamID doesn't write '1IDS', chunk size and padding */
size_t
SdifFPutAllStreamID(SdifFileT *SdifF)
{
  unsigned int     iID;
  SdifHashNT *pID;
  size_t           SizeW = 0;
  FILE             *file =SdifF->TextStream;

  SizeW += fprintf(file, "{\n");
  
  for(iID=0; iID<SdifF->StreamIDsTable->SIDHT->HashSize; iID++)
    for (pID = SdifF->StreamIDsTable->SIDHT->Table[iID]; pID; pID = pID->Next)
      SizeW += SdifFPutOneStreamID(SdifF,  (SdifStreamIDT *)pID->Data);
  
  SizeW += fprintf(file, "}");

  return SizeW;
}

/*DOC:
  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
  Write all StreamID in SdifString
*/
int
SdifFAllStreamIDToSdifString(SdifFileT *SdifF, SdifStringT *SdifString)
{
  unsigned int iID;
  SdifHashNT   *pID;
  int          success;
  
  success = 1;
  
  for (iID = 0; iID<SdifF->StreamIDsTable->SIDHT->HashSize; iID++)
    for (pID = SdifF->StreamIDsTable->SIDHT->Table[iID]; pID; pID = pID->Next)
      success *= SdifFOneStreamIDToSdifString(SdifString, (SdifStreamIDT *)pID->Data);

  return success;
}
