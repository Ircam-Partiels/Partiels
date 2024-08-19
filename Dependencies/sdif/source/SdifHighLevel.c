/* 
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
 */

/* $Id: SdifHighLevel.c,v 3.21 2011-04-06 17:08:45 diemo Exp $
 *
 * SdifHighLevel.c	8.12.1999	Diemo Schwarz
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.20  2009/08/10 17:22:23  diemo
 * fix matrix selection for SdifQuery: In SdifReadFile function, check if
 * matrix header is selected before calling matrix callback.
 *
 * Revision 3.19  2009/04/09 09:54:43  diemo
 * comm
 *
 * Revision 3.18  2007/11/26 18:52:08  roebel
 * Changed data types to have less casts and
 * less compilation warnings with MSVC.
 * renamed some functions that are deprecated in MSVC.
 *
 * Revision 3.17  2005/10/21 14:32:29  schwarz
 * protect all static buffers from overflow by using snprintf instead of sprintf
 * move big errorMess buffers into error branch to avoid too large stack allocation
 *
 * Revision 3.16  2005/05/23 17:52:53  schwarz
 * Unified error handling:
 * - SdifErrorEnum (global errors) integrated into SdifErrorTagET (file errors)
 * - no more SdifError.[ch], everything done by SdifErrMess.[ch]
 *
 * Revision 3.15  2005/05/20 21:13:09  roebel
 * (Module):
 * back to returning 0 in case of file read error.
 * the case where it was really used for max is now checked
 * by means of comparing the expected matrix size.
 *
 * Revision 3.14  2005/05/13 16:12:58  schwarz
 * oops, debug output!
 *
 * Revision 3.13  2005/05/13 15:17:48  schwarz
 * stop read loop on errors and return values of callbacks
 *
 * Revision 3.12  2004/09/14 15:44:00  schwarz
 * protect from wrong file or no file errors
 * SdifMinMaxT with double
 *
 * Revision 3.11  2004/09/13 13:06:27  schwarz
 * SdifReadSimple even simpler, SdifReadFile for full-scale callback reading.
 * Moving the functionality of querysdif into the library with SdifQuery,
 * result in SdifQueryTreeT.
 *
 * Revision 3.10  2004/09/09 17:52:16  schwarz
 * SdifReadSimple: simple callback-based reading of an entire SDIF file.
 *
 * Revision 3.9  2004/06/03 11:18:00  schwarz
 * Profiling showed some waste of cycles in byte swapping and signature reading:
 * - byte swapping now array-wise, not element-wise in SdifSwap<N>[Copy] routines:   -> from 0.24 s (18.5%) to 0.14s
 * - ASCII signature reading function SdiffGetSignature replaced by new binary
 *   function SdiffReadSignature (also in SdifFGetSignature, so the change is
 *   mostly transparent):
 *   -> from 0.11 s (9.6%)  to 0.01 s
 * - overall run time improvement with test case sdifextractall_a01:
 *   -> from 1.20 s         to 0.86 s (40% faster)
 *
 * Revision 3.8  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.7  2002/08/28 16:52:39  schwarz
 * Fixed bug that end of file was not recognised:
 * signature is then eEmptySignature, and not eEof.
 * Error check for SdifFReadFrameHeader.
 *
 * Revision 3.6  2001/05/02 09:34:44  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.5  2000/11/21 14:51:50  schwarz
 * - sdif.h is now included by all sdif/Sdif*.c files.
 * - Removed all public typedefs, enums, structs, and defines from the
 *   individual sdif/Sdif*.h files, because they were duplicated in sdif.h.
 * - Todo: Do the same for the function prototypes, decide which types and
 *   prototypes really need to be exported.
 * - Removed SdifFileStruct.h.
 * - Preliminary new version of SdiffGetPos, SdiffSetPos.  They used the
 *   type fpos_t, which is no longer a long on RedHat 7 Linux.
 *
 * Revision 3.4  2000/11/15 14:53:31  lefevre
 * no message
 *
 * Revision 3.3  2000/10/27  20:03:36  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.2  2000/08/22  13:38:33  schwarz
 * First alpha version of SdifFReadNextSelectedFrameHeader.
 *
 * Revision 3.1.2.1  2000/08/21  21:35:29  tisseran
 * *** empty log message ***
 *
 * Revision 3.1  2000/03/01  11:20:20  schwarz
 * Added preliminary sketch of SdifHighLevel
 */


/* include */		 /* to get these definitions: */
#include "sdif_portability.h"

#include "SdifGlobals.h"
#include "SdifFile.h"
#include "SdifFRead.h"
#include "SdifSelect.h"


#define _DB 0


/* Read frame headers until a frame matching the file selection has
   been found or the end of the file has been reached.  Return false
   if end of file was reached, true if data has been read. */
int SdifFReadNextSelectedFrameHeader (SdifFileT *f)
{
    size_t bytesread = 0, numread = 0;

    /* search for a frame we're interested in 
       TODO: heed max time */
    while (1)
    {
	if (SdifFCurrSignature(f) == eEmptySignature)
	    return 0;	/* EXIT RETURN EOF */

	/* TODO: 
	   first check: if frame sig not selected, don't read frame header
	   --> SdifFCurrSignatureIsSelected(), SdifFSkipFrame() */
	if (!(numread = SdifFReadFrameHeader(f)))
	    return 0;	/* EXIT RETURN ERROR */
	else
	    bytesread += numread;

	if (SdifFCurrFrameIsSelected (f))
	    break;	/* EXIT RETURN TRUE */

	bytesread += SdifFSkipFrameData (f);
	SdifFGetSignature(f, &bytesread);
    }

    return bytesread;
}




/*
 * callback-based reading 
 */

/* Reads an entire SDIF file, calling matrixfunc for each matrix in
   the SDIF selection taken from the filename.  Matrixfunc is called
   with the SDIF file pointer, the matrix count within the current
   frame, and the userdata unchanged.

   No row/column selection!

   returns success
*/
size_t SdifReadSimple (const char	       *filename, 
		       SdifMatrixDataCallbackT  matrixfunc,
		       void                    *userdata)
{
    return (SdifReadFile (filename, NULL, NULL, 
			  NULL, matrixfunc, NULL, userdata));
}


size_t SdifReadFile (const char             *filename, 
		     SdifOpenFileCallbackT   openfilefunc,
		     SdifFrameCallbackT      framefunc,
		     SdifMatrixCallbackT     matrixfunc,
		     SdifMatrixDataCallbackT matrixdatafunc,
		     SdifCloseFileCallbackT  closefilefunc,
		     void		    *userdata)
{
    SdifFileT *file = NULL;
    int	       eof  = 0, go_on = 1;
    size_t     bytesread = 0, newread;
    SdifUInt4  m, wantit;

    /* open input file (parses selection from filename into file->Selection) */
    file = SdifFOpen (filename, eReadFile);
    if (!file)
        return 0;

    bytesread  = SdifFReadGeneralHeader(file);
    if (!bytesread  ||  SdifFLastError(file))
	return 0;	/* something's wrong, maybe no SDIF file */

    bytesread += SdifFReadAllASCIIChunks(file);
    if (SdifFLastError(file))
    {   /* error has already been printed by the library, just clean
           up and exit */
	return 0;
    }

    /* call begin file handler, return false stops reading */
    if (openfilefunc)
	go_on = openfilefunc(file, userdata);

    /* main read loop:
       Read next selected frame header.  Current signature has
       already been read by SdifFReadAllASCIIChunks or the last loop. */
    while (go_on  &&  !eof  &&  
	   (newread = SdifFReadNextSelectedFrameHeader(file)) > 0)
    {
	bytesread += newread;

	/* call frame header handler that decides if we are interested */
	wantit = framefunc  ?  framefunc(file, userdata)  :  1;

#if _DB
	fprintf(SdifStdErr, 
		"@frame\t%s  matrices %d  stream %d  time %f, wantit %d\n",
		SdifSignatureToString(SdifFCurrFrameSignature(file)), 
		SdifFCurrNbMatrix(file), SdifFCurrID(file), 
		SdifFCurrTime(file), wantit);
#endif
	
	if (wantit)
	{          
	    /* for matrices loop */
	    for (m = 0; go_on  &&  m < SdifFCurrNbMatrix(file); m++)
	    {
		/* Read matrix header */
		newread = SdifFReadMatrixHeader(file);

		if (newread == 0)
		{   /* read problem (also with 0 since we want a full header)  */
		    go_on = 0;
		}
		else
		{
		    bytesread += newread;

		    /* Check if matrix type is in selection */
		    if (SdifFCurrMatrixIsSelected(file))
		    {   /* call matrix header handler */
			wantit = matrixfunc  ?  matrixfunc(file, m, userdata)  :  1;
		    }
		    else
			wantit = 0;
#if _DB
		    fprintf(SdifStdErr, 
			    "@matrix\t%s  rows %d  cols %d, wantit %d\n", 
			    SdifSignatureToString(SdifFCurrMatrixSignature(file)), 
			    SdifFCurrNbRow(file), SdifFCurrNbCol(file), wantit);
#endif
		    if (wantit)
		    {   /* read matrix data */
			newread = SdifFReadMatrixData(file);
		    
			if (newread == 0  &&  (SdifFCurrNbRow(file) != 0  &&
					       SdifFCurrNbCol(file) != 0))
			{   /* read problem (0 is ok, can be (0, n) matrix) */
			    go_on = 0;
			}
			else
			{   /* read ok */
			    bytesread += newread;
			    
			    /* call matrix data handler */
			    if (matrixdatafunc)
			    	go_on = matrixdatafunc(file, m, userdata);
			}
		    }
		    else
		    {   /* a matrix type we're not interested in, so we skip it */
			bytesread += SdifFSkipMatrixData(file);
		    }
		}
	    }   /* end for matrices */
	}
	else
	{   /* we didn't want this frame */
	    bytesread += SdifFSkipFrameData(file);
	}

	eof = SdifFGetSignature(file, &bytesread) == eEof;
    }   /* end while frames */ 

    if (closefilefunc)
	closefilefunc(file, userdata);

    /* cleanup */
    SdifFClose(file);

    return bytesread;
}




/*
 * file querying
 */

/* init SdifMinMaxT with sentinels */
#define initminmax(m)	((m).min = DBL_MAX, (m).max = -DBL_MAX)

/* update SdifMinMaxT structure */
#define minmax(m, v)	{ if ((v) < (m).min)   (m).min = (v); \
			  if ((v) > (m).max)   (m).max = (v); }


static int SigEqual (SdifQueryTreeElemT *node, 
		     SdifSignature s, int parent, int stream)
{
    return node->sig    == s
       &&  node->stream == stream
       &&  node->parent == parent;
}

int GetSigIndex (SdifQueryTreeT *tree, SdifSignature s, int parent, int stream)
{
    int i = 0;
    SdifQueryTreeElemT *node;

    while (i < tree->num  &&  !SigEqual(&tree->elems[i], s, parent, stream))
	i++;

    if (i == tree->num)
    {   /* add new signature */
	if (tree->num >= tree->allocated)
	{ /* grow elems array */
	    tree->allocated += 1024;
	    tree->elems     = SdifRealloc(tree->elems, SdifQueryTreeElemT,  
					  tree->allocated);
	}

	node         = &tree->elems[i];
	node->sig    = s;
	node->parent = parent;
	node->stream = stream;
	node->count  = 0;
	initminmax(node->time);
	initminmax(node->nmatrix);
	initminmax(node->ncol);
	initminmax(node->nrow);

	if (parent != -1)
	    tree->nummatrix++;	/* count distinct types */
	tree->num++;
    }

    return (i);
}

int SdifQueryCountFrame (SdifFileT *in, void *userdata)
{
    SdifQueryTreeT *tree    = (SdifQueryTreeT *) userdata;
    SdifSignature   sig	    = SdifFCurrSignature(in);
    int		    stream  = SdifFCurrID(in);
    SdifFloat8	    time    = SdifFCurrTime(in); /* keep double  for minmax */
    SdifFloat8	    nmatrix = SdifFCurrNbMatrix(in);   /* double is preferred for minmax */
    int		    i       = GetSigIndex(tree, sig, -1, stream);

    tree->elems[i].count++;
    minmax(tree->time,		   time);	/* global time */
    minmax(tree->elems[i].time,    time);	/* this frame/stream's time */
    minmax(tree->elems[i].nmatrix, nmatrix);
    tree->current = i;

    return 1;	/* go on reading matrices */
}


int SdifQueryCountMatrix (SdifFileT *in, int m, void *userdata)
{
    SdifQueryTreeT *tree   = (SdifQueryTreeT *) userdata;
    SdifSignature   sig	   = SdifFCurrMatrixSignature(in);
    SdifFloat8	    nrow   = SdifFCurrNbRow(in); /* double for minmax */
    SdifFloat8	    ncol   = SdifFCurrNbCol(in); /* double for minmax */
    int		    parent = tree->current;
    int		    i	   = GetSigIndex(tree, sig, parent, -1);

    tree->elems[i].count++;
    minmax(tree->elems[i].nrow, nrow);
    minmax(tree->elems[i].ncol, ncol);

    return 0;	/* don't read matrix data */
}



SdifQueryTreeT *SdifCreateQueryTree(int max)
{
    SdifQueryTreeT *newtree = SdifMalloc(SdifQueryTreeT);

    newtree->allocated = max;
    newtree->elems     = SdifCalloc(SdifQueryTreeElemT, max);

    return (SdifInitQueryTree(newtree));
}

void SdifFreeQueryTree(SdifQueryTreeT *tree)
{
    SdifFree(tree->elems);
    SdifFree(tree);
}

/* reset to zero */
SdifQueryTreeT *SdifInitQueryTree(SdifQueryTreeT *tree)
{
    tree->num       = 0;
    tree->nummatrix = 0;
    tree->current   = 0;
    initminmax(tree->time);

    return (tree);
}


size_t SdifQuery (const char            *filename, 
		  SdifOpenFileCallbackT  openfilefunc,
       /*out*/    SdifQueryTreeT        *tree)
{
    return (SdifReadFile(filename, openfilefunc, SdifQueryCountFrame, 
			 SdifQueryCountMatrix, NULL, NULL, (void *) tree));
}
