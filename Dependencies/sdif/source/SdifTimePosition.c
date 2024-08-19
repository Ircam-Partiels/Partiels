/* $Id: SdifTimePosition.c,v 3.10 2011-04-15 23:21:31 roebel Exp $
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
 *
 *  NOT INCLUDE IN THE SDIF PROJECT AT NOV 1997
 * author: Dominique Virolle 1997
 *
 *
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.9  2005/04/07 15:56:48  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.8  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.7  2001/05/02 09:34:48  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.6  2000/11/21 14:51:51  schwarz
 * - sdif.h is now included by all sdif/Sdif*.c files.
 * - Removed all public typedefs, enums, structs, and defines from the
 *   individual sdif/Sdif*.h files, because they were duplicated in sdif.h.
 * - Todo: Do the same for the function prototypes, decide which types and
 *   prototypes really need to be exported.
 * - Removed SdifFileStruct.h.
 * - Preliminary new version of SdiffGetPos, SdiffSetPos.  They used the
 *   type fpos_t, which is no longer a long on RedHat 7 Linux.
 *
 * Revision 3.5  2000/11/15 14:53:37  lefevre
 * no message
 *
 * Revision 3.4  2000/10/27  20:03:46  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.3.2.1  2000/08/21  21:35:57  tisseran
 * *** empty log message ***
 *
 * Revision 3.3  2000/05/15  16:22:36  schwarz
 * Avoid warning about KillerFT function pointer type (ANSI prototype given).
 * Argument to kill func is now void *.
 *
 * Revision 3.2  1999/09/28  13:09:17  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.1  1999/03/14  10:57:27  virolle
 * SdifStdErr add
 *
 * Revision 2.4  1999/02/28  12:16:57  virolle
 * memory report
 *
 * Revision 2.3  1999/01/23  15:56:03  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:49  virolle
 * General Lists, and special chunk preparation to become frames
 */


#include <stdlib.h>
#include "sdif_portability.h"
#include "sdif.h"
#include "SdifErrMess.h"


SdifTimePositionT*
SdifCreateTimePosition(SdifFloat8 Time, SdiffPosT Position)
{
  SdifTimePositionT* NewTimePosition = NULL;
  
  NewTimePosition = SdifMalloc(SdifTimePositionT);
  if (NewTimePosition)
    {
      NewTimePosition->Time = Time;
      NewTimePosition->Position = Position;

      return NewTimePosition ;
    }
  else
    {
      _SdifError(eAllocFail, "Create TimePosition");
      return NULL;
    }
}











void
SdifKillTimePosition(void* TimePosition)
{
  if (TimePosition)
    SdifFree (TimePosition);
  else
    _SdifError(eFreeNull, "Kill TimePosition");
}







SdifTimePositionLT*
SdifCreateTimePositionL(void)
{
  SdifTimePositionLT *NewTimePositionL = NULL;
  
  NewTimePositionL = SdifMalloc(SdifTimePositionLT);
  if (NewTimePositionL)
    {
      NewTimePositionL->TimePosList = SdifCreateList(SdifKillTimePosition);
      return NewTimePositionL;
    }
  else
    {
      _SdifError(eAllocFail, "TimePositionL allocation");
      return NULL;
    }
}






void
SdifKillTimePositionL(SdifTimePositionLT *TimePositionL)
{
  if (TimePositionL)
    {
      SdifKillList(TimePositionL->TimePosList);
      SdifFree(TimePositionL);
    }
  else
    _SdifError(eFreeNull, "TimePositionL free");
}









SdifTimePositionLT*
SdifTimePositionLPutTail(SdifTimePositionLT* TimePositionL, SdifFloat8 Time, SdiffPosT Position)
{
    SdifTimePositionT* NewTP;

    NewTP = SdifCreateTimePosition(Time, Position);
    SdifListPutTail(TimePositionL->TimePosList, NewTP);
    return TimePositionL;
}


SdifTimePositionT*
SdifTimePositionLGetTail(SdifTimePositionLT* TimePositionL)
{
    return (SdifTimePositionT*) SdifListGetTail(TimePositionL->TimePosList);
}
