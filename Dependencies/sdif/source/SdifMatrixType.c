/* $Id: SdifMatrixType.c,v 3.13 2008-12-18 11:42:19 diemo Exp $
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
 * Matrix Types management (interpreted sdif frame types)
 *  
 * author: Dominique Virolle 1997
 *
 *
 * SdifHashTableType to stock Matrix Types
 *      index : matrix type signature (HashInt4)
 *       Data : matrix type (SdifMatrixTypeT*)
 *     Killer : SdifKillMatrixType (if types are coded, Killer  == NULL)
 *
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.12  2007/12/10 10:46:20  roebel
 * Use const char* for read only function arguments.
 *
 * Revision 3.11  2005/10/21 14:32:30  schwarz
 * protect all static buffers from overflow by using snprintf instead of sprintf
 * move big errorMess buffers into error branch to avoid too large stack allocation
 *
 * Revision 3.10  2004/07/22 14:47:56  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.9  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.8  2003/10/14 10:10:37  schwarz
 * SdifMatrixTypeGetColumnName returns pointer to name of column at index.
 *
 * Revision 3.7  2002/05/24 19:37:52  ftissera
 * Change code to be compatible with C++
 * Cast pointers to correct type.
 *
 * Revision 3.6  2001/05/02 09:34:46  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.5  2000/11/15 14:53:32  lefevre
 * no message
 *
 * Revision 3.4  2000/10/27  20:03:38  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.3.2.1  2000/08/21  21:35:35  tisseran
 * *** empty log message ***
 *
 * Revision 3.3  2000/05/15  16:22:32  schwarz
 * Avoid warning about KillerFT function pointer type (ANSI prototype given).
 * Argument to kill func is now void *.
 *
 * Revision 3.2  1999/09/28  13:09:04  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.1  1999/03/14  10:57:08  virolle
 * SdifStdErr add
 *
 * Revision 2.4  1999/02/28  12:16:51  virolle
 * memory report
 *
 * Revision 2.3  1999/01/23  15:55:55  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:40  virolle
 * General Lists, and special chunk preparation to become frames
 *
 *
 *
 */


#include "sdif_portability.h"

#include "SdifMatrixType.h"
#include <stdlib.h>





SdifColumnDefT*
SdifCreateColumnDef(const char *Name, SdifUInt4 Num)
{
  SdifColumnDefT *NewColumnDef = NULL;
  
  NewColumnDef = SdifMalloc(SdifColumnDefT);
  if (NewColumnDef != NULL)
    {
      NewColumnDef->Name = SdifCreateStrNCpy(Name, SdifStrLen(Name)+1);
      NewColumnDef->Num = Num;
      
      return NewColumnDef;
    }
  else
    {
      _SdifError(eAllocFail, "ColumnDef allocation");
      return NULL;
    }
}






void
SdifKillColumnDef(void *ColumnDef)
{
  if (ColumnDef)
    {
      SdifKillStr(((SdifColumnDefT *) ColumnDef)->Name);
      SdifFree(ColumnDef);
    }
  else
    _SdifError(eFreeNull, "ColumnDef free");
}








SdifMatrixTypeT*
SdifCreateMatrixType(SdifSignature Signature, SdifMatrixTypeT *PredefinedMatrixType)
{
  SdifMatrixTypeT *NewMatrixType = NULL;

  NewMatrixType = SdifMalloc(SdifMatrixTypeT);
  if (NewMatrixType)
    {
      NewMatrixType->Signature      = Signature;
      NewMatrixType->ColumnUserList = SdifCreateList(SdifKillColumnDef);
      NewMatrixType->NbColumnDef    = 0;
      NewMatrixType->ModifMode      = eCanModif;
      
      if (PredefinedMatrixType)
	{
	  if (PredefinedMatrixType->Signature != Signature)
	    {
	      char errorMess[_SdifStringLen];

	      snprintf(errorMess, sizeof(errorMess), "'%s'(Pre) != '%s'",
		      SdifSignatureToString(PredefinedMatrixType->Signature),
		      SdifSignatureToString(Signature));
	      _SdifError(eInvalidPreType, errorMess);
	    }
	  else
	    {
	      NewMatrixType->MatrixTypePre  = PredefinedMatrixType;
	      NewMatrixType->NbColumnDef    = PredefinedMatrixType->ColumnUserList->NbData;
	    }
	}
      else
	{
	  NewMatrixType->MatrixTypePre  = NULL;
	}
     
      return NewMatrixType;
    }
  else
    {
      _SdifError(eAllocFail, "MatrixType allocation");
      return NULL;
    }
}






void
SdifKillMatrixType(SdifMatrixTypeT *MatrixType)
{

  /* don't kill the predfined type because it is one of another bank of types (Predefined Types)*/

  if (MatrixType)
    {
        SdifKillList(MatrixType->ColumnUserList);
        SdifFree(MatrixType);
    }
  else
    _SdifError(eFreeNull, "MatrixType free");
}







SdifUInt4
SdifMatrixTypeGetNumColumnDef(SdifMatrixTypeT *MatrixType, const char *NameCD)
{
    int Num = 0;
    SdifColumnDefT* CurrColumnDef = NULL;

    if (MatrixType->MatrixTypePre)
        Num = SdifMatrixTypeGetNumColumnDef(MatrixType->MatrixTypePre, NameCD);

    if (Num == 0)
    {
        SdifListInitLoop(MatrixType->ColumnUserList);
        while (SdifListIsNext(MatrixType->ColumnUserList))
        {
            CurrColumnDef = (SdifColumnDefT*) 
		SdifListGetNext(MatrixType->ColumnUserList);
            if (SdifStrCmp(CurrColumnDef->Name, NameCD) == 0)
            {
	            Num = CurrColumnDef->Num;
	            break;
	        }
        }
    }
  
  return Num;
}









SdifColumnDefT*
SdifMatrixTypeGetColumnDef(SdifMatrixTypeT *MatrixType, const char *NameCD)
{
  SdifColumnDefT *ColumnDef = NULL;
  SdifColumnDefT *CurrColumnDef = NULL;

  if (MatrixType->MatrixTypePre)
    ColumnDef = SdifMatrixTypeGetColumnDef(MatrixType->MatrixTypePre, NameCD);
  
  if (! ColumnDef)
    {
        SdifListInitLoop(MatrixType->ColumnUserList);
        while (SdifListIsNext(MatrixType->ColumnUserList))
        {
            CurrColumnDef = (SdifColumnDefT*)
		SdifListGetNext(MatrixType->ColumnUserList);
            if (SdifStrCmp(CurrColumnDef->Name, NameCD) == 0)
            {
	            ColumnDef = CurrColumnDef;
	            break;
	        }
        }
    }
  
  return ColumnDef;
}






SdifColumnDefT*
SdifMatrixTypeGetNthColumnDef(SdifMatrixTypeT *MatrixType, SdifUInt4 NumCD)
{
  SdifColumnDefT *ColumnDef = NULL;
  SdifColumnDefT *CurrColumnDef = NULL;

  if (MatrixType->MatrixTypePre)
    ColumnDef = SdifMatrixTypeGetNthColumnDef(MatrixType->MatrixTypePre, NumCD);

  if (! ColumnDef)
    {
        SdifListInitLoop(MatrixType->ColumnUserList);
        while (SdifListIsNext(MatrixType->ColumnUserList))
        {
            CurrColumnDef = (SdifColumnDefT*)
		SdifListGetNext(MatrixType->ColumnUserList);
            if (CurrColumnDef->Num == NumCD)
            {
	            ColumnDef = CurrColumnDef;
	            break;
	        }
        }
    }
  
  return ColumnDef;
}



/* SdifMatrixTypeGetColumnName returns pointer to name of column at index. */
const char* SdifMatrixTypeGetColumnName (SdifMatrixTypeT *mtype, int index)
{
    SdifColumnDefT *cd = SdifMatrixTypeGetNthColumnDef(mtype, index);

    if (cd)
	return cd->Name;
    else
	return NULL;
}


SdifUInt4
SdifMatrixTypeGetNbColumns(SdifMatrixTypeT *mtype)
{
    return mtype->NbColumnDef;
}




SdifMatrixTypeT*
SdifMatrixTypeInsertTailColumnDef(SdifMatrixTypeT *MatrixType, const char *NameCD)
{
  
  if (SdifMatrixTypeGetColumnDef(MatrixType, NameCD))
    {
      _SdifError(eExistYet, NameCD);
      return NULL;
    }

  switch (MatrixType->ModifMode)
    {
    case eNoModif:
        _SdifError(eNoModifErr, SdifSignatureToString(MatrixType->Signature));
        return NULL;
    case eCanModif:
        /* then add ColumnDef in the Columns list */
        SdifListPutTail(MatrixType->ColumnUserList,
                        SdifCreateColumnDef(NameCD, MatrixType->NbColumnDef+1));
        break;
    default :
        return NULL;
    }

  MatrixType->NbColumnDef++;
  return MatrixType;
}







SdifMatrixTypeT*
SdifGetMatrixType(SdifHashTableT *MatrixTypesTable, SdifSignature Signature)
{
  return (SdifMatrixTypeT*)SdifHashTableSearch(MatrixTypesTable, &Signature, 1);
}


void
SdifPutMatrixType(SdifHashTableT *MatrixTypesTable, SdifMatrixTypeT* MatrixType)
{
  SdifHashTablePut(MatrixTypesTable, &(MatrixType->Signature), 1, MatrixType);
}




SdifUInt2
SdifExistUserMatrixType(SdifHashTableT *MatrixTypesTable)
{
  unsigned int     iName;
  SdifHashNT*      pName;
  SdifMatrixTypeT*  MtrxT;
  
  for(iName=0; iName<MatrixTypesTable->HashSize; iName++)
    for (pName = MatrixTypesTable->Table[iName]; pName;  pName=pName->Next)
      {
        MtrxT = (SdifMatrixTypeT*) pName->Data;
        if (SdifListGetNbData(MtrxT->ColumnUserList) > 0)
            return 1;
      }
  return 0;
}
