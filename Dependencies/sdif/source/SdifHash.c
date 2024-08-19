/* $Id: SdifHash.c,v 3.12 2009-01-07 16:24:01 diemo Exp $
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
 * SdifHash.c
 *
 * SdifHash is coded like a small independant library.
 * Every HashTable have its own size and data type.
 * Every data type must have a function to free memory : 
 *     void <KillerName>(<type>* ptr).
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.11  2008/12/18 11:42:19  diemo
 * Improvements of the public API for scripting languages binding to libSDIF:
 * - added SdifHashTableGetNbData
 * - added access methods to struct elements of type definitions.
 *
 * Revision 3.10  2005/05/23 17:52:53  schwarz
 * Unified error handling:
 * - SdifErrorEnum (global errors) integrated into SdifErrorTagET (file errors)
 * - no more SdifError.[ch], everything done by SdifErrMess.[ch]
 *
 * Revision 3.9  2005/04/07 15:56:47  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.8  2004/09/09 17:43:14  schwarz
 * local SdifHashInt4 function as macro
 *
 * Revision 3.7  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.6  2002/05/24 19:37:52  ftissera
 * Change code to be compatible with C++
 * Cast pointers to correct type.
 *
 * Revision 3.5  2001/05/02 09:34:44  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.4  2000/11/15 14:53:31  lefevre
 * no message
 *
 * Revision 3.3  2000/10/27  20:03:35  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.2.2.1  2000/08/21  21:35:27  tisseran
 * *** empty log message ***
 *
 * Revision 3.2  1999/09/28  13:09:01  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.1  1999/03/14  10:57:01  virolle
 * SdifStdErr add
 *
 * Revision 2.2  1999/02/28  12:16:48  virolle
 * memory report
 *
 * Revision 2.1  1998/12/21  18:27:26  schwarz
 * Inserted copyright message.
 *
 * Revision 2.0  1998/11/29  11:41:51  virolle
 * - New management of interpretation errors.
 * - Alignement of frames with CNMAT (execpt specials Chunk 1NVT, 1TYP, 1IDS).
 * _ Sdif Header File has a Sdif format version.
 * - Matrices order in frames is not important now. (only one occurence of
 *   a Matrix Type in a Frame Type declaration )
 * - Hard coded predefined types more dynamic management.
 * - Standart streams (stdin, stdout, stderr) set as binary for Windows32 to
 *   have exactly the same result on each plateforme.
 *
 * Revision 1.3  1998/05/04  15:44:17  schwarz
 * Replaced constant char* args by const char* (also some void*).
 *
 */

#include "sdif_portability.h"

#include "SdifGlobals.h"
#include "SdifHash.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


SdifHashTableT*
SdifCreateHashTable(unsigned int HashSize,
		    SdifHashIndexTypeET IndexType,
		    void (*Killer)(void *))
{
/*
 * The hash table size is better if it is a prime number like : 31, 61, 127, 251...
 * Killer is the name of the hash table data type destructor.
 *  ( void (*Killer)() ) No verification of arguments.
 * This can occasion a warning at the compilation :
 *     Warning <...>: SdifHash.c, line <...>:
 *     No prototype for the call to an anonymous function
 *                    (*HTable->Killer)(pNode->Data);
 *                    -----------------^
 *     To achieve better type-checking, there should be a full prototype for
 *     the function being called.
 */
  SdifHashTableT *NewTable = NULL;
  SdifHashNT **CurrTable;
  unsigned int i;

  NewTable = SdifMalloc(SdifHashTableT);
  if (NewTable)
    {
      NewTable->HashSize = HashSize;
      NewTable->NbOfData = 0;
      NewTable->IndexType = IndexType;
      NewTable->Killer = Killer;
      NewTable->Table = SdifCalloc(SdifHashNT*, HashSize);
      if (NewTable->Table)
	    {
	      CurrTable = NewTable->Table;
	      for(i=0; i<HashSize; i++)
	       *CurrTable++ = NULL;
	    }
      else
	    {
	      _SdifError(eAllocFail, "HashTable allocation");
	     return NULL;
        }
      return NewTable;
    }
  else
    {
      _SdifError(eAllocFail, "HashTable allocation");
      return NULL;
    }
}


unsigned int 
SdifHashTableGetNbData (SdifHashTableT* HTable)
{
    return HTable->NbOfData;
}

void *
SdifHashTableGetData (SdifHashTableT* HTable, int i)
{
    return HTable->Table[i]->Data;
}


void
SdifMakeEmptyHashTable(SdifHashTableT* HTable)
{
  unsigned int i;
  SdifHashNT *pNode;
  
  for(i=0; i<HTable->HashSize; i++)
  {
      while (HTable->Table[i])
      {
          pNode = HTable->Table[i];
          if (HTable->IndexType == eHashChar)
	      SdifFree(pNode->Index.Char[0]);
          if (HTable->Killer)
	      (*(HTable->Killer))((void *)(pNode->Data)); /* this line can occasion a warning */
          else
	  {
              /* if (pNode->Data)
	       *   SdifFree(pNode->Data);
	       *else
	       *   fprintf(stderr, "HashTable->Data kill memory error : \n");
	       */
	      /* consider that the object pNode->Data cannot be kill because static
	       */
	  }
	  HTable->Table[i] = pNode->Next;
	  SdifFree(pNode);
      }
  }
}


void
SdifKillHashTable(SdifHashTableT* HTable)
{
  if (HTable)
    {
      SdifMakeEmptyHashTable(HTable);
      SdifFree(HTable->Table);
      SdifFree(HTable);
    }
  else
    {
      _SdifError(eFreeNull, "HashTable free");
    }
}







/******************  eHashChar ****************/
/* nchar represente le nombre de caracteres significatifs pour Hasher */
unsigned int
SdifHashChar(const char* s, unsigned int nchar, unsigned int HashSize)
{
  unsigned int valHash;
  unsigned int i;

  valHash = 0;
  for(i=0; i<nchar; i++)
    valHash = (valHash*256 + ((unsigned) *s++)) % HashSize;
  return valHash;
}

  





void*
SdifHashTableSearchChar(SdifHashTableT* HTable, const char *s, unsigned int nchar)
{
  SdifHashNT *pNode;
  
  
  for (pNode = HTable->Table[SdifHashChar(s, nchar, HTable->HashSize)]; pNode!=NULL; pNode = pNode->Next)
    {
      if (strncmp((char*) s, (char*) pNode->Index.Char[0], nchar)== 0)
	    {
         return pNode->Data;     /* trouve */
        }
    }

  return NULL; /* pas trouve */
}






SdifHashTableT*
SdifHashTablePutChar(SdifHashTableT* HTable,
		     const char *s,
		     unsigned int nchar,
		     void* Data)
{
  SdifHashNT *pNode;
  unsigned int valHash;

  if ( ! SdifHashTableSearchChar(HTable, s, nchar))
    {
      pNode = SdifMalloc(SdifHashNT);
      if (pNode)
        {
	      pNode->Index.Char[0] = SdifCalloc(char, nchar);
          if (pNode->Index.Char[0])
	        strncpy((char*) pNode->Index.Char[0], (char*) s, nchar);
	      else
	        return NULL; /* erreur memoire */
	      valHash = SdifHashChar(s, nchar, HTable->HashSize);
	      pNode->Data = Data; /* Attention : copie de pointeur */
	      HTable->NbOfData++;
	      pNode->Next = HTable->Table[valHash];
	      HTable->Table[valHash] = pNode;
	    }
    }
  return HTable;
}








/***************** eHashInt4 **********************/

#if 0
unsigned int
SdifHashInt4(unsigned int i, unsigned int HashSize)
{
  return i % HashSize;
}
#else
#   define SdifHashInt4(i, n)	((i) % (n))
#endif


void*
SdifHashTableSearchInt4(SdifHashTableT* HTable, unsigned int i)
{
  SdifHashNT *pNode;
  
  for (pNode = HTable->Table[SdifHashInt4(i, HTable->HashSize)]; pNode!=NULL; pNode = pNode->Next)
    if (pNode->Index.Int4 == i)
      return pNode->Data;     /* trouve */
  return NULL; /* pas trouve */
}





SdifHashTableT*
SdifHashTablePutInt4(SdifHashTableT* HTable, const unsigned int i, void* Data)
{
  SdifHashNT *pNode;
  SdifHashNT *CurrNode;
  unsigned int valHash;
  
  if ( ! SdifHashTableSearchInt4(HTable, i))
    {
      pNode = SdifMalloc(SdifHashNT);
      if (pNode)
	    {
	      pNode->Index.Int4 = i;
	      valHash = SdifHashInt4(i, HTable->HashSize);
	      pNode->Data = Data; /* Attention : copy de pointeur */
	      HTable->NbOfData++;
	  
          if (! HTable->Table[valHash])
            {
	          pNode->Next = NULL;
	          HTable->Table[valHash] = pNode;
	        }
	      else
	        {
	          if (pNode->Index.Int4 < HTable->Table[valHash]->Index.Int4)
		        {
		          pNode->Next = HTable->Table[valHash];
		          HTable->Table[valHash] = pNode;
		        }
	          else
		        {
		          for(CurrNode = HTable->Table[valHash]; CurrNode->Next; CurrNode = CurrNode->Next)
		            if (pNode->Index.Int4 < CurrNode->Next->Index.Int4)
		              break;
		  
		          pNode->Next = CurrNode->Next;
		          CurrNode->Next = pNode;
		        }
	        }
	    }
    }
  return HTable;
}






/*************** for all ******************/



void*
SdifHashTableSearch(SdifHashTableT* HTable, void *ptr, unsigned int nobj)
{
  switch (HTable->IndexType)
    {
    case eHashChar :
      return SdifHashTableSearchChar(HTable, (const char *) ptr, nobj);
    case eHashInt4 :
      return SdifHashTableSearchInt4(HTable, *((unsigned int*) ptr));
    default :
      return NULL;
    }
}






SdifHashTableT*
SdifHashTablePut(SdifHashTableT* HTable, const void *ptr, unsigned int nobj, void* Data)
{
  switch (HTable->IndexType)
    {
    case eHashChar :
      return SdifHashTablePutChar(HTable, (const char*)ptr, nobj, Data);
    case eHashInt4 :
      return SdifHashTablePutInt4(HTable, *((const unsigned int*)ptr), Data);
    default :
      return NULL;
    }
}


/*********** Iteration over all hash table elements ***************/

/* Allocate and initialise hash table iterator from given hash table or NULL,
   return pointer to it */
SdifHashTableIteratorT* SdifCreateHashTableIterator (SdifHashTableT *HTable)
{
    SdifHashTableIteratorT *iter = SdifMalloc(SdifHashTableIteratorT);
    SdifHashTableIteratorInitLoop(iter, HTable);
    return iter;
}

/* Deallocate hash table iterator created with SdifCreateHashTableIterator */
void SdifKillHashTableIterator (SdifHashTableIteratorT *iter)
{
    SdifFree(iter);
}

/* Initialise hash table iterator given by pointer (or NULL) */
int  SdifHashTableIteratorInitLoop (SdifHashTableIteratorT *iter, 
				    SdifHashTableT *hash)
{
    iter->HTable   = hash;
    iter->BinIndex = 0;
    iter->Entry    = NULL;

    if (hash)
    {
	while (iter->BinIndex < hash->HashSize)
	{
	    iter->Entry = hash->Table[iter->BinIndex];
	    if (iter->Entry != NULL)
		break;

	    iter->BinIndex++;
	}
    }
    return iter->Entry != NULL;
}

/* Test if iterator has more elements */
int  SdifHashTableIteratorIsNext (SdifHashTableIteratorT *iter)
{
    return iter->Entry != NULL;
}

/* Return current Data pointer and advance iterator */
void* SdifHashTableIteratorGetNext (SdifHashTableIteratorT *iter)
{
    void *curr = iter->Entry->Data;

    iter->Entry = iter->Entry->Next;

    if (iter->Entry == NULL)
    {
	/* end of this bin list, search next bin with non-empty list */
	while (++(iter->BinIndex) < iter->HTable->HashSize  &&
	       (iter->Entry = iter->HTable->Table[iter->BinIndex]) == NULL)
	    ;
    }
    return curr;
}

