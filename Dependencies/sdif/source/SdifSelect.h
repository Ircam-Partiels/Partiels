/* $Id: SdifSelect.h,v 3.17 2005-04-07 15:20:22 schwarz Exp $
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
 * SdifSelect.c		27. August 1999		Diemo Schwarz
 */

/*

DESCRIPTION

  Standardize access path to a selection of data from an SDIF file.

  I repeat my suggestion for the syntax of the select specification ,
  which I sent to Matt on June 21:
  
  I think it's a good idea to include the filename in the selection.
  This would encourage programmers to use some parsing procedure
  SdifGetFilenameAndSelection, which splits a command-line argument up
  into the filename and the selection, thus transparently allowing SDIF
  readers to handle a specific part of an SDIF file only.
  
  For various problems with filename and shell metacharacters, I suggest
  to mark the start of the select spec with '::' (or '##' or '--').
  This way, there is no ambiguity with filenames containing the
  select-spec characters, and to specify a filename containing '::'
  itself, you just have to append '::'.
  
  The syntax for the select-spec would then be:
  
  [filename][::[[#stream]:][frame][/matrix][.column][_row][@time]]
  
  With these mnemonics:
  - #stream as with "number", 
  - frame/matrix as in a file system hierarchy,
  - .column as in a C-struct (with column name or number) 
    Watch out: "@1.2" will be parsed as time 1.2 seconds, not time 1 column 2!
    (use ".2@1" for that)
  - _row as in a LaTeX index.  
  (As .column_row may seem unnatural for people with a maths background
  these can also be given in inverse order as _row.column, which still 
  looks good.)
  - @time like in "at time ..."
  - @time+delta can have delta < 0, so we have "at time plus/minus delta"
  
  - Streams and columns can be given either as a number or as a name.  
  
  - All specifications can be comma-separated lists of values.
  Numerical specifications can also be ranges l-u (a missing value l or u 
  would be replaced by the respective min/max).
  
  - Whitespace between the specs is allowed (before the :: it is
    considered part of the filename).

  - Again, watch out for possible ambiguities between floating point
    time spec and column spec: @1.2 will be time 1.2, not time 1,
    column 2.  It is advisable to put the time spec last to avoid this
    (.2@1 is ok) or use spaces (@1 .2 is also ok.
  
TODO
  - handle quoted strings:
  - Allow quoting names for weird frame and matrix types.
  - One could later allow regular expressions for names. 
  - functions to set up specs (lists of ranges)

LOG
  $Log: not supported by cvs2svn $
  Revision 3.16  2003/08/06 15:18:54  schwarz
  removed part of double defs (sdif.h)

  Revision 3.15  2002/09/20 14:43:03  schwarz
  - SdifParseSignatureList Parse comma-separated list of signatures
  - SdifKillSelectElement  now public

  Revision 3.14  2002/05/24 19:37:28  ftissera
  Change macro to be compatible with C++

  Revision 3.13  2001/05/02 09:34:47  tisseran
  Change License from GNU Public License to GNU Lesser Public License.

  Revision 3.12  2000/11/21 14:51:50  schwarz
  - sdif.h is now included by all sdif/Sdif*.c files.
  - Removed all public typedefs, enums, structs, and defines from the
    individual sdif/Sdif*.h files, because they were duplicated in sdif.h.
  - Todo: Do the same for the function prototypes, decide which types and
    prototypes really need to be exported.
  - Removed SdifFileStruct.h.
  - Preliminary new version of SdiffGetPos, SdiffSetPos.  They used the
    type fpos_t, which is no longer a long on RedHat 7 Linux.

  Revision 3.11  2000/11/15 14:53:35  lefevre
  no message

  Revision 3.10  2000/10/27  20:03:43  roebel
  autoconf merged back to main trunk

  Revision 3.9.2.2  2000/08/21  21:35:48  tisseran
  *** empty log message ***

  Revision 3.9.2.1  2000/08/21  14:04:23  tisseran
  *** empty log message ***

  Revision 3.8  2000/05/15  16:22:04  schwarz
  Changed prototypes of existing functions (we apologize for the inconvenience)
  SdifFCurrFrameIsSelected and SdifFCurrMatrixIsSelected.
  They now take only an SdifFileT as parameter, no longer a selection.
  If you don't want to test the file selection (automatically
  parsed from the filename), use these new functions like this:
  SdifFrameIsSelected  (file->CurrFramH, myselection) and
  SdifMatrixIsSelected (file->CurrMtrxH, myselection).

  Revision 3.7  2000/05/12  16:55:31  schwarz
  Added prototype and doc for SdifBaseName.
  Avoid avoidable warnings.

  Revision 3.6  2000/03/01  11:18:46  schwarz
  Added SdifCreateSelection.
  Fixed Linux-only bug in SdifGetFilenameAndSelection (high addresses).
  SdifSelectTestReal first tests from current select element onwards.

  Revision 3.5  1999/10/13  16:05:58  schwarz
  Changed data type codes (SdifDataTypeET) to SDIF format version 3, as
  decided with Matt Wright June 1999, added integer data types.
  Added writing of 1NVT with real frame header (but data is still not in
  matrices).
  The data type handling makes heavy use of code-generating macros,
  called for all data types with the sdif_foralltypes macro, thus
  adding new data types is easy.

  Revision 3.4  1999/10/07  15:06:42  schwarz
  Added SdifSelectGetFirst<type>, SdifSelectGetNext(Int|Real).

  Revision 3.3  1999/09/28  13:09:12  schwarz
  Included #include <preincluded.h> for cross-platform uniformisation,
  which in turn includes host_architecture.h and SDIF's project_preinclude.h.

  Revision 3.2  1999/09/20  13:23:03  schwarz
  First finished version, API to be improved.

  Revision 3.1  1999/08/31  10:03:00  schwarz
  Added module SdifSelect which parses an access specification to a
  chosen part of SDIF data.  Can be added to a file name.  */


#ifndef _SdifSelect_
#define _SdifSelect_


#include "SdifFrame.h"
#include "SdifMatrix.h"
#include "SdifList.h"


/*DOC:
  Return pointer to start of filename component in path inPathFileName.
 */
char *SdifBaseName (const char* inPathFileName);


/* 
// FUNCTION GROUP:	Selection Init/Deinit
 */

/* init module, called by SdifGenInit */
int SdifInitSelect (void);

/*DOC: 
  Allocate space for an sdif selection.
*/
SdifSelectionT *SdifCreateSelection (void);

/*DOC: 
*/
int SdifInitSelection (SdifSelectionT *sel, const char *filename, int namelen);

/*DOC: 
*/
int SdifFreeSelection (SdifSelectionT *sel);

/*
// FUNCTION GROUP:	Parse and Set Selection
*/


/*DOC: 
  Returns pointer to first char of select spec (starting with ::), 
  or NULL if not found.
*/
char *SdifSelectFindSelection (const char *filename);


/*DOC: 
  Returns pointer to a copy of only the filename part of the input filename.
  Writes parsed selection to sel.
*/
char *SdifGetFilenameAndSelection (/*in*/  const char *filename, 
				   /*out*/ SdifSelectionT *sel);

/*DOC: 
*/
void SdifPrintSelection (FILE *out, SdifSelectionT *sel, int options);



/*
// FUNCTION GROUP:	Query parsed ranges (list of ranges).
*/

/*DOC:
  Query parsed ranges (list of ranges) for a selection element (one of
  the SdifListP lists in SdifSelectionT).  Init list traversal with
  SdifListInitLoop, then call SdifSelectGetNext<type>(list) until it
  returns 0.

  The number of selections in the list is SdifListGetNbData(list), if
  it is 0, or SdifListIsEmpty(list) is true, then there was no
  selection for that element.

  If force_range is 1, the out value is converted to a range in any
  case, with value <= range guaranteed.  
*/
int SdifSelectGetNextIntRange  (/*in*/  SdifListP list, 
				/*out*/ SdifSelectElementIntT  *range, 
				/*in*/  int force_range);

/*DOC: 
  See SdifSelectGetNextInt.
*/
int SdifSelectGetNextRealRange (/*in*/  SdifListP list, 
				/*out*/ SdifSelectElementRealT *range, 
				/*in*/  int force_range);

/*DOC: 
  Query list of parsed selection elements (one of the SdifListP
  lists in SdifSelectionT).  Init list traversal with
  SdifListInitLoop, then call SdifSelectGetNext<type>(list) until it
  returns 0.

  See also SdifSelectGetNextInt.  
*/
SdifSignature  SdifSelectGetNextSignature (/*in*/  SdifListP list);

/*DOC: 
  See SdifSelectGetNextSignature.
*/
char	      *SdifSelectGetNextString    (/*in*/  SdifListP list);


/*DOC: 
  Return value of first selection (ignoring range).
*/
int	       SdifSelectGetFirstInt       (SdifListP l, int defval);
double	       SdifSelectGetFirstReal      (SdifListP l, double defval);
char	      *SdifSelectGetFirstString    (SdifListP l, char *defval);
SdifSignature  SdifSelectGetFirstSignature (SdifListP l, SdifSignature defval);

#endif /* _SdifSelect_ */
