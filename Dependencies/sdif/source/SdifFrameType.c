/* $Id: SdifFrameType.c,v 3.11 2008-12-18 11:42:19 diemo Exp $
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
 * Frame Types management (interpreted sdif frame types)
 *
 * SdifHashTableT to stock Frame Types
 *         index : frame type name (HashChar, use _SdifNameLen for size)
 *          Data : Frame type (SdifFrameTypeT*)
 *        Killer : SdifKillFrameType
 *
 * Frame Components are into a hash table too
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.10  2007/12/10 10:46:42  roebel
 * Use const char* for read only function arguments.
 *
 * Revision 3.9  2005/10/21 14:32:29  schwarz
 * protect all static buffers from overflow by using snprintf instead of sprintf
 * move big errorMess buffers into error branch to avoid too large stack allocation
 *
 * Revision 3.8  2004/07/22 14:47:56  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.7  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.6  2002/05/24 19:37:52  ftissera
 * Change code to be compatible with C++
 * Cast pointers to correct type.
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
 * Revision 3.2.2.1  2000/08/21  21:35:21  tisseran
 * *** empty log message ***
 *
 * Revision 3.2  1999/09/28  13:08:59  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.1  1999/03/14  10:56:55  virolle
 * SdifStdErr add
 *
 *
 */



#include "sdif_portability.h"

#include "SdifFrameType.h"
#include <stdlib.h>
#include "SdifMatrixType.h"





SdifComponentT*
SdifCreateComponent(SdifSignature MtrxS, char *Name, SdifUInt4 Num)
{
  SdifComponentT *NewComponent = NULL;

  NewComponent = SdifMalloc(SdifComponentT);
  if (NewComponent)
    {
      NewComponent->MtrxS = MtrxS;
      NewComponent->Name = SdifCreateStrNCpy(Name, SdifStrLen(Name)+1);
      NewComponent->Num = Num;
      
      return NewComponent;
    }
  else
    {
      _SdifError(eAllocFail, "Component allocation");
      return NULL;
    }
}







void
SdifKillComponent(SdifComponentT *Component)
{
  if (Component)
    {
      SdifKillStr(Component->Name);
      SdifFree(Component);
    }
  else
    _SdifError(eFreeNull, "Component free");
}







SdifFrameTypeT*
SdifCreateFrameType(SdifSignature FramS, SdifFrameTypeT *PredefinedFrameType)
{
  SdifFrameTypeT *NewFrameType = NULL;
  
  NewFrameType = SdifMalloc(SdifFrameTypeT);
  if (NewFrameType)
    {
      NewFrameType->Signature = FramS;

      NewFrameType->ComponentUseHT = SdifCreateHashTable(13, eHashInt4, (void (*) (void *))SdifKillComponent);
      NewFrameType->NbComponentUse   = 0;
      NewFrameType->ModifMode = eCanModif;

      if (PredefinedFrameType)
      {
	    if (PredefinedFrameType->Signature != FramS)
	    {
	      char errorMess[_SdifStringLen];	

	      snprintf(errorMess, sizeof(errorMess), "'%s'(Pre) != '%s'",
		      SdifSignatureToString(PredefinedFrameType->Signature),
		      SdifSignatureToString(FramS));
	      _SdifError(eInvalidPreType, errorMess);
	    }
	  else
	    {
	      NewFrameType->FrameTypePre = PredefinedFrameType;
	      NewFrameType->NbComponent  = PredefinedFrameType->NbComponent;
	    }
      }
      else
      {
        NewFrameType->FrameTypePre = NULL;
        NewFrameType->NbComponent  = 0;
      }
          
      return NewFrameType;
    }
  else
    {
      _SdifError(eAllocFail, "FrameType allocation");
      return NULL;
    }
}


void
SdifKillFrameType(SdifFrameTypeT *FrameType)
{
  if (FrameType)
    {
      SdifKillHashTable(FrameType->ComponentUseHT);
      SdifFree(FrameType);
    }
  else
    _SdifError(eFreeNull, "FrameType free");
}


SdifSignature 
SdifFrameTypeGetComponentSignature (SdifComponentT *comp)
{
    return comp->MtrxS;
}


char *
SdifFrameTypeGetComponentName (SdifComponentT *comp)
{
    return comp->Name;
}


SdifUInt4
SdifFrameTypeGetNbComponents (SdifFrameTypeT *FrameType)
{
    return FrameType->NbComponent;
}


SdifComponentT*
SdifFrameTypeGetComponent_MtrxS(SdifFrameTypeT *FrameType, SdifSignature MtrxS)
{
  SdifComponentT* Component;

  Component = NULL;
  if (FrameType->FrameTypePre)
    Component = SdifFrameTypeGetComponent_MtrxS(FrameType->FrameTypePre, MtrxS);

  if (! Component)
  {
    Component = (SdifComponentT*) SdifHashTableSearch(FrameType->ComponentUseHT,
                                                     &MtrxS,
                                                     1);
  }

  return Component;
}


SdifComponentT*
SdifFrameTypeGetComponent(SdifFrameTypeT *FrameType, const char *NameC)
{
  unsigned int     iHTN;
  SdifHashNT*      pHTN;
  SdifHashTableT*  ComponentUseHT = FrameType->ComponentUseHT;
  SdifComponentT*  Component;
  SdifComponentT*  ComponentTmp;

  Component = NULL;
  if (FrameType->FrameTypePre)
    Component = SdifFrameTypeGetComponent(FrameType->FrameTypePre, NameC);

  for(iHTN=0; iHTN<ComponentUseHT->HashSize && !Component; iHTN++)
    for (pHTN = ComponentUseHT->Table[iHTN]; pHTN && !Component;  pHTN=pHTN->Next)
    {
      ComponentTmp = (SdifComponentT*) pHTN->Data;
      if (SdifStrCmp (ComponentTmp->Name, NameC) == 0)
      {
        Component = ComponentTmp;
      }
    }

  return Component;
}




SdifComponentT*
SdifFrameTypeGetNthComponent(SdifFrameTypeT *FrameType, SdifUInt4 NumC)
{
  unsigned int     iHTN;
  SdifHashNT*      pHTN;
  SdifHashTableT*  ComponentUseHT = FrameType->ComponentUseHT;
  SdifComponentT*  Component;
  SdifComponentT*  ComponentTmp;

  Component = NULL;
  if (FrameType->FrameTypePre)
    Component = SdifFrameTypeGetNthComponent(FrameType->FrameTypePre, NumC);

  for(iHTN=0; iHTN<ComponentUseHT->HashSize && !Component; iHTN++)
    for (pHTN = ComponentUseHT->Table[iHTN]; pHTN && !Component;  pHTN=pHTN->Next)
    {
      ComponentTmp = (SdifComponentT*) pHTN->Data;
      if (ComponentTmp->Num == NumC)
      {
        Component = ComponentTmp;
      }
    }

  return Component;
}



SdifFrameTypeT*
SdifFrameTypePutComponent(SdifFrameTypeT *FrameType,
                          SdifSignature MtrxS,
                          char *NameC)
{
  SdifComponentT *NewComponent = NULL;



  if (SdifFrameTypeGetComponent_MtrxS(FrameType, MtrxS))
    {
      _SdifError(eExistYet, SdifSignatureToString(MtrxS));
      return NULL;
    }
  else
    if (SdifFrameTypeGetComponent(FrameType, NameC))
      {
        _SdifError(eExistYet, NameC);
        return NULL;
      }
    else
    
  switch (FrameType->ModifMode)
    {
    case eNoModif:
      _SdifError(eNoModifErr, SdifSignatureToString(FrameType->Signature));
      return NULL;
      
    case eCanModif:
      NewComponent = SdifCreateComponent(MtrxS, NameC, FrameType->NbComponent+1);
      SdifHashTablePut(FrameType->ComponentUseHT, &MtrxS, 1, NewComponent);
      
      FrameType->NbComponentUse++;
      FrameType->NbComponent++;
      return FrameType;
      
    default :
      return NULL;
    }
}






SdifFrameTypeT*
SdifGetFrameType(SdifHashTableT *FrameTypeHT, SdifSignature FramS)
{
  return (SdifFrameTypeT*) SdifHashTableSearch(FrameTypeHT, &FramS, 1);
}




void
SdifPutFrameType(SdifHashTableT *FrameTypeHT, SdifFrameTypeT *FrameType)
{
  SdifHashTablePut(FrameTypeHT, &(FrameType->Signature), 1, FrameType);
}







SdifUInt2
SdifExistUserFrameType(SdifHashTableT *FrameTypeHT)
{
  unsigned int     iName;
  SdifHashNT*      pName;
  SdifFrameTypeT*  FramT;
  
  for(iName=0; iName<FrameTypeHT->HashSize; iName++)
    for (pName = FrameTypeHT->Table[iName]; pName;  pName=pName->Next)
      {
        FramT = (SdifFrameTypeT*) pName->Data;
	    if (FramT->NbComponentUse > 0)
	      return 1;
      }
  return 0;
}
