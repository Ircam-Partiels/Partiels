/* $Id: SdifSignatureTab.c,v 3.8 2011-04-15 23:21:31 roebel Exp $
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
 * SdifSignatureTab.c		4. May 2000		Diemo Schwarz
 * 
 * Handling of a Table of Signatures
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.7  2007/11/26 18:50:48  roebel
 * Changed data types to have less casts and
 * less compilation warnings with MSVC.
 * renamed some functions that are deprecated in MSVC.
 *
 * Revision 3.6  2005/04/07 15:56:48  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.5  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.4  2001/05/02 09:34:47  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.3  2000/11/15 14:53:35  lefevre
 * no message
 *
 * Revision 3.2  2000/10/27  20:03:43  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.1.2.1  2000/08/21  21:35:48  tisseran
 * *** empty log message ***
 *
 * Revision 3.1  2000/05/04  14:59:32  schwarz
 * Added modules SdifCheck and SdifSignatureTab, containing functions
 * formerly in SdifFile.
 */


#include "sdif_portability.h"

#include "sdif.h"
#include "SdifGlobals.h"


SdifSignatureTabT*
SdifCreateSignatureTab (const SdifUInt4 NbSignMax)
{
  SdifSignatureTabT* NewSignTab = NULL;
  SdifUInt4 iSign;

  NewSignTab = SdifMalloc(SdifSignatureTabT);
  if (NewSignTab)
    {
      NewSignTab->Tab = SdifCalloc(SdifSignature, NbSignMax);
      if (NewSignTab->Tab)
        {
          NewSignTab->NbSignMax = NbSignMax;
          for (iSign=0; iSign<NewSignTab->NbSignMax; iSign++)
            NewSignTab->Tab[iSign] = 0;
          NewSignTab->NbSign    = 0;
        }
      else
        {
          _SdifError(eAllocFail, "NewSignTab->Tab");
          return NULL;
        }
    }
  else
    {
      _SdifError(eAllocFail, "NewSignTab");
      return NULL;
    }

  return NewSignTab;
}



void
SdifKillSignatureTab (SdifSignatureTabT* SignTab)
{
  if (SignTab)
    {
      if (SignTab->Tab)
      {
	  SdifFree(SignTab->Tab);
      }
      SdifFree(SignTab);
    }
  else
    {
      _SdifError(eAllocFail, "NewSignTab");
    }
}



SdifSignatureTabT*
SdifReAllocSignatureTab (SdifSignatureTabT* SignTab,
			 const  SdifUInt4 NewNbSignMax)
{
    if (SignTab->NbSignMax  < NewNbSignMax)
    {
	SignTab->Tab = SdifRealloc(SignTab->Tab, SdifSignature, NewNbSignMax);
	if (SignTab->Tab )
	{
	    SignTab->NbSignMax = NewNbSignMax;
	}
	else
	{
	    _SdifError(eAllocFail, "SignTab->Tab RE-allocation");
	    return NULL;
	}
    }

    return SignTab;
}



SdifSignatureTabT*
SdifReInitSignatureTab (SdifSignatureTabT* SignTab, 
			const SdifUInt4 NewNbSignMax)
{
    SdifUInt4 iSign;

    SdifReAllocSignatureTab (SignTab, NewNbSignMax);

    for (iSign=0; iSign<NewNbSignMax; iSign++)
        SignTab->Tab[iSign] = 0;

    SignTab->NbSign  = 0;

    return SignTab;
}




SdifSignature
SdifIsInSignatureTab (const SdifSignatureTabT* SignTab, 
		      const SdifSignature Sign)
{
  SdifUInt4 iSign;

  for (iSign=0; iSign<SignTab->NbSign; iSign++)
    if (SignTab->Tab[iSign] == Sign)
      return Sign;

  return 0;
}



int
SdifFindInSignatureTab (const SdifSignatureTabT* SignTab, 
			const SdifSignature Sign)
{
  SdifUInt4 iSign;

  for (iSign=0; iSign<SignTab->NbSign; iSign++)
    if (SignTab->Tab[iSign] == Sign)
      return iSign;

  return -1;
}


SdifSignatureTabT*
SdifPutInSignatureTab (SdifSignatureTabT* SignTab, const SdifSignature Sign)
{
  SignTab->Tab[SignTab->NbSign] = Sign;
  SignTab->NbSign++;
  return SignTab;
}


SdifSignatureTabT*
SdifAddToSignatureTab (SdifSignatureTabT* SignTab, const SdifSignature Sign)
{
    if (SignTab->NbSign >= SignTab->NbSignMax)
    {
	SdifReAllocSignatureTab (SignTab, SignTab->NbSign + 
					  _SdifSignatureTabGranule);
    }
    return SdifPutInSignatureTab (SignTab, Sign);
}


SdifSignature
SdifGetFromSignatureTab (const SdifSignatureTabT* SignTab, const int index)
{
    if (index >= 0  &&  (SdifUInt4) index < SignTab->NbSign)
	return (SignTab->Tab[index]);
    else
	return eEmptySignature;
}
