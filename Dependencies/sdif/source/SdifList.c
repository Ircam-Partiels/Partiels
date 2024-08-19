/*
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
 * SdifList.c
 *
 * SdifList is coded like a small independant library.
 * Every List have its own size and data type.
 * Every data type must have a function to free memory : 
 *     void <KillerName>(<type>* ptr).
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.13  2007/03/21 19:44:15  roebel
 * Don't use global variables without initialization. These globals are treated differently
 * on MacOSX and they are not allowed in dynamic libraries without extra flags.
 * To simplify the situation I now initialized all global variables
 * or make them static.
 *
 * Revision 3.12  2005/05/23 17:52:53  schwarz
 * Unified error handling:
 * - SdifErrorEnum (global errors) integrated into SdifErrorTagET (file errors)
 * - no more SdifError.[ch], everything done by SdifErrMess.[ch]
 *
 * Revision 3.11  2005/04/07 15:56:47  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.10  2004/09/09 17:43:39  schwarz
 * SdifListConcat function
 *
 * Revision 3.9  2004/07/22 14:47:56  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.8  2003/12/15 13:15:55  schwarz
 * SdifKillListCurr for SdifNameValuesLKillCurrNVT, untested
 *
 * Revision 3.7  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.6  2001/05/02 09:34:45  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.5  2000/11/15 14:53:31  lefevre
 * no message
 *
 * Revision 3.4  2000/10/27  20:03:36  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.3.2.1  2000/08/21  21:35:31  tisseran
 * *** empty log message ***
 *
 * Revision 3.3  2000/03/01  11:19:23  schwarz
 * SdifListInitLoop also returns whether list is full.
 * Cocoonable comments.
 *
 * Revision 3.2  1999/09/28  13:09:02  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.1  1999/03/14  10:57:03  virolle
 * SdifStdErr add
 *
 * Revision 2.3  1999/02/28  12:16:49  virolle
 * memory report
 *
 * Revision 2.2  1999/01/23  15:55:52  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.1  1999/01/23  13:57:37  virolle
 * General Lists, and special chunk preparation to become frames
 */

#include "sdif_portability.h"

#include "SdifGlobals.h"
#include "SdifList.h"
#include <stdio.h>
#include <stdlib.h>


/* stocks management */


void
SdifInitListNStock(SdifListNStockT *Stock, unsigned int SizeOfOneStock)
{
    Stock->StockList                = NULL;    
    Stock->SizeOfOneStock           = SizeOfOneStock;    
    Stock->NbStock                  = 0;
    Stock->NbNodesUsedInCurrStock   = 0;    
    Stock->Trash                    = NULL;    
}



void
SdifNewStock(SdifListNStockT *Stock)
{
    SdifListNT* NewStock;

    NewStock = SdifCalloc(SdifListNT, Stock->SizeOfOneStock);
    if (NewStock)
    {
        (NewStock[0]).Next              = Stock->StockList;
        Stock->StockList                = NewStock;
        Stock->NbStock++;
        Stock->NbNodesUsedInCurrStock   = 1;
        /* first is used yet (cf. (NewStock[0]).Next in this function) */
    }
}


SdifListNT*
SdifGetNewNodeFromTrash(SdifListNStockT *Stock)
{
    SdifListNT* RecycledNode;

    RecycledNode = Stock->Trash;
    Stock->Trash = Stock->Trash->Next;
    return RecycledNode;
}



SdifListNT*
SdifGetNewNodeFromStock(SdifListNStockT *Stock)
{
    if (    (Stock->NbStock ==0)
         || (Stock->NbNodesUsedInCurrStock == Stock->SizeOfOneStock) )
    {
        SdifNewStock(Stock);
    }

    return &(Stock->StockList[(Stock->NbNodesUsedInCurrStock)++]);
}


SdifListNT*
SdifGetNewNode(SdifListNStockT *Stock)
{
    if (Stock->Trash)
        return SdifGetNewNodeFromTrash(Stock);
    else
        return SdifGetNewNodeFromStock(Stock);
}


void
SdifPutNodeInTrash(SdifListNStockT *Stock, SdifListNT* OldNode)
{
    OldNode->Next = Stock->Trash;
    Stock->Trash = OldNode;
}



SdifListNT*
SdifKillListNStock(SdifListNT* OldStock)
{
    SdifListNT* NextStock;

    NextStock = (OldStock[0]).Next;
    SdifFree(OldStock);
    return NextStock;
}



void
SdifListNStockMakeEmpty(SdifListNStockT *Stock)
{
    while (Stock->StockList)
    {
       Stock->StockList = SdifKillListNStock(Stock->StockList);
    }
    SdifInitListNStock(Stock, Stock->SizeOfOneStock);
}



/* global variable gSdifListNodeStock */



SdifListNStockT gSdifListNodeStock = { 0, 2, 0, 0, 0};

SdifListNStockT*
SdifListNodeStock(void)
{
#if HAVE_PTHREAD
	struct SdifGlobals* globals = GetSdifGlobals();
	return &(globals->sdifListNodeStock);
#else
    return & gSdifListNodeStock;
#endif
}

void
SdifInitListNodeStock(unsigned int SizeOfOneStock)
{
    SdifInitListNStock(SdifListNodeStock(), SizeOfOneStock);
}


void SdifDrainListNodeStock(void)
{
    SdifListNStockMakeEmpty(SdifListNodeStock());
}


/* nodes management */


SdifListNT*
SdifCreateListNode (SdifListNT *Next, void *Data)
{
    SdifListNT *NewNode;
    NewNode = SdifGetNewNode(SdifListNodeStock());
    NewNode->Next = Next;
    NewNode->Data = Data;
    return NewNode;
}




SdifListNT*
SdifKillListNode (SdifListNT *Node, KillerFT Killer)
{
    SdifListNT* Next;

    Next = Node->Next;
    if (Killer != NULL)
        (*(Killer))(Node->Data); /* this line can occasion a warning */

    SdifPutNodeInTrash (SdifListNodeStock(), Node);
    return Next;
}



/* user lists management */

SdifListT*
SdifCreateList(KillerFT Killer)
{
    SdifListT* NewList;

    NewList = SdifMalloc(SdifListT);
    if (NewList)
    {
        NewList->Head = NULL;
        NewList->Tail = NULL;
        NewList->Curr = NULL;
        NewList->Killer = Killer;
        NewList->NbData = 0;
        return NewList;
    }
    else
    {
      _SdifError(eAllocFail, "List allocation");
      return NULL;
    }
}


SdifListT*
SdifKillListHead(SdifListT* List)
{
    if (List->Head)
    {
        if (List->Head == List->Tail)
            List->Tail = NULL;

        List->Head = SdifKillListNode(List->Head, List->Killer);
        List->NbData--;
        return List;
    }
    else
    {
      _SdifError(eFreeNull, "KillListHead");
      return NULL;
    }
}


SdifListT*
SdifKillListCurr(SdifListT* List)
{
    if (List->Curr)
    {
        if (List->Curr == List->Head)
	    SdifKillListHead(List);
	else 
	{
	    if (List->Curr == List->Tail)
	    {   /* curr is tail, find elem before curr to be new tail */
		SdifUInt4 i;

		List->Tail = List->Head;
		for (i = 0; i < List->NbData - 1; i++)
		    List->Tail = List->Tail->Next;
	    }
	    
	    List->Curr = SdifKillListNode(List->Curr, List->Killer);
	    List->NbData--;
	}
        return List;
    }
    else
    {
      _SdifError(eFreeNull, "KillListCurr");
      return NULL;
    }
}


SdifListT*
SdifMakeEmptyList (SdifListT* List)
{
    while(List->Head)
        SdifKillListHead(List);

    return List;
}



void
SdifKillList (SdifListT* List)
{
    if (List)
    {
        SdifMakeEmptyList(List);
        SdifFree(List);
    }
    else
    {
      _SdifError(eFreeNull, "KillList");
    }
}





void*
SdifListGetHead (SdifListT* List)
{
    if (!List->Head)
    {
        return NULL;
    }
    else
    {
        List->Curr = List->Head;
        return List->Head->Data;
    }
}



void*
SdifListGetTail (SdifListT* List)
{
    if (List->Tail)
        return List->Tail->Data;
    else
        return NULL;
}




int
SdifListIsNext(SdifListT* List)
{
    return ( (List->Head != NULL) && (List->Curr != List->Tail) );
}



int
SdifListIsEmpty(SdifListT* List)
{
    return (List->Head == NULL);
}



int
SdifListInitLoop(SdifListT* List)
{
    List->Curr = NULL;
    return (List->Head != NULL);	/* true if List has elements */
}


void*
SdifListGetNext (SdifListT* List)
{
    if (List->Curr == NULL)
    {
        return SdifListGetHead(List);
    }
    else
    {
        if (List->Curr != List->Tail)
        {
            List->Curr= List->Curr->Next;
            return List->Curr->Data;
        }
        else
        {
          _SdifError(eArrayPosition, "ListGetNext error, current node is the tail");
          return NULL;
        }
    }
}



void*
SdifListGetCurr(SdifListT* List)
{
    if (List->Curr == NULL)
        return NULL;
    else
        return List->Curr->Data;
}



SdifListT*
SdifListPutTail (SdifListT* List, void *pData)
{   
    if (List->Head == NULL) /* first node */
    {
        List->Head = SdifCreateListNode(NULL, pData);
        List->Tail = List->Head;
    }
    else
    {
        List->Tail->Next = SdifCreateListNode(NULL, pData);
        List->Tail = List->Tail->Next;
    }
    List->NbData++;

    return List;
}




SdifListT*
SdifListPutHead (SdifListT* List, void *pData)
{
    if (List->Head == NULL) /* first node */
    {
        List->Head = SdifCreateListNode(NULL, pData);
        List->Tail = List->Head;
    }
    else
    {
        List->Head = SdifCreateListNode(List->Head->Next, pData);
    }
    List->NbData++;

    return List;
}


unsigned int
SdifListGetNbData(SdifListT* List)
{
    return List->NbData;
}


/* append list b to list a, creating double references to the data! */
SdifListT *SdifListConcat(SdifListT *a, SdifListT *b)
{
    SdifListInitLoop(b);

    while (SdifListIsNext(b))
    {
	SdifListPutTail(a, SdifListGetNext(b));
    }

    return a;
}
