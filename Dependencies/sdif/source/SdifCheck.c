/* $Id: SdifCheck.c,v 3.8 2005-04-07 15:56:46 schwarz Exp $
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
 * SdifCheck.c		2. May 2000		Diemo Schwarz
 * 
 * File test functions.
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.7  2004/07/13 18:03:30  roebel
 * Properly initialize byte counter variables to 0.
 *
 * Revision 3.6  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.5  2001/05/02 09:34:40  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.4  2000/11/15 14:53:23  lefevre
 * no message
 *
 * Revision 3.3  2000/10/27  20:03:24  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.2.2.2  2000/08/21  21:34:54  tisseran
 * *** empty log message ***
 *
 * Revision 3.2.2.1  2000/08/21  18:48:44  tisseran
 * Use SdifFSkipFrameData
 *
 * Revision 3.2  2000/05/15  16:22:02  schwarz
 * Changed prototypes of existing functions (we apologize for the inconvenience)
 * SdifFCurrFrameIsSelected and SdifFCurrMatrixIsSelected.
 * They now take only an SdifFileT as parameter, no longer a selection.
 * If you don't want to test the file selection (automatically
 * parsed from the filename), use these new functions like this:
 * SdifFrameIsSelected  (file->CurrFramH, myselection) and
 * SdifMatrixIsSelected (file->CurrMtrxH, myselection).
 *
 * Revision 3.1  2000/05/04  14:59:34  schwarz
 * Added modules SdifCheck and SdifSignatureTab, containing functions
 * formerly in SdifFile.
 *
 */

#include "sdif_portability.h"

#include <sdif.h>
#include "SdifFRead.h"
#include "SdifFGet.h"
#include "SdifFile.h"
#include "SdifSelect.h"


int
SdifCheckFileFormat (const char *name)
{
    int		ret = 0;
    size_t	size= 0;
    SdifFileT	*file;

    SdifDisableErrorOutput ();
    if ((file = SdifFOpen (name, eReadFile)))
    {
        SdifFGetSignature (file, &size);
        ret = file->CurrSignature == eSDIF;
	SdifFClose (file);
    }
    SdifEnableErrorOutput ();
    return (ret);
}


SdifSignature
SdifCheckFileFramesTab (const char *name, const SdifSignatureTabT *frames)
{
    SdifSignature  ret = eEmptySignature;
    size_t	   bytesread = 0;
    SdifFileT	   *in;

    if ((in = SdifFOpen (name, eReadFile)))
    {
	/* Read file header and first signature (ASCII chunks are
           treated as normal frames, here) */
        SdifFReadGeneralHeader  (in);
        SdifFGetSignature (in, &bytesread);
	ret = SdifCheckNextFrame (in, frames, NULL);
    }
    SdifFClose (in);
    return (ret);
}


int 
SdifCheckFileFramesIndex (const char *name, const SdifSignature *frames)
{
    int			ret = -1;
    size_t		bytesread = 0;
    SdifFileT	       *in;
    SdifSignatureTabT  *sigs = SdifCreateSignatureTab (1);

    while (*frames)
	SdifAddToSignatureTab (sigs, *frames++);

    if ((in = SdifFOpen (name, eReadFile)))
    {
	/* Read file header and first signature (ASCII chunks are
           treated as normal frames, here) */
        SdifFReadGeneralHeader  (in);
        SdifFGetSignature (in, &bytesread);
	SdifCheckNextFrame (in, sigs, &ret);
    }
    SdifFClose (in);
    SdifKillSignatureTab (sigs);
    return (ret);
}


SdifSignature
SdifCheckNextFrame (/*in*/  SdifFileT *in, const SdifSignatureTabT *frames,
		    /*out*/ int *index)
{
    size_t	bytesread=0;
    int		ret = -1;

    /* Loop while not end-of-file (signature is 0) and nothing found yet */
    while (SdifFCurrSignature (in)  &&  ret < 0)
    {
	/* Read frame header.  Current signature has already been read */
	SdifFReadFrameHeader (in);
	
	/* Check frame time, type, and stream id */
	if (SdifFCurrFrameIsSelected (in))
	{   /* Test if frame header is in list */
	    ret = SdifFindInSignatureTab (frames, SdifFCurrSignature (in));
	}
	
	/* Skip frame and read next sig in any case */
	SdifFSkipFrameData (in);
	SdifFGetSignature (in, &bytesread);
    }   /* end while frames */
    if (index)	*index = ret;
    return (SdifGetFromSignatureTab (frames, ret));
}
