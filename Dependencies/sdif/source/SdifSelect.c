/* $Id: SdifSelect.c,v 3.29 2009-10-29 23:30:20 roebel Exp $
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
  
  - Whitespace between the specs should be allowed.
  
  - Allow quoting names for weird frame and matrix types.
  
  - One could later allow regular expressions for names. 

  - Extension: Use !element to negate selection, e.g. !@0-1 selects
    all outside time range

TODO
  - int SdifSelectFlattenIntList (in list, out array, in nummax)
    write ordered expanded list of int selection 
    (ranges/deltas x-y expanded to x..y)
    return number of selection written to array, not more than nummax.
  - keep signature selections parallely as hash tables for fast lookup
  - optimize selecttestint/real: loop from current 
  - SdifSelectionT: array of select elements
      struct { SdifListP list; SdifSelectElementT minmax; } elem [eSelNum];
    indexed by
      enum   { eTime, eStream, eFrame, eMatrix, eColumn, eRow, eSelNum }
    to use in all API functions instead of SdifListP.

LOG
  $Log: not supported by cvs2svn $
  Revision 3.28  2009/04/17 16:50:44  diemo
  clarified syntax in code-generating macro

  Revision 3.27  2007/11/27 12:09:44  roebel
  Removed unused functions.

  Revision 3.26  2007/11/26 18:50:56  roebel
  Changed data types to have less casts and
  less compilation warnings with MSVC.
  renamed some functions that are deprecated in MSVC.

  Revision 3.25  2007/03/21 19:44:15  roebel
  Don't use global variables without initialization. These globals are treated differently
  on MacOSX and they are not allowed in dynamic libraries without extra flags.
  To simplify the situation I now initialized all global variables
  or make them static.

  Revision 3.24  2006/03/22 22:00:46  roebel
  cast pointer difference to int for printf arguments

  Revision 3.23  2005/10/21 14:32:30  schwarz
  protect all static buffers from overflow by using snprintf instead of sprintf
  move big errorMess buffers into error branch to avoid too large stack allocation

  Revision 3.22  2005/04/07 15:56:48  schwarz
  removed some now empty local include files,
  added include of <sdif.h> and "SdifGlobals.h"

  Revision 3.21  2004/09/09 17:48:51  schwarz
  SdifSelectAppendList function, avoids the double references SdifListConcat
  woulde create.

  Revision 3.20  2004/07/22 14:47:56  bogaards
  removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6

  Revision 3.19  2003/11/07 21:47:18  roebel
  removed XpGuiCalls.h and replaced preinclude.h  by local files

  Revision 3.18  2003/08/06 15:20:45  schwarz
  SdifSelectIntMask added for more efficient integer selections, new functions:
  - SdifInitIntMask, SdifGetIntMask
  - SdifSelectTestIntMask
  - SdifFNumStreamsSelected, SdifFNumRowsSelected, SdifFNumColumnsSelected
  - SdifFRowIsSelected, SdifFColumnIsSelected
  int value/range had to be changed to SdifUInt4 for this

  Revision 3.17  2002/09/20 14:43:03  schwarz
  - SdifParseSignatureList Parse comma-separated list of signatures
  - SdifKillSelectElement  now public

  Revision 3.16  2002/08/05 14:20:53  roebel
  Fixed compiler warning.
  Added support to replace a selection.

  Revision 3.15  2002/05/24 19:37:52  ftissera
  Change code to be compatible with C++
  Cast pointers to correct type.

  Revision 3.14  2001/05/02 09:34:47  tisseran
  Change License from GNU Public License to GNU Lesser Public License.

  Revision 3.13  2000/11/15 14:53:34  lefevre
  no message

  Revision 3.12  2000/10/27  20:03:42  roebel
  autoconf merged back to main trunk

  Revision 3.11.2.2  2000/08/21  21:35:46  tisseran
  *** empty log message ***

  Revision 3.11.2.1  2000/08/21  14:04:22  tisseran
  *** empty log message ***

  Revision 3.10  2000/06/15  15:24:10  schwarz
  Better column names for AR coefs.

  Revision 3.9  2000/05/15  16:22:03  schwarz
  Changed prototypes of existing functions (we apologize for the inconvenience)
  SdifFCurrFrameIsSelected and SdifFCurrMatrixIsSelected.
  They now take only an SdifFileT as parameter, no longer a selection.
  If you don't want to test the file selection (automatically
  parsed from the filename), use these new functions like this:
  SdifFrameIsSelected  (file->CurrFramH, myselection) and
  SdifMatrixIsSelected (file->CurrMtrxH, myselection).

  Revision 3.8  2000/05/12  16:55:30  schwarz
  Added prototype and doc for SdifBaseName.
  Avoid avoidable warnings.

  Revision 3.7  2000/05/12  14:38:13  schwarz
  Finally getting rid of basename, which always caused trouble:
  On some unix systems, libgen was needed, on Mac it didn't exist at all.
  We're replacing it with SdifBaseName.

  Revision 3.6  2000/03/01  11:18:45  schwarz
  Added SdifCreateSelection.
  Fixed Linux-only bug in SdifGetFilenameAndSelection (high addresses).
  SdifSelectTestReal first tests from current select element onwards.

  Revision 3.5  1999/10/15  12:21:48  schwarz
  Changed min/max to upper case.
  Test frame takes _SdifAllStreamID into account.

  Revision 3.4  1999/10/07  15:06:42  schwarz
  Added SdifSelectGetFirst<type>, SdifSelectGetNext(Int|Real).

  Revision 3.3  1999/09/28  13:09:11  schwarz
  Included #include <preincluded.h> for cross-platform uniformisation,
  which in turn includes host_architecture.h and SDIF's project_preinclude.h.

  Revision 3.2  1999/09/20  13:23:02  schwarz
  First finished version, API to be improved.

  Revision 3.1  1999/08/31  10:02:59  schwarz
  Added module SdifSelect which parses an access specification to a
  chosen part of SDIF data.  Can be added to a file name.  */



/* include */		 /* to get these definitions: */
#include "sdif_portability.h"

#include <stdio.h>
#include <string.h>	 /* string functions */
#include <math.h>	 /* fabs */
#include <assert.h>	 /* N.B. that assert() calls will vanish with NDEBUG */

#include <sdif.h>
#include "SdifFile.h"	 /* SdifFileT */
#include "SdifGlobals.h" /* eEmptySignature, MIN/MAX */
#include "SdifMatrixType.h"
#include "SdifList.h"	 /* List structs and functions */
#include "SdifSelect.h"


typedef struct SdifSelectContextS{
	const char *mInput;
	const char *mSymbol;
	const char *mOriginal;
	SdifSelectTokens mToken;
}SdifSelectContext;

#define INPUT 	context->mInput
#define SYMBOL 	context->mSymbol
#define ORIG 	context->mOriginal
#define TOKEN 	context->mToken

static int debug = 0;


char *SdifBaseName (const char* path)
{
	const char* retFileName = strrchr (path, HOST_DIRECTORY_DIVIDER);

	if (retFileName != NULL)
	    return ((char *) retFileName + 1);
	else
	    return ((char *) path);
}

void SdifInitIntMask (SdifSelectIntMaskP mask);


/*
// DATA GROUP:		terminal symbols and character classes for parsing
*/

/* terminal symbols (strings) for SdifSelectTokens, MUST follow that order!
   N.B.: When symbols have common prefixes (e.g. "::" and ":"),
   the longer symbol must come first. */
const char *SdifSelectSeparators   [sst_num + 1] = { "::", "#", ":", "/", ".", 
					       "_",  "@", ",", "-", "+", "??"};
int   SdifSelectSeparatorLen [sst_num + 1] = {0};
char  sep_first		     [sst_num + 1] = {0}; /* first chars of all separators */
const char *SdifSelectWhitespace = " \t\r\n";	    /* chars considered as space */
/* todo: build charmap for first and whitespace */


/* static prototypes (to be warning free) */
static SdifSelectTokens findtoken (SdifSelectContext *context); 
static void skipspace (SdifSelectContext *context);
static SdifSelectTokens parsenexttoken (SdifSelectContext *context);

static int parseint	  (SdifSelectValueT *valu,SdifSelectContext *context);
static int parsereal      (SdifSelectValueT *valu,SdifSelectContext *context);
static int parsestring	  (SdifSelectContext *context);
static int parsesig	  (SdifSelectValueT *valu,SdifSelectContext *context);
static int parsecol	  (SdifSelectValueT *valu,SdifSelectContext *context);
static int parse	  (int (*parseval) (SdifSelectValueT *valu, SdifSelectContext *context),
			   SdifListP list, int range_allowed, char *name,SdifSelectContext *context);
/*
static int getint (SdifSelectValueT val);
static double getreal (SdifSelectValueT val);
static SdifSignature getsignature (SdifSelectValueT val);
static char *getstring (SdifSelectValueT val);
*/



/* 
// FUNCTION GROUP:	Init/Deinit
*/

/* init module, called by SdifGenInit */
int SdifInitSelect (void)
{
    int i;

    for (i = 0; i < sst_num; i++)
    {
	SdifSelectSeparatorLen [i] = strlen (SdifSelectSeparators [i]);
	sep_first [i] = SdifSelectSeparators [i][0];
    }
    /* settings for sst_num */
    SdifSelectSeparatorLen [i] = 0;
    sep_first [i] = 0;
    return (1);
}



/* killer function for SdifKillList: free one SdifSelectElement */
void SdifKillSelectElement (/*SdifSelectionT*/ void *victim)
{
    SdifFree (victim);
}


/* copy a selection list source, appending to an existing one dest the
   same but freshly allocated elements from source */
SdifListT *SdifSelectAppendList (SdifListT *dest, SdifListT *source)
{
    SdifListInitLoop(source);

    while (SdifListIsNext(source))
    {   /* allocate new element */
	SdifSelectElementT *elem = SdifMalloc (SdifSelectElementT);

	/* copy from old one */
	*elem = *(SdifSelectElementT *) SdifListGetNext(source);

	SdifListPutTail(dest, elem);
    }

    return dest;
}


int SdifInitSelectionLists (SdifSelectionT *sel)
{

    sel->stream = SdifCreateList (SdifKillSelectElement);
    sel->frame  = SdifCreateList (SdifKillSelectElement);
    sel->matrix = SdifCreateList (SdifKillSelectElement);
    sel->column = SdifCreateList (SdifKillSelectElement);
    sel->row    = SdifCreateList (SdifKillSelectElement);
    sel->time   = SdifCreateList (SdifKillSelectElement);

    SdifInitIntMask(&sel->streammask);
    SdifInitIntMask(&sel->rowmask);
    SdifInitIntMask(&sel->colmask);

    return (1);
}

int SdifFreeSelectionLists (SdifSelectionT *sel)
{

    SdifKillList (sel->stream);
    SdifKillList (sel->frame);
    SdifKillList (sel->matrix);
    SdifKillList (sel->column);
    SdifKillList (sel->row);
    SdifKillList (sel->time);

    if (sel->streammask.mask)   SdifFree(sel->streammask.mask);
    if (sel->rowmask.mask)      SdifFree(sel->rowmask.mask);
    if (sel->colmask.mask)      SdifFree(sel->colmask.mask);

    return (1);
}

int SdifInitSelection (SdifSelectionT *sel, const char *filename, int namelen)
{
    assert (gSdifInitialised  &&  "SDIF library not initialised!");

    if (!filename)
	filename = "";
	
    /* copy and null-terminate filename */
    sel->filename = SdifCreateStrNCpy (filename, namelen + 1);
    sel->filename [namelen] = 0;
    sel->basename = SdifBaseName (sel->filename);

    SdifInitSelectionLists(sel);
    return (1);
}


SdifSelectionT *SdifCreateSelection ()
{
    return (SdifMalloc (SdifSelectionT));
}


int SdifFreeSelection (SdifSelectionT *sel)
{
    if (sel->filename)	
	SdifFree (sel->filename);
    else
	_SdifError (eFreeNull, "Selection->filename");

    SdifFreeSelectionLists (sel);

#if defined(DEBUG)  &&  DEBUG
    /* set ALL pointers to NULL */
    memset(sel, 0, sizeof(SdifSelectionT));
#endif
    SdifFree(sel);
    return (1);
}


void SdifInitIntMask (SdifSelectIntMaskP mask)
{
    mask->mask 	  = NULL;
    mask->num  	  = 0;
    mask->max  	  = 0;	/* include all */
    mask->openend = 1;
}


/* convert list of int selections to mask */
void SdifSelectGetIntMask (SdifListP list, SdifSelectIntMaskP mask)
{
    SdifSelectElementIntT range;
    SdifUInt4 num = 0;
    SdifUInt4 max = 0;
    SdifUInt4 i   = 0;

    if (SdifListIsEmpty(list))
    {
	SdifInitIntMask (mask);
    }
    else
    {
	/* find maximum
	   todo: keep max while parsing, with select element struct */
	SdifListInitLoop(list);
	while (SdifSelectGetNextIntRange(list, &range, 1))
	{
	    if (range.range > max)
		max = range.range;
	    /* later: check here for open-end range type (selection of "1-end") */
	}

	/* allocate mask */
	mask->mask = SdifCalloc(int, max + 1);

	/* fill mask */
	SdifListInitLoop(list);
	while (SdifSelectGetNextIntRange(list, &range, 1))
	{
	    for (i = range.value; i <= range.range; i++)
	    {
		num += mask->mask[i] != 1; /* count fresh ints in selection */
		mask->mask[i] = 1;
	    }
	}

	mask->num  = num;
	mask->max  = max;
	mask->openend = 0;
    }
}



/*
// FUNCTION GROUP:	internal parsing functions
*/


/* abbreviations */
#define symbol(token)	(assert(token <= sst_num), \
			 SdifSelectSeparators [token])
#define symlen(token)	(assert(token <= sst_num), \
			 SdifSelectSeparatorLen [token])


/*static SdifSelectTokens	TOKEN;
static const char	*INPUT, *SYMBOL, *ORIG;*/




static SdifSelectTokens findtoken (SdifSelectContext *context)
{
    SdifSelectTokens t = sst_norange;
    while (t < sst_num  &&  strncmp (INPUT, symbol(t), symlen(t)) != 0) {
#ifdef __cplusplus
	t = (SdifSelectTokens) ((int) t+1);
#else
	++t;
#endif
    }
    return (t);
}


/* skip space by advancing INPUT to first non-space character */
static void skipspace (SdifSelectContext *context)
{
    INPUT += strspn (INPUT, SdifSelectWhitespace);
}


/* Sets TOKEN to next token read.  Consumes INPUT, when token found
   (if not, caller has to advance).  Returns true if a token from
   SdifSelectTokens was found. */
static SdifSelectTokens parsenexttoken (SdifSelectContext *context)
{
    skipspace (context);
    TOKEN  = findtoken (context);/* find token */
    SYMBOL = INPUT;			  /* points to token's symbol */
    INPUT += symlen(TOKEN);	  /* advance INPUT, when token found. */

    if (debug)  fprintf (stderr, 
	"parsenexttoken\tTOKEN %2d %-2s  SYMBOL '%10s'  INPUT '%10s'\n",
	TOKEN, symbol (TOKEN), SYMBOL, INPUT);
    return (TOKEN);
}



/* parse<type> functions parse one <type> element (ignoring leading
   whitespace).  They consume the input, i.e. advance INPUT to the
   char after what they've parsed!  (SYMBOL stays on what was parsed,
   for error output.)  They return true on success 
*/

static int parseint (SdifSelectValueT *valu,SdifSelectContext *context)
{
    SYMBOL = INPUT;
    valu->integer = strtol (SYMBOL, (/*not const*/ char **) &INPUT, 10);
    if (debug)  fprintf (stderr, 
    "parseint\t\tTOKEN %2d %-2s  SYMBOL '%10s'  INPUT '%10s'  ret %d  val %d\n",
    TOKEN, symbol (TOKEN), SYMBOL, INPUT, INPUT > SYMBOL, valu->integer);
    return (INPUT > SYMBOL);	/* if pointer has advanced, we have
				   successfully parsed something */
}

static int parsereal (SdifSelectValueT *valu,SdifSelectContext *context)
{
    SYMBOL = INPUT;
    valu->real = strtod (SYMBOL, (/*not const*/ char **) &INPUT);
    return (INPUT > SYMBOL);	/* if pointer has advanced, we have
				   successfully parsed something */
}


/* read until next separator character
   Later: handle quoting "..." or '...'
   Set SYMBOL to string read (non-terminated!), return length, advance INPUT.
*/
static int parsestring (SdifSelectContext *context)
{
    int len;

    skipspace (context);/* advance INPUT to first non-space character */
    SYMBOL = INPUT;

    /* find first separator character (NULL if string ended) */
    INPUT = strpbrk (SYMBOL, sep_first);
    len   = INPUT  ?  INPUT - SYMBOL  :  strlen (SYMBOL);
    if (!INPUT)   INPUT = "";
    return (len);
}


/* read until next separator character
   Later: read only allowed chars for signature, handle "...". */
static int parsesig (SdifSelectValueT *valu, SdifSelectContext *context)
{
    char sigstr [4];
    int	 siglen = parsestring (context);

    memset(sigstr,0,4);

    strncpy (sigstr, SYMBOL, MIN (siglen, 4));
    valu->signature = SdifStringToSignature (sigstr);

    if (debug) fprintf (stderr, 
	"parsesig\t\tTOKEN %2d %-2s  SYMBOL '%10s'  INPUT '%10s'  "
	"ret %d  val '%s'  siglen %d\n",
	TOKEN, symbol (TOKEN), SYMBOL, INPUT, siglen != 0, 
	SdifSignatureToString (valu->signature), siglen);
    return (siglen != 0);
}


/* read either column name (later) or number
   Later: handle quotes "...". 
*/
static int parsecol (SdifSelectValueT *valu, SdifSelectContext *context)
{
    if (parseint (valu, context))	/* see if we can find a number */
	return (1);
    else
    {	/* no, parse column name and later find column index from types */
	SdifColumnDefT  *col   = NULL;
	int		len    = parsestring (context);
	char		*cname = SdifCalloc (char, 100);

	strncpy (cname, SYMBOL, len);
	cname [len] = '0';
	/*
	col = SdifMatrixTypeGetColumnDef (SdifMatrixTypeT *MatrixType, cname);
        */

	if (debug) fprintf (stderr,
	    "parsecol\t\tTOKEN %2d %-2s  SYMBOL '%10s'  INPUT '%10s'  "
	    "ret %d  colname '%s'\n",
	    TOKEN, symbol (TOKEN), SYMBOL, INPUT, col != NULL, cname);
	assert (!"can't handle column names, yet");
	SdifFree (cname);
	return (col != NULL);
    }
}



/* Parse one element's list of values plus range or delta.  
   Return true if ok. */
static int parse (int (*parseval) (SdifSelectValueT *valu, SdifSelectContext *context), SdifListP list, 
		  int range_allowed, char *name, SdifSelectContext *context)
{
#   define print_error1(msg, arg)	/* todo: use sdiferr... */       \
	   fprintf (stderr,						 \
		    "ERROR: SDIF selection: can't parse %s from '%s'\n(recently read: '%.*s'):\n" msg "\n\n", name, SYMBOL, (int)(SYMBOL - ORIG), ORIG, arg)

#   define print_error0(msg)	/* todo: use sdiferr... */       \
	   fprintf (stderr,						 \
		    "ERROR: SDIF selection: can't parse %s from '%s'\n(recently read: '%.*s'):\n" msg "\n\n", name, SYMBOL, (int)(SYMBOL - ORIG), ORIG)

    int		       ret   = 0;	/* being pessimistic */
    SdifSelectElementT *elem = SdifMalloc (SdifSelectElementT);
    elem->rangetype = sst_norange;

    if (parseval (&elem->value, context))
    {	
	switch (parsenexttoken (context))
	{
	    case sst_range:
	    case sst_delta:	/* add elem only if range ok */
		if (range_allowed)
		{
		    if (parseval (&elem->range, context))
		    {   /* add elem and continue looking for list */
			elem->rangetype = TOKEN;
			SdifListPutTail (list, elem);
			if (parsenexttoken (context) == sst_list)
			    ret = parse (parseval, list, range_allowed, name, context);
			else
			    if (!(ret = TOKEN != sst_range  &&  TOKEN != sst_delta))
				print_error1 ("Another range symbol '%s' after complete range", symbol(TOKEN));
				
		    }
		    else
			print_error0 ("Malformed range expression\n");
		}
		else
		    print_error0 ("Range only permitted with numerical values\n");
	    break;
	
	    case sst_list:	/* add elem and recursively read comma-list */
		SdifListPutTail (list, elem);
		ret = parse (parseval, list, range_allowed, name, context);
	    break;

	    case sst_num:	/* next token is no separator */
		SdifListPutTail (list, elem);   /* add this elem in any case */
		if (*INPUT)	/* error when not at end of INPUT */
		    print_error1 ("No separator after value '%s'\n", INPUT);
		else
		    ret = 1;
	    break;

	    default:		/* token for next element parsed */
		SdifListPutTail (list, elem);   /* add this elem and proceed */
		ret = 1;	/* ok: token for next element */
	    break;
	}
    }
    else
	print_error1 ("Can't read value from '%s'\n", INPUT);

    if (debug)  fprintf (stderr, 
	"parse %s\tTOKEN %2d %-2s  SYMBOL '%10s'  INPUT '%10s'  ret %d\n",
	name, TOKEN, symbol (TOKEN), SYMBOL, INPUT, ret);
    return (ret);
}




/*
// FUNCTION GROUP:	externally callable functions
*/

/* parse elements #stream :frame /matrix .column _row @time in any order */
int SdifParseSelection (SdifSelectionT *sel, const char *str)
{
    int ret = 2;	/* first iteration */

    SdifSelectContext theContext;
	SdifSelectContext *context = &theContext;
    INPUT = ORIG = str;	/* set input to parse from */
	
    if (INPUT)
	parsenexttoken (context);
    while (ret  &&  INPUT  &&  *INPUT)
    {
	switch (TOKEN)
	{
	    case sst_stream: 
		ret = parse (parseint,  sel->stream, 1, "stream", context); break;
	    case sst_frame:  
		ret = parse (parsesig,  sel->frame,  0, "frame", context);  break;
	    case sst_matrix: 
		ret = parse (parsesig,  sel->matrix, 0, "matrix", context); break;
	    case sst_column: 
		ret = parse (parsecol,  sel->column, 1, "column", context); break;
	    case sst_row:    
		ret = parse (parseint,  sel->row,    1, "row", context);    break;
	    case sst_time:   
		ret = parse (parsereal, sel->time,   1, "time", context);   break;
	    default:
		if (ret == 2  &&  *INPUT)
		{   /* special case: if frame is first element, don't
		       need to use symbol : to avoid ":::1FRM" */
		    ret = parse (parsesig, sel->frame, 0, "frame", context);
		}
	    break;
	}
    }

    /* convert row/column selection to masks */
    SdifSelectGetIntMask(sel->stream, &sel->streammask);
    SdifSelectGetIntMask(sel->row,    &sel->rowmask);
    SdifSelectGetIntMask(sel->column, &sel->colmask);

    return (ret);
}


/* parse comma-separated list of signatures into list of SdifSelectElementT,
   return true if ok */
int SdifParseSignatureList (SdifListT *list, const char *str)
{
    SdifSelectContext context;

	context.mInput = str;
	context.mOriginal = str;
    return (parse (parsesig,  list,  0, "signature-list",&context));
}


/* Returns pointer to first char of select spec (starting with ::), or
   NULL if not found.  */
char *SdifSelectFindSelection (const char *filename)
{
    const char  *spec = filename;
    char	*last = NULL;

    assert (gSdifInitialised  &&  "SDIF library not initialised!");

    /* find last spec separator '::' in last */
    while (spec)
    {
	if ((spec  = strstr (spec, symbol(sst_specsep))))
	{
	    last  = (char *) spec;
	    spec += symlen (sst_specsep);
	}
    }
    return (last);
}


char *SdifGetFilenameAndSelection (/*in*/  const char *filename, 
				   /*out*/ SdifSelectionT *sel)
{
    const char *spec = SdifSelectFindSelection (filename);

    SdifInitSelection  (sel, filename, spec  ?  spec - filename 
					     :  (filename ? strlen (filename)
						          : 0));
    if (spec)
	SdifParseSelection (sel, spec + symlen (sst_specsep));
    return (sel->filename);
}

void SdifReplaceSelection (/*in*/  const char *selectionstr, 
			   /*out*/ SdifSelectionT *sel)
{

    SdifFreeSelectionLists (sel);
    SdifInitSelectionLists (sel);

    if(selectionstr)
	SdifParseSelection (sel, selectionstr );
    return;
}


void SdifPrintSelection (FILE *out, SdifSelectionT *sel, int options)
{
    char *tn [] = {" ", " any\n"}, 
	 *nc [] = {"\n", ", "};

#   define printinit(elem) \
    fprintf (out, "   %-6s:%s", #elem, tn [SdifListIsEmpty(sel->elem)]);\
    SdifListInitLoop (sel->elem)

#   define printrange(elem, type, fmt)		\
    {	SdifSelectElement##type##T range;	printinit (elem);\
        while(SdifSelectGetNext##type##Range (sel->elem, &range, options & 1))\
        {  \
	    int nor = range.rangetype == sst_norange, \
		nxt = SdifListIsNext (sel->elem);     \
	    fprintf (out,         nor ? fmt "%s" : fmt " %s " fmt "%s",	      \
		     range.value, nor ? nc [nxt] : symbol (range.rangetype),  \
		     range.range, nc [nxt]); \
	} \
    }

#   define printsig(elem)    \
    {	SdifSignature sig;   printinit (elem); \
        while ((sig = SdifSelectGetNextSignature (sel->elem)))   {	\
	    fprintf (out, "%s%s", SdifSignatureToString (sig),		\
				  nc [SdifListIsNext (sel->elem)]);     \
	} \
    }

    fprintf (out, "selection: file %s  basename %s\n", 
	     sel->filename, sel->basename);
    printrange (stream, Int,  "%d");
    printsig   (frame);
    printsig   (matrix);
    printrange (column, Int,  "%d");
    printrange (row,    Int,  "%d");
    printrange (time,   Real, "%g");
}




/*
// FUNCTION GROUP:	Access functions for SdifSelectValueT
*/
/* seem to be unused
static int	      getint	   (SdifSelectValueT val)
{ return (val.integer);   }

static double	      getreal	   (SdifSelectValueT val)
{ return (val.real);	  }	  
					  
static SdifSignature  getsignature (SdifSelectValueT val)
{ return (val.signature); }	  
					  
static char	     *getstring	   (SdifSelectValueT val)
{ return (val.string);    }

*/

/*
// FUNCTION GROUP:	Add Selections to Element Lists
*/


#define _foralltypes(macro) \
macro (Int,	  int,		 integer)   \
macro (Real,	  double,	 real)	    \
macro (Signature, SdifSignature, signature) \
macro (String,	  char *,	 string)


#define _addrange(_name_, _type_, _field_) \
_addrangeproto   (_name_, _type_, _field_) \
{   SdifSelectElementT *elem = SdifMalloc (SdifSelectElementT);	        \
    elem->value._field_ = Value;     \
    elem->range._field_ = Range;     \
    elem->rangetype   = Rt;        \
    SdifListPutTail (List, elem);  \
}

#define _addsimple(_name_, _type_, _field_) \
_addsimpleproto   (_name_, _type_, _field_) \
{   SdifSelectAdd##_name_##Range (List, Value, sst_norange, (_type_) 0);   }

#define _add(_name_, _type_, _field_) \
_addrange   (_name_, _type_, _field_)  \
_addsimple  (_name_, _type_, _field_)

_foralltypes (_add)


/*
// FUNCTION GROUP:	Query parsed ranges (list of ranges).

   Init list traversal with SdifListInitLoop, then call SdifSelectGetNext* 
   until it returns 0.
*/

int SdifSelectGetNextIntRange  (/*in*/  SdifListP list, 
				/*out*/ SdifSelectElementIntT  *range, 
				/*in*/  int force_range)
{
    int avail, delta;

    if ((avail = SdifListIsNext (list)))
    {
	SdifSelectElementT *elem = (SdifSelectElementT *) SdifListGetNext (list);
	
	if (force_range)
	{
	    switch (elem->rangetype)
	    {
	    	case sst_range:
		    range->value = MIN (elem->value.integer, elem->range.integer);
		    range->range = MAX (elem->value.integer, elem->range.integer);
	    	break;
    
	    	case sst_delta:
		    delta        = abs ((int)elem->range.integer);
		    range->value = elem->value.integer - delta;
		    range->range = elem->value.integer + delta;
	    	break;

		default:
		    range->value = elem->value.integer;
		    range->range = elem->value.integer;
		break;
	    }
	    range->rangetype = sst_range;
	}
	else
	{
	    range->value     = elem->value.integer;
	    range->range     = elem->range.integer;
	    range->rangetype = elem->rangetype;
	}
    }
    return (avail);
}


int SdifSelectGetNextRealRange (/*in*/  SdifListP list, 
				/*out*/ SdifSelectElementRealT *range, 
				/*in*/  int force_range)
{
    int		avail;
    double	delta;

    if ((avail = SdifListIsNext (list)))
    {
	SdifSelectElementT *elem = (SdifSelectElementT *) SdifListGetNext (list);
	
	if (force_range)
	{
	    switch (elem->rangetype)
	    {
	    	case sst_range:
		    range->value = MIN (elem->value.real, elem->range.real);
		    range->range = MAX (elem->value.real, elem->range.real);
	    	break;
    
	    	case sst_delta:
		    delta        = fabs (elem->range.real);
		    range->value = elem->value.real - delta;
		    range->range = elem->value.real + delta;
	    	break;

		default:
		    range->value = elem->value.real;
		    range->range = elem->value.real;
		break;
	    }
	    range->rangetype = sst_range;
	}
	else
	{
	    range->value     = elem->value.real;
	    range->range     = elem->range.real;
	    range->rangetype = elem->rangetype;
	}
    }
    return (avail);
}


char *SdifSelectGetNextString    (/*in*/  SdifListP list)
{
     return (SdifListIsNext (list)  
	     ?  ((SdifSelectElementT *) SdifListGetNext (list))->value.string
	     :  NULL);
}


SdifSignature SdifSelectGetNextSignature (/*in*/  SdifListP list)
{
     return (SdifListIsNext (list)  
	     ?  ((SdifSelectElementT *) SdifListGetNext(list))->value.signature
	     :  eEmptySignature);
}


/* define code for:
int	       SdifSelectGetFirstInt       (SdifListP l, int defval);
double	       SdifSelectGetFirstReal      (SdifListP l, double defval);
char	      *SdifSelectGetFirstString    (SdifListP l, char *defval);
SdifSignature  SdifSelectGetFirstSignature (SdifListP l, SdifSignature defval);

Return value of first selection (ignoring range).
*/


#define _getfirst(name, type, field)					     \
type SdifSelectGetFirst##name (/*in*/ SdifListP list, type defval)	     \
{    SdifListInitLoop (list);						     \
     return (SdifListIsNext (list)  					     \
	     ?  ((SdifSelectElementT *) SdifListGetNext (list))->value.field \
	     :  defval);						     \
}

_foralltypes (_getfirst)



/*
// FUNCTION GROUP:	Selection Testing Functions
*/

int SdifSelectTestIntMask (SdifSelectIntMaskT *mask, SdifUInt4 cand)
{
    if (mask->num == 0  ||  cand > mask->max)
	return (mask->openend);
    else
	return (mask->mask[cand]);
}


int SdifSelectTestIntRange (SdifSelectElementT *elem, SdifUInt4 cand)
{
    if (!elem)   return (0);

    switch (elem->rangetype)
    {
    	case sst_norange: 
	    return (elem->value.integer == cand);
    	case sst_range: 
	    if (elem->value.integer <= elem->range.integer)
	        return (elem->value.integer <= cand  &&  cand <= elem->range.integer);
	    else
	        return (elem->value.integer >= cand  &&  cand >= elem->range.integer);	    
    	case sst_delta: 
	    return ( ((SdifUInt4)abs (((int)elem->value.integer - (int)cand))) <= elem->range.integer);
        default:
	    assert (!"corrupt rangetype");
	    return (0);
    }
}


int SdifSelectTestRealRange (SdifSelectElementT *elem, double cand)
{
    if (!elem)   return (0);

    switch (elem->rangetype)
    {
    	case sst_norange: 
	    return (elem->value.real == cand);
    	case sst_range: 
	    if (elem->value.real <= elem->range.real)
	        return(elem->value.real <= cand  &&  cand <= elem->range.real);
	    else
	        return(elem->value.real >= cand  &&  cand >= elem->range.real);
    	case sst_delta: 
	    return (fabs (elem->value.real - cand) <= fabs (elem->range.real));
        default:
	    assert (!"corrupt rangetype");
	    return (0);
    }
}



int SdifSelectTestInt (SdifListT *list, SdifUInt4 cand)
{
    if (SdifListIsEmpty (list))
	return (1);	/* no select spec means: take everything */
    SdifListInitLoop (list);
    while (SdifListIsNext (list))
    {
	if (SdifSelectTestIntRange ((SdifSelectElementT *)SdifListGetNext (list), cand))
	    return (1);
    }
    return (0);
}

int SdifSelectTestReal (SdifListT *list, double cand)
{
    if (SdifListIsEmpty (list))
	return (1);	/* no select spec means: take everything */

    /* first test from current select element onwards */
    if (SdifSelectTestRealRange ((SdifSelectElementT *)SdifListGetCurr (list), cand))
        return (1);

    SdifListInitLoop (list);
    while (SdifListIsNext (list))
    {
	if (SdifSelectTestRealRange ((SdifSelectElementT *) SdifListGetNext (list), cand))
	    return (1);
    }
    return (0);
}

int SdifSelectTestSignature (SdifListT *list, const SdifSignature cand)
{
    if (SdifListIsEmpty (list))
	return (1);	/* no select spec means: take everything */
    SdifListInitLoop (list);
    while (SdifListIsNext (list))
    {
	if (((SdifSelectElementT *) SdifListGetNext (list))->value.signature 
	    == cand)
	    return (1);
    }
    return (0);
}

int SdifSelectTestString (SdifListT *list, const char *cand)
{
    if (SdifListIsEmpty (list))
	return (1);	/* no select spec means: take everything */
    SdifListInitLoop (list);
    while (SdifListIsNext (list))
    {
	if (strcmp (((SdifSelectElementT *) SdifListGetNext (list))->value.string, cand) == 0)
	    return (1);
    }
    return (0);
}


/*
// FUNCTION GROUP:	Using a Selection in File I/O.
*/


/* Get number of selected streams in file selection, 0 for all  */
int SdifFNumStreamsSelected (SdifFileT *file)
{
    return (file->Selection->streammask.num);
}

/* Get number of selected rows in file selection  */
int SdifFNumRowsSelected (SdifFileT *file)
{
    if (file->Selection->rowmask.num)
	return (file->Selection->rowmask.num);
    else
	return (SdifFCurrNbRow(file));
}

/* Get number of selected columns in file selection  */
int SdifFNumColumnsSelected (SdifFileT *file)
{
    if (file->Selection->colmask.num)
	return (file->Selection->colmask.num);
    else
	return (SdifFCurrNbCol(file));
}


/* Test the selection elements applicable to frames: time, stream,
   frame type.  Can be called after SdifFReadFrameHeader().  */
int SdifFrameIsSelected (SdifFrameHeaderT *frame, SdifSelectionT *sel)
{
    return ((SdifSelectTestIntMask   (&sel->streammask, frame->NumID)
	     ||  frame->NumID == _SdifAllStreamID)	         &&
	     SdifSelectTestReal	     (sel->time,   frame->Time)  &&
	     SdifSelectTestSignature (sel->frame,  frame->Signature));
}

/* Test the selection elements applicable to matrices: the matrix
   signature Can be called after SdifFReadMatrixHeader().  */
int SdifMatrixIsSelected (SdifMatrixHeaderT *matrix, SdifSelectionT *sel)
{
    return (SdifSelectTestSignature (sel->matrix, matrix->Signature));
}


/* Test the selection elements applicable to frames: time, stream,
   frame type.  Can be called after SdifFReadFrameHeader().  */
int SdifFCurrFrameIsSelected (SdifFileT *file)
{
    return (SdifFrameIsSelected (file->CurrFramH, file->Selection));
}

/* Test the selection elements applicable to matrices: the matrix
   signature Can be called after SdifFReadMatrixHeader().  */
int SdifFCurrMatrixIsSelected (SdifFileT *file)
{
    return (SdifMatrixIsSelected (file->CurrMtrxH, file->Selection));
}

/* Test file selection if a given row (starting from 1) is selected */
int SdifFRowIsSelected (SdifFileT *file, int row)
{
    return (SdifSelectTestIntMask(&file->Selection->rowmask, row));
}

/* Test file selection if a given column (starting from 1) is selected */
int SdifFColumnIsSelected (SdifFileT *file, int col)
{
    return (SdifSelectTestIntMask(&file->Selection->colmask, col));
}
