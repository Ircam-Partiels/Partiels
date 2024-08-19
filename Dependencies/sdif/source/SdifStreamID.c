/* $Id: SdifStreamID.c,v 3.9 2003-11-07 21:47:18 roebel Exp $
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
 * StreamID structures management
 *
 * author: Dominique Virolle 1997
 *
 *
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.8  2002/05/24 19:37:52  ftissera
 * Change code to be compatible with C++
 * Cast pointers to correct type.
 *
 * Revision 3.7  2001/05/02 09:34:48  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.6  2000/11/15 14:53:36  lefevre
 * no message
 *
 * Revision 3.5  2000/10/27  20:03:44  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.4.2.1  2000/08/21  21:35:50  tisseran
 * *** empty log message ***
 *
 * Revision 3.4  2000/05/22  15:23:17  schwarz
 * Added functions to retrieve and inspect the stream ID table of a file.
 *
 * Revision 3.3  1999/10/15  12:26:55  schwarz
 * No time parameter for name value tables and stream ID tables, since
 * this decision is better left to the library.  (It uses the _SdifNoTime
 * constant, which happens to be _Sdif_MIN_DOUBLE_.)
 *
 * Revision 3.2  1999/09/28  13:09:13  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.1  1999/03/14  10:57:21  virolle
 * SdifStdErr add
 *
 * Revision 2.4  1999/02/28  12:16:55  virolle
 * memory report
 *
 * Revision 2.3  1999/01/23  15:56:00  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:47  virolle
 * General Lists, and special chunk preparation to become frames
 *
 *
 *
 *
 */



#include "sdif_portability.h"

#include "SdifStreamID.h"
#include <stdlib.h>



SdifStreamIDT*
SdifCreateStreamID(SdifUInt4 NumID, char *Source, char *TreeWay)
{
  SdifStreamIDT *NewID = NULL;
  
  NewID =  SdifMalloc(SdifStreamIDT);
  if (NewID)
    {
      NewID->NumID = NumID;
      NewID->Source  = SdifCreateStrNCpy(Source, SdifStrLen(Source)+1);
      NewID->TreeWay = SdifCreateStrNCpy(TreeWay, SdifStrLen(TreeWay)+1);
      
      return NewID;
    }
  else
    {
      _SdifError(eAllocFail, "StreamID allocation");
      return NULL;
    }
}






void
SdifKillStreamID(SdifStreamIDT *StreamID)
{
  if (StreamID)
    {
      SdifKillStr(StreamID->TreeWay);
      SdifKillStr(StreamID->Source);
      SdifFree(StreamID);
    }
  else
    _SdifError(eFreeNull, "StreamID free");
}








SdifStreamIDTableT*
SdifCreateStreamIDTable(SdifUInt4 HashSize)
{
    SdifStreamIDTableT* NewSIDTable;

    NewSIDTable = SdifMalloc(SdifStreamIDTableT);
    if (NewSIDTable)
    {
        NewSIDTable->StreamID   = _SdifNoStreamID;
        NewSIDTable->SIDHT      = SdifCreateHashTable(HashSize, eHashInt4, 
						      (void(*)(void *))
						      SdifKillStreamID);;
        return NewSIDTable;
    }
    else
    {
      _SdifError(eAllocFail, "StreamIDTable allocation");
      return NULL;
    }
}





void
SdifKillStreamIDTable (SdifStreamIDTableT* SIDTable)
{
    if (SIDTable)
    {
      SdifKillHashTable(SIDTable->SIDHT);
      SdifFree(SIDTable);
    }
    else
        _SdifError(eFreeNull, "StreamIDTable free");
}



SdifStreamIDT*
SdifStreamIDTablePutSID(SdifStreamIDTableT* SIDTable, SdifUInt4 NumID, char *Source, char *TreeWay)
{
    SdifStreamIDT* StreamID;

    StreamID = SdifCreateStreamID(NumID, Source, TreeWay);
    if (StreamID)
        SdifHashTablePut(SIDTable->SIDHT, &(StreamID->NumID), 1, StreamID);

    return StreamID;
}




SdifStreamIDT*
SdifStreamIDTableGetSID(SdifStreamIDTableT* SIDTable, SdifUInt4 NumID)
{
    return (SdifStreamIDT*)SdifHashTableSearch(SIDTable->SIDHT, &(NumID), 1);
}



SdifUInt4
SdifStreamIDTableGetNbData(SdifStreamIDTableT* SIDTable)
{
    return SIDTable->SIDHT->NbOfData;
}



SdifUInt4
SdifStreamIDEntryGetSID	(SdifStreamIDT *SID)
{
    return SID->NumID;
}

char *
SdifStreamIDEntryGetSource (SdifStreamIDT *SID)
{
    return SID->Source;
}

char *
SdifStreamIDEntryGetTreeWay (SdifStreamIDT *SID)
{
    return SID->TreeWay;
}
