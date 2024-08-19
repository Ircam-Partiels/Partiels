/* $Id: SdifErrMess.h,v 3.15 2006-02-03 11:36:56 roebel Exp $
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
 * SdifErrMess.h
 *
 * high level (SdifF) error management
 *
 * author: Dominique Virolle 1998
 * 
 * $Log: not supported by cvs2svn $
 * Revision 3.14  2005/05/23 17:52:53  schwarz
 * Unified error handling:
 * - SdifErrorEnum (global errors) integrated into SdifErrorTagET (file errors)
 * - no more SdifError.[ch], everything done by SdifErrMess.[ch]
 *
 * Revision 3.13  2005/04/07 15:20:23  schwarz
 * removed duplicated declarations that belong to the external API in sdif.h
 *
 * Revision 3.12  2004/07/22 14:47:55  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.11  2002/05/24 19:36:38  ftissera
 * Change SdifFError to use new error and warning handlers.
 *
 * Revision 3.10  2001/07/12 14:15:31  roebel
 * Removed warning due to more columns then expected from the list
 * possible warnings. In fact this is allowed by the standard and should
 * not generate a warning.
 *
 * Revision 3.9  2001/05/02 09:34:40  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.8  2000/11/21 14:51:48  schwarz
 * - sdif.h is now included by all sdif/Sdif*.c files.
 * - Removed all public typedefs, enums, structs, and defines from the
 *   individual sdif/Sdif*.h files, because they were duplicated in sdif.h.
 * - Todo: Do the same for the function prototypes, decide which types and
 *   prototypes really need to be exported.
 * - Removed SdifFileStruct.h.
 * - Preliminary new version of SdiffGetPos, SdiffSetPos.  They used the
 *   type fpos_t, which is no longer a long on RedHat 7 Linux.
 *
 * Revision 3.7  2000/11/15 14:53:25  lefevre
 * no message
 *
 * Revision 3.6  2000/10/27  20:03:25  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.5  2000/08/22  13:17:24  schwarz
 * Centralised error report function SdifFError, called from _SdifFError
 * macro.  Error level names (gSdifErrorLevel) are printed, so that users
 * can tell warnings from errors.  Errors are counted by level in
 * SdifFileT.ErrorCount[].  Error counts can be queried with SdifFNumErrors.
 *
 * Revision 3.4  2000/08/07  15:05:44  schwarz
 * Error checking for read general header.
 * Remove double definition of 1GAI matrix type.
 *
 * Revision 3.3  2000/05/15  16:22:30  schwarz
 * Avoid warning about KillerFT function pointer type (ANSI prototype given).
 * Argument to kill func is now void *.
 *
 * Revision 3.2  1999/09/28  10:35:43  schwarz
 * Added SdifEnableErrorOutput/SdifDisableErrorOutput to temporarily
 * switch off error messages when doing things that might fail.
 *
 * Revision 3.1  1999/03/14  10:56:34  virolle
 * SdifStdErr add
 */


#ifndef _SDIFERRMESS_H
#define _SDIFERRMESS_H 1

#include <stdio.h>
#include <sdif.h>
#include "SdifGlobals.h"
#include "SdifList.h"


/* Table of error format for the fprintf */
extern const SdifErrorT gSdifErrMessFormat[];

extern SdifExitFuncT gSdifExitFunc;
extern SdifExceptionFuncT  gSdifErrorFunc;
extern SdifExceptionFuncT  gSdifWarningFunc;
extern FILE* SdifStdErr;


SdifErrorT*	SdifCreateError		(SdifErrorTagET Tag,
					 SdifErrorLevelET Level, 
					 const char* UserMess);
void		SdifKillError		(void *Error);
SdifErrorLT*	SdifCreateErrorL	(SdifFileT* SdifF);
void		SdifKillErrorL		(SdifErrorLT *ErrorL);
SdifUInt4       SdifInsertTailError     (SdifErrorLT* ErrorL, 
					 unsigned int ErrorCount [], 
					 SdifErrorTagET Tag, 
					 const char* UserMess);
/* SdifErrorT** Error); */
SdifErrorT*	SdifLastError		(SdifErrorLT *ErrorL);
SdifErrorTagET	SdifLastErrorTag	(SdifErrorLT *ErrorL);

SdifUInt4	SdifFError		(SdifFileT* SdifF, 
					 SdifErrorTagET ErrorTag, 
					 const char *UserMess, 
					 const char *file, 
					 const int line);
SdifInt4	SdifFsPrintError	(char* oErrMess, 
                                         unsigned int oErrMessLen,
                                         SdifFileT* SdifF,
					 SdifErrorTagET ErrorTag,
					 const char *UserMess,
					 const char *LibFile, int LibLine);


void SdifDefaultErrorFunc (SdifErrorTagET errnum, SdifErrorLevelET errlev, 
			   char *msg, SdifFileT *file, SdifErrorT *error,
			   char *sourcefilename, int sourcefileline);
void SdifDefaultWarningFunc (SdifErrorTagET errnum, SdifErrorLevelET errlev, 
			     char *msg, SdifFileT *file, SdifErrorT *error,
			     char *sourcefilename, int sourcefileline);


#define _SdifFError(SdifF, ErrorTag, UserMess) \
	 SdifFError(SdifF, ErrorTag, UserMess, __FILE__, __LINE__)


#define _SdifError(error, mess) _SdifFError(NULL, (error), (mess))


#define _Debug(mess) \
fprintf(SdifStdErr, "*Sdif Debug* %s, %d:\n", __FILE__, __LINE__), \
fprintf(SdifStdErr, "%s\n",(mess))

#define _SdifRemark(mess) \
fprintf(SdifStdErr, "*Sdif* %s\n", mess)


#endif  /* _SdifErrMess_ */
