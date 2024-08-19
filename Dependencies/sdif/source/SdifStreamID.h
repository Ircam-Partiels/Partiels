/* $Id: SdifStreamID.h,v 3.8 2001-05-02 09:34:48 tisseran Exp $
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
 * SdifStreamID.h
 *
 * StreamID structures management
 *
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.7  2000/11/21 14:51:51  schwarz
 * - sdif.h is now included by all sdif/Sdif*.c files.
 * - Removed all public typedefs, enums, structs, and defines from the
 *   individual sdif/Sdif*.h files, because they were duplicated in sdif.h.
 * - Todo: Do the same for the function prototypes, decide which types and
 *   prototypes really need to be exported.
 * - Removed SdifFileStruct.h.
 * - Preliminary new version of SdiffGetPos, SdiffSetPos.  They used the
 *   type fpos_t, which is no longer a long on RedHat 7 Linux.
 *
 * Revision 3.6  2000/10/27 20:03:44  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.5.2.1  2000/08/21  21:35:51  tisseran
 * *** empty log message ***
 *
 * Revision 3.5  2000/05/22  15:23:18  schwarz
 * Added functions to retrieve and inspect the stream ID table of a file.
 *
 * Revision 3.4  1999/10/15  12:26:56  schwarz
 * No time parameter for name value tables and stream ID tables, since
 * this decision is better left to the library.  (It uses the _SdifNoTime
 * constant, which happens to be _Sdif_MIN_DOUBLE_.)
 *
 * Revision 3.3  1999/09/28  13:09:14  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.2  1999/08/25  18:32:37  schwarz
 * Added cocoon-able comments with sentinel "DOC:" (on a single line).
 *
 * Revision 3.1  1999/03/14  10:57:22  virolle
 * SdifStdErr add
 *
 * Revision 2.3  1999/01/23  15:56:01  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:47  virolle
 * General Lists, and special chunk preparation to become frames
 *
 * Revision 2.1  1998/12/21  18:27:40  schwarz
 * Inserted copyright message.
 *
 * Revision 2.0  1998/11/29  11:42:07  virolle
 * - New management of interpretation errors.
 * - Alignement of frames with CNMAT (execpt specials Chunk 1NVT, 1TYP, 1IDS).
 * _ Sdif Header File has a Sdif format version.
 * - Matrices order in frames is not important now. (only one occurence of
 *   a Matrix Type in a Frame Type declaration )
 * - Hard coded predefined types more dynamic management.
 * - Standart streams (stdin, stdout, stderr) set as binary for Windows32 to
 *   have exactly the same result on each plateforme.
 *
 * Revision 1.4  1998/11/10  15:31:55  schwarz
 * Removed all 'extern' keywords for prototypes, since this is redundant
 * (function prototypes are automatically linked extern), and it
 * prohibits cocoon from generating an entry in the HTML documentation
 * for this function.
 *
 */


#ifndef _SdifStreamID_
#define _SdifStreamID_

#include "SdifGlobals.h"
#include "SdifHash.h"



/*
// FUNCTION GROUP:	Stream ID Tables for 1IDS ASCII chunk
*/

/*DOC: 
  Permet de créer un pointeur sur un objet de type StreamIDT.  

<p>Exemple dans le cas d'un TreeWay pour chant (non fichier):

<pre>
void ConsOneStreamID(SdifFileT *SdifF,
		     int        NumID,
		     char      *PatchType,
		     int        NumPatch,
		     char      *ObjType,
		     int        NumObj,
		     int        NbSubObj,
		     float      StartTime,
		     float      EndTime)
{
  SdifStreamIDT* StreamID;
  char TreeWay[512];

  sprintf(TreeWay, "%s/%d/%s/%d/%d/%s/%f", PatchType, NumPatch, ObjType,
		    NumObj, NbSubObj, StartTime, EndTime);
  StreamID = SdifCreateStreamID(NumID, "Chant", TreeWay);

  SdifHashTablePut(SdifF->StreamIDsTable, &(StreamID->NumID), 1, StreamID);
}
</pre>

Pour recuperer un StreamID il faut utiliser la fonction SdifHashTableGet
<pre>
  SdifStreamIDT *StreamID = (SdifStreamIDT*) SdifHashTableGet (SdifF->StreamIDsTable, &NumID, 0);
</pre>
Le troisième argument n'est pas utilisé, car la table est indexée directement
par des entiers (création de la table avec l'option eInt4). 
*/
SdifStreamIDT* SdifCreateStreamID(SdifUInt4 NumID, char *Source, char *TreeWay);
void           SdifKillStreamID(SdifStreamIDT *StreamID);


/*DOC:
  Create a stream ID table.  <strong>The stream ID table of the SDIF
  file structure is created automatically by SdifFOpen().</strong> 
  It can be obtained by SdifFStreamIDTable(). */
SdifStreamIDTableT* SdifCreateStreamIDTable     (SdifUInt4 HashSize);

/*DOC:
  Deallocate a stream ID table.  <strong>The stream ID table of the SDIF
  file structure is killed automatically by SdifFClose.</strong>  
  It can be obtained by SdifFStreamIDTable. */
void                SdifKillStreamIDTable       (SdifStreamIDTableT *SIDTable);

/*DOC:
  Add an entry to a stream ID table.  The table will be written by
  SdifFWriteAllASCIIChunks.
  [in]  SIDTable pointer to stream ID table, e.g. obtained by SdifFStreamIDTable
  [in]	NumID	stream ID of the frames the stream ID table describes
  [in]	Source	Source identifier for the table (ex. "Chant")
  [in]	TreeWay	Routing and parameters, separated by slashes
  [return]
		The stream ID table entry just created and added */
SdifStreamIDT*      SdifStreamIDTablePutSID     (SdifStreamIDTableT *SIDTable,
						 SdifUInt4	     NumID, 
						 char 		    *Source, 
						 char 		    *TreeWay);

/*DOC:
  Retrieve an entry to a stream ID table.  The table has to have been
  read by SdifFReadAllASCIIChunks.

  [in]  SIDTable pointer to stream ID table, e.g. obtained by 
		 SdifFStreamIDTable
  [in]	NumID	 stream ID of the frames the stream ID table describes
  [return]
		 pointer to stream ID table entry, or NULL if no entry for 
		 stream ID NumID exists. */
SdifStreamIDT*      SdifStreamIDTableGetSID     (SdifStreamIDTableT *SIDTable, 
						 SdifUInt4	     NumID);

/*DOC:
  Return number of entries in stream ID table SIDTable */
SdifUInt4           SdifStreamIDTableGetNbData  (SdifStreamIDTableT *SIDTable);


/*DOC:
  Return stream ID field in stream ID table entry SID */
SdifUInt4	    SdifStreamIDEntryGetSID	(SdifStreamIDT *SID);

/*DOC:
  Return source field in stream ID table entry SID */
char		   *SdifStreamIDEntryGetSource	(SdifStreamIDT *SID);

/*DOC:
  Return "treeway" field in stream ID table entry SID */
char		   *SdifStreamIDEntryGetTreeWay	(SdifStreamIDT *SID);


#endif /* _SdifStreamID_ */
