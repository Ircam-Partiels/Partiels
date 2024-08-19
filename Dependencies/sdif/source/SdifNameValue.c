/* $Id: SdifNameValue.c,v 3.18 2009-01-07 16:23:27 diemo Exp $
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
 * SdifNameValue.c
 *
 * Name values management. For 1 SdifFileT*, we have one SdifNameValueLT*
 * which contains a list of hash tables. Each hach table contains name-values.
 *
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.17  2005/04/07 15:56:47  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.16  2003/12/15 13:15:40  schwarz
 * SdifNameValuesLKillCurrNVT, untested
 *
 * Revision 3.15  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.14  2003/08/06 15:10:47  schwarz
 * Finally removed obsolete functions (like SdifSkip...).
 *
 * Revision 3.13  2002/12/17 15:56:52  roebel
 * Comment changed.
 *
 * Revision 3.12  2002/12/13 15:58:12  roebel
 * Fixed two bugs in SdifNameValuesLSetCurrNVT. Name table access would change
 * the name table indexes due to faulty assignment instead of comparison.
 *
 * Revision 3.11  2002/05/24 19:37:52  ftissera
 * Change code to be compatible with C++
 * Cast pointers to correct type.
 *
 * Revision 3.10  2001/07/19 14:24:34  lefevre
 * Macintosh Compilation
 *
 * Revision 3.9  2001/05/04  18:14:18  schwarz
 * Comments for last checkin.
 *
 * Revision 3.8  2001/05/04 18:09:18  schwarz
 * Added function SdifNameValuesLPutCurrNVTTranslate.
 *
 * Revision 3.7  2001/05/02 09:34:46  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.6  2000/11/15 14:53:33  lefevre
 * no message
 *
 * Revision 3.5  2000/10/27  20:03:39  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.4.2.1  2000/08/21  21:35:39  tisseran
 * *** empty log message ***
 *
 * Revision 3.4  2000/05/15  16:22:34  schwarz
 * Avoid warning about KillerFT function pointer type (ANSI prototype given).
 * Argument to kill func is now void *.
 *
 * Revision 3.3  1999/10/15  12:26:53  schwarz
 * No time parameter for name value tables and stream ID tables, since
 * this decision is better left to the library.  (It uses the _SdifNoTime
 * constant, which happens to be _Sdif_MIN_DOUBLE_.)
 *
 * Revision 3.2  1999/09/28  13:09:07  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.1  1999/03/14  10:57:12  virolle
 * SdifStdErr add
 *
 * Revision 2.4  1999/02/28  12:16:54  virolle
 * memory report
 *
 * Revision 2.3  1999/01/23  15:55:56  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:41  virolle
 * General Lists, and special chunk preparation to become frames
 *
 * Revision 2.1  1998/12/21  18:27:32  schwarz
 * Inserted copyright message.
 *
 * Revision 2.0  1998/11/29  11:41:58  virolle
 * - New management of interpretation errors.
 * - Alignement of frames with CNMAT (execpt specials Chunk 1NVT, 1TYP, 1IDS).
 * _ Sdif Header File has a Sdif format version.
 * - Matrices order in frames is not important now. (only one occurence of
 *   a Matrix Type in a Frame Type declaration )
 * - Hard coded predefined types more dynamic management.
 * - Standart streams (stdin, stdout, stderr) set as binary for Windows32 to
 *   have exactly the same result on each plateforme.
 *
 * Revision 1.3  1998/04/24  12:40:35  schwarz
 * Made char * arguments constant for SdifNameValuesLPut and functions called by it.
 */


#include "sdif_portability.h"

#include <string.h>
#include <stdlib.h>

#include <sdif.h>
#include "SdifGlobals.h"



/*
 * NameValue
 */

SdifNameValueT*
SdifCreateNameValue(const char *Name,  const char *Value)
{
  SdifNameValueT *NewNameValue = NULL;

  NewNameValue = SdifMalloc(SdifNameValueT);
  if (NewNameValue)
    {
      NewNameValue->Name  = SdifCreateStrNCpy(Name, SdifStrLen(Name)+1);
      NewNameValue->Value = SdifCreateStrNCpy(Value, SdifStrLen(Value)+1);

      return NewNameValue;
    }
  else
    {
      _SdifError(eAllocFail, "NameValue allocation");
      return NULL;
    }
}



void
SdifKillNameValue(SdifNameValueT *NameValue)
{
  if (NameValue)
    {
      SdifKillStr(NameValue->Name);
      SdifKillStr(NameValue->Value);
      SdifFree(NameValue);
    }
  else
    _SdifError(eFreeNull, "NameValue free");
}


char *SdifNameValueGetName (SdifNameValueT *nv)
{
    return nv->Name;
}

char *SdifNameValueGetValue (SdifNameValueT *nv)
{
    return nv->Value;
}




/*
 * NameValueTable
 */



SdifNameValueTableT*
SdifCreateNameValueTable (SdifUInt4 StreamID,
                          SdifUInt4 HashSize,
                          SdifUInt4 NumTable)
{
    SdifNameValueTableT* NewNVTable;

    NewNVTable = SdifMalloc(SdifNameValueTableT);
    if (NewNVTable)
    {
        NewNVTable->StreamID   = StreamID;
        NewNVTable->NVHT       = SdifCreateHashTable(HashSize, eHashChar, 
						     (void(* )(void *)) SdifKillNameValue);
        NewNVTable->NumTable   = NumTable;
        return NewNVTable;
    }
    else
    {
      _SdifError(eAllocFail, "NameValueTable allocation");
      return NULL;
    }
}


/* does not remove NVT from list! */
void
SdifKillNameValueTable  (void* NVTable)
{
  if (NVTable)
    {
      SdifKillHashTable(((SdifNameValueTableT *) NVTable)->NVHT);
      SdifFree(NVTable);
    }
  else
    _SdifError(eFreeNull, "NameValueTable free");
}





SdifNameValueT*
SdifNameValueTableGetNV(SdifNameValueTableT* NVTable, const char *Name)
{
    return (SdifNameValueT*) SdifHashTableSearch(NVTable->NVHT, (char*) Name, SdifStrLen(Name)+1);
}



SdifNameValueT*
SdifNameValueTablePutNV(SdifNameValueTableT* NVTable, const char *Name,  const char *Value)
{
  SdifNameValueT *NewNameValue = NULL;
  
  NewNameValue = SdifCreateNameValue(Name, Value);

  if (NewNameValue)
    SdifHashTablePut(NVTable->NVHT, Name, SdifStrLen(Name)+1, NewNameValue);

  return NewNameValue;
}


SdifUInt4
SdifNameValueTableGetNumTable(SdifNameValueTableT* NVTable)
{
    return NVTable->NumTable;
}


SdifUInt4
SdifNameValueTableGetStreamID(SdifNameValueTableT* NVTable)
{
    return NVTable->StreamID;
}


SdifHashTableT *
SdifNameValueTableGetHashTable (SdifNameValueTableT* NVTable)
{
    return NVTable->NVHT;
}




/*
 * NameValueTableList
 */




SdifNameValuesLT*
SdifCreateNameValuesL(SdifUInt4  HashSize)
{
  SdifNameValuesLT *NewNameValuesL = NULL;
  
  NewNameValuesL = SdifMalloc(SdifNameValuesLT);

  if (NewNameValuesL)
    {
      NewNameValuesL->NVTList   = SdifCreateList(SdifKillNameValueTable);
      NewNameValuesL->CurrNVT   = NULL;
      NewNameValuesL->HashSize  = HashSize;
      
      return NewNameValuesL;
    }
  else
    {
      _SdifError(eAllocFail, "NameValues List allocation");
      return NULL;
    }
}




void
SdifKillNameValuesL(SdifNameValuesLT *NameValuesL)
{
  if (NameValuesL)
    {
      SdifKillList(NameValuesL->NVTList);
      SdifFree(NameValuesL);
    }
  else
    _SdifError(eFreeNull, "NameValues List free");
}





SdifNameValuesLT*
SdifNameValuesLNewTable(SdifNameValuesLT *NameValuesL, SdifUInt4 StreamID)
{
    SdifNameValueTableT* NewNVT;

    NewNVT = SdifCreateNameValueTable(  StreamID,
                                        NameValuesL->HashSize,
                                        SdifListGetNbData(NameValuesL->NVTList) + 1);

    SdifListPutTail(NameValuesL->NVTList, NewNVT);

    NameValuesL->CurrNVT = NewNVT;
    
    return NameValuesL;
}




SdifNameValueTableT*
SdifNameValuesLSetCurrNVT(SdifNameValuesLT *NameValuesL, SdifUInt4 NumCurrNVT)
/* condition of succes : NumCurrNVT < SdifListGetNbData(NameValuesL->NVTList)
 */
{
    SdifNameValueTableT* NVT=NULL;


    SdifUInt4 nb =  SdifListGetNbData(NameValuesL->NVTList);
    if(nb < NumCurrNVT ){
      _SdifError(eArrayPosition, "SdifNameValuesLSetCurrNVT: name table size exeeded");
      NameValuesL->CurrNVT = 0;
      return  NameValuesL->CurrNVT;
    }

    NVT = (SdifNameValueTableT*) SdifListGetCurr(NameValuesL->NVTList);
    if (NVT)
    {
      /* If current name table is  below the one that is requested
       * we just follow the list */
      if (NVT->NumTable <= NumCurrNVT)
        {

	  if ((NVT->NumTable == NumCurrNVT))
	    {
	      return NVT;
	    }


	  while (SdifListIsNext(NameValuesL->NVTList))
	    {
	      NVT = (SdifNameValueTableT*) SdifListGetNext(NameValuesL->NVTList);
	      if ((NVT->NumTable == NumCurrNVT))
		{
		  NameValuesL->CurrNVT = NVT;
		  break;
		}
	    }
	  return NameValuesL->CurrNVT;
        }
    }


    NVT=NULL;
    SdifListInitLoop(NameValuesL->NVTList);
    while (SdifListIsNext(NameValuesL->NVTList))
    {
        NVT = (SdifNameValueTableT*) SdifListGetNext(NameValuesL->NVTList);
        if ((NVT->NumTable == NumCurrNVT))
        {
            NameValuesL->CurrNVT = NVT;
            break;
        }
    }
  return NameValuesL->CurrNVT;
}


void SdifNameValuesLKillCurrNVT(SdifNameValuesLT *NameValuesL)
{
    SdifKillListCurr(NameValuesL->NVTList);
}



/* Return nvt for key Name found in first NVT */
SdifNameValueT*
SdifNameValuesLGet(SdifNameValuesLT *NameValuesL, char *Name)
{
  SdifNameValueT
    *NameValue = NULL;
  SdifNameValueTableT * CurrTable;

  SdifListInitLoop(NameValuesL->NVTList);
  while (   (!NameValue)  && (SdifListIsNext(NameValuesL->NVTList)) )
  {
      CurrTable = (SdifNameValueTableT *)SdifListGetNext(NameValuesL->NVTList);
      NameValue = (SdifNameValueT *) SdifNameValueTableGetNV(CurrTable, Name);
  }

  return NameValue;
}



SdifNameValueT*
SdifNameValuesLGetCurrNVT(SdifNameValuesLT *NameValuesL, const char *Name)
{
  return SdifNameValueTableGetNV(NameValuesL->CurrNVT, Name);
}



SdifNameValueT*
SdifNameValuesLPutCurrNVT(SdifNameValuesLT *NameValuesL, 
		   const char *Name,  const char *Value)
{
    return SdifNameValueTablePutNV(NameValuesL->CurrNVT, Name, Value);
}


/* Add a Name-Value pair to the current Name-Value Table, while
   replacing reserved characters and spaces with underscores "_" (using
   SdifStringToNV).  FYI: The strings are copied. */
SdifNameValueT*
SdifNameValuesLPutCurrNVTTranslate(SdifNameValuesLT *NameValuesL, 
				   const char *Name,  const char *Value)
{
    SdifNameValueT* ret;
	char *tname  = (char *) malloc(strlen(Name)  + 4);
	char *tvalue = (char *) malloc(strlen(Value) + 4);

	strcpy(tname,  Name);
	strcpy(tvalue, Value);

    SdifStringToNV(tname);
    SdifStringToNV(tvalue);

    ret = SdifNameValueTablePutNV(NameValuesL->CurrNVT, tname, tvalue);

    free(tname);
    free(tvalue);
    return ret;
}


SdifUInt2
SdifNameValuesLIsNotEmpty(SdifNameValuesLT *NameValuesL)
{
    SdifNameValueTableT* NVT;

    if (SdifListIsEmpty(NameValuesL->NVTList))
        return 0;
    else
    {
        SdifListInitLoop(NameValuesL->NVTList);
        while (SdifListIsNext(NameValuesL->NVTList))
        {
            NVT = (SdifNameValueTableT*)SdifListGetNext(NameValuesL->NVTList);
            if (NVT->NVHT->NbOfData > 0)
                return 1;
        }
    }
  return 0;
}


/* get generic sdif list with nvts from nvt container struct nvtl */
SdifListT *
SdifNameValueTableList (SdifNameValuesLT *nvtl)
{
    return nvtl->NVTList;
}
