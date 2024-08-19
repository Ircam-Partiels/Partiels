/* $Id: SdifErrMess.c,v 3.28 2011-04-12 20:17:32 roebel Exp $
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
 * SdifErrorMess.c
 *
 * high level (SdifF) error management
 *
 * author: Dominique Virolle 1998
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.27  2011/04/12 14:18:18  roebel
 * Fixed Sdif[fF]GetPos and Sdif[fF]SetPos to correctly support large files (>2GB).
 *
 * Revision 3.26  2008/01/09 16:09:16  bogaards
 * clarified error message for badly formed strings
 *
 * Revision 3.25  2006/02/03 11:36:45  roebel
 * Fixed SdifFsPrintError which now creates a proper
 * error message. Before the error messages were
 * limited to 4 characters!
 *
 * Revision 3.24  2005/10/21 14:32:29  schwarz
 * protect all static buffers from overflow by using snprintf instead of sprintf
 * move big errorMess buffers into error branch to avoid too large stack allocation
 *
 * Revision 3.23  2005/05/24 18:19:43  roebel
 * Removed double semicolons, which apparently make gcc 2.96 stumble.
 *
 * Revision 3.22  2005/05/24 09:33:29  roebel
 * Fixed last checkin comment which turned out to be the start of
 * a c-comment.
 * Synchronized the extended ErrorTagET with the new
 * table of error messages.
 *
 * Revision 3.21  2005/05/23 19:17:53  schwarz
 * - Sdiffread/Sdiffwrite functions with SdifFileT instead of FILE *
 *   -> eof error reporting makes more sense
 * - more cleanup of sdif.h, above functions are private in SdifRWLowLevel.h
 * - eEof becomes error 4 to be distinguishable from ascii chars
 * - SdifFScanNameValueLCurrNVT reimplemented for ascii only
 *
 * Revision 3.20  2005/05/23 17:52:53  schwarz
 * Unified error handling:
 * - SdifErrorEnum (global errors) integrated into SdifErrorTagET (file errors)
 * - no more SdifError.[ch], everything done by SdifErrMess.[ch]
 *
 * Revision 3.19  2005/05/13 15:30:48  schwarz
 * make it possible that global errors from SdifError be passed through
 * the SdifErrMsg functions as file errors.
 *
 * Revision 3.18  2004/07/22 14:47:55  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.17  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.16  2003/11/07 12:09:08  ellis
 * Added the declaration of of two functions in the header file
 * SdifFAllFrameTypeToSdifString and SdifFAllMatrixTypeToSdifString
 *
 * Revision 3.15  2002/05/24 20:08:31  ftissera
 * Comment alternative to print error/warning message
 *
 * Revision 3.14  2002/05/24 19:36:38  ftissera
 * Change SdifFError to use new error and warning handlers.
 *
 * Revision 3.13  2001/07/12 14:15:31  roebel
 * Removed warning due to more columns then expected from the list
 * possible warnings. In fact this is allowed by the standard and should
 * not generate a warning.
 *
 * Revision 3.12  2001/05/02 09:34:40  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.11  2000/11/15 14:53:24  lefevre
 * no message
 *
 * Revision 3.10  2000/10/27  20:03:25  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.9  2000/08/22  13:17:23  schwarz
 * Centralised error report function SdifFError, called from _SdifFError
 * macro.  Error level names (gSdifErrorLevel) are printed, so that users
 * can tell warnings from errors.  Errors are counted by level in
 * SdifFileT.ErrorCount[].  Error counts can be queried with SdifFNumErrors.
 *
 * Revision 3.8  2000/08/07  15:05:44  schwarz
 * Error checking for read general header.
 * Remove double definition of 1GAI matrix type.
 * 
 * Revision 3.8.2.2  2000/08/21  21:34:59  tisseran
 * *** empty log message ***
 *
 * Revision 3.8.2.1  2000/08/21  14:04:03  tisseran
 * *** empty log message ***
 *
 * Revision 3.7  2000/05/15  16:22:29  schwarz
 * Avoid warning about KillerFT function pointer type (ANSI prototype given).
 * Argument to kill func is now void *.
 *
 * Revision 3.6  2000/03/01  11:17:29  schwarz
 * More meaningful error messages.
 *
 * Revision 3.5  1999/11/03  16:42:30  schwarz
 * Use _SdifNVTStreamID for stream ID of 1NVT frames because of CNMAT
 * restriction of only one frame type per stream.
 * (See SdifNameValuesLNewTable)
 *
 * Revision 3.4  1999/10/13  16:05:39  schwarz
 * Changed data type codes (SdifDataTypeET) to SDIF format version 3, as
 * decided with Matt Wright June 1999, added integer data types.
 * Added writing of 1NVT with real frame header (but data is still not in
 * matrices).
 * The data type handling makes heavy use of code-generating macros,
 * called for all data types with the sdif_foralltypes macro, thus
 * adding new data types is easy.
 *
 * Revision 3.3  1999/09/28  13:08:49  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.2  1999/09/28  10:34:47  schwarz
 * Added SdifEnableErrorOutput/SdifDisableErrorOutput to temporarily
 * switch off error messages when doing things that might fail.
 *
 * Revision 3.1  1999/03/14  10:56:33  virolle
 * SdifStdErr add
 */


#include "sdif_portability.h"

#include "SdifErrMess.h"
#include "SdifFile.h"
#include <string.h>
#include <stdlib.h>


static void SdifExit (void);

SdifExitFuncT	    gSdifExitFunc = SdifExit;
SdifExceptionFuncT  gSdifErrorFunc = SdifDefaultErrorFunc;
SdifExceptionFuncT  gSdifWarningFunc = SdifDefaultWarningFunc;

/* global variables set by the _SdifError macro */
/*char	     *SdifErrorFile;
int	      SdifErrorLine;
*/

/* global variables to control error output */
int	      gSdifErrorOutputEnabled = 1;
SdifErrorTagET gSdifLastError	      = eFalse; //eNoError;
FILE*	      SdifStdErr	      = NULL;


const char * gSdifErrorLevel [ eNumLevels ] = {
    "Fatal Error",
    "Error",
    "Warning",
    "Remark",
    "Message",	/* eNoLevel, shouldn't occur */
};


const SdifErrorT gSdifErrMessFormat[] = {
{ eUnknown,		 eNoLevel, "Warning unknown"},
{ eNoError,		 eNoLevel, "No Error"},
{ eTypeDataNotSupported, eWarning, "Type of data not actualy supported : %s\n"},
{ eNameLength,		 eWarning, "Badly formatted string : '%s'\n"},
{ eEof,			 eFatal,   "End of file : %s\n" },
{ eReDefined,		 eError,   "%s redefined\n"},
{ eUnDefined,		 eError,   "%s undefined\n"},
{ eSyntax,		 eWarning, "Syntax error: %s\n"},
{ eBadTypesFile,	 eWarning, "%s is not a Types Definitions file\n"},
{ eBadType,		 eWarning, "Bad Type used : %s\n"},
{ eBadHeader,		 eError,   "Bad Sdif Header : %s\n"},
{ eRecursiveDetect,	 eWarning, "Recursive declaration detected : %s\n"},
{ eUnInterpreted,	 eWarning, "Attempt to interpret %s failed\n"},
{ eOnlyOneChunkOf,	 eWarning, "There can be only 0 or 1 '%s' chunk.\n"},
{ eUserDefInFileYet,	 eWarning, "%s has been completed in this file yet\n"},
{ eBadMode,		 eError,   "Bad mode at sdif file opening (%d), %s\n"},
{ eBadStdFile,		 eError,   "Bad standart file or bad mode (%d), %s\n"},
{ eReadWriteOnSameFile,	 eError,   "Read file and Write file are the same file : %s"},
{ eBadFormatVersion,	 eError,   "Bad Format Version : %s"},
{ eMtrxUsedYet,		 eWarning, "Matrix has been used in this frame yet : %s\n"},
{ eMtrxNotInFrame,	 eWarning, "Matrix is not in frames components : %s\n"},
/* global errors */
{ eFreeNull,		 eFatal,   "Attempt to free a NULL pointer : '%s'\n" },
{ eAllocFail,		 eFatal,   "Attempt to allocate memory : '%s'\n" },
{ eArrayPosition,	 eFatal,   "Attempt to access to a non-existing square in an array : '%s'\n" },
{ eFileNotFound,	 eWarning, "File not found or permission denied: \"%s\"\n" },
{ eInvalidPreType,	 eWarning, "Invalid Predefined Type : %s\n" },
{ eAffectationOrder,	 eWarning, "Affectation must be in order : '%s'\n" },
{ eNoModifErr,		 eWarning, "Type has been defined yet: '%s'\n" },
{ eNotInDataTypeUnion,	 eFatal,   "Type of data Not in DataTypeUnion  : '%s'\n" },
{ eNotFound,	         eWarning, "Not Find : '%s'\n" },
{ eExistYet,	         eWarning, "Object Exists Yet : '%s'\n" },
{ eWordCut,	         eWarning, "Word cut read : '%s'\n" },
{ eTokenLength,	         eFatal,   "Token too long : '%s'\n" }
};



SdifErrorT*
SdifCreateError(SdifErrorTagET Tag, SdifErrorLevelET Level, const char* UserMess)
{
  SdifErrorT *NewError = NULL;

  NewError = SdifMalloc(SdifErrorT);
  if (NewError)
    {
      NewError->Tag		= Tag;
      NewError->Level	= Level;
      NewError->UserMess= SdifCreateStrNCpy(UserMess, SdifStrLen(UserMess)+1);
      
      return NewError;
    }
  else
    {
      _SdifError(eAllocFail, "Error allocation");
      return NULL;
    }
}


void
SdifKillError(void *Error)
{
  if (Error)
    {
	  if (((SdifErrorT *)Error)->UserMess)
		SdifKillStr(((SdifErrorT *) Error)->UserMess);

      SdifFree(Error);
    }
  else
    _SdifError(eFreeNull, "Error free");
}


SdifErrorLT*
SdifCreateErrorL(SdifFileT* SdifF)
{
  SdifErrorLT *NewErrorL = NULL;
  
  NewErrorL = SdifMalloc(SdifErrorLT);
  if (NewErrorL)
  {
	  NewErrorL->SdifF = SdifF;
      NewErrorL->ErrorList = SdifCreateList(SdifKillError);
      return NewErrorL;
	}
  else
    {
      _SdifError(eAllocFail, "ErrorL allocation");
      return NULL;
    }
}


void
SdifKillErrorL(SdifErrorLT *ErrorL)
{
/* Error->SdifF is only a link to know the Sdif file of the error
 * then it isn't killed
 */
  if (ErrorL)
    {
        SdifKillList(ErrorL->ErrorList);
	SdifFree(ErrorL);
    }
  else
    _SdifError(eFreeNull, "ErrorL free");
}




SdifUInt4 SdifInsertTailError(SdifErrorLT* ErrorL, unsigned int ErrorCount [], 
			      SdifErrorTagET Tag, const char* UserMess)
/*, SdifErrorT**  NewError)*/
{
    SdifErrorLevelET	Level    = gSdifErrMessFormat[Tag].Level;
    SdifUInt4		Count    = ++ErrorCount [Level];

    SdifErrorT*         NewError = SdifCreateError(Tag, Level, UserMess);
    /*    if(Level <= eError) {      */
      SdifListPutTail(ErrorL->ErrorList, NewError);
      /*    } */
    return Count;
}



SdifErrorT*
SdifLastError(SdifErrorLT *ErrorL)
{
	if (SdifListIsEmpty(ErrorL->ErrorList))
		return NULL;
	else
        return (SdifErrorT*) SdifListGetTail(ErrorL->ErrorList);
}



SdifErrorTagET
SdifLastErrorTag(SdifErrorLT *ErrorL)
{
	if (SdifListIsEmpty(ErrorL->ErrorList))
		return eNoError;
    else
        return ((SdifErrorT*) SdifListGetTail(ErrorL->ErrorList))->Tag;
}



/* called by macro _SdifFError */
SdifUInt4 SdifFError (SdifFileT* SdifF, SdifErrorTagET ErrorTag, 
		      const char *UserMess, const char *file, const int line)
{
    SdifUInt4   count = 0;
    char        sdifBufferError[4096];
    SdifErrorT *Error = NULL;
    SdifErrorLevelET ErrorLevel = gSdifErrMessFormat[ErrorTag].Level;

    if (SdifF)
    {   /* add to list and count error */
	count = SdifInsertTailError(SdifF->Errors, SdifF->ErrorCount,  
				    ErrorTag, UserMess);
	Error = SdifLastError(SdifF->Errors);
    }

    SdifFsPrintError(sdifBufferError, sizeof(sdifBufferError),
                     SdifF, ErrorTag, UserMess, 
		     __FILE__, __LINE__);

    /* call error/warning callback that handles printing */
    switch (ErrorLevel)
    {
        case eFatal:
        case eError:
	    (*gSdifErrorFunc)   (ErrorTag, ErrorLevel, sdifBufferError, 
				 SdifF, Error, (char *)file, line);
	    if (ErrorLevel == eFatal)
	    {
		(*gSdifExitFunc) ();
	    }
	    /* else: no exit, because it was always like that */
	break;

        case eWarning:
	    (*gSdifWarningFunc) (ErrorTag, eWarning, sdifBufferError, 
				 SdifF, Error, (char *)file, line);
	break;

/*
  default:
  _SdifError(eUnknow, "Unknown error level");
  break;
*/
        default:
	    if (gSdifErrorOutputEnabled)
		fprintf (SdifStdErr, "%s", sdifBufferError);
	break;
    }

    return count;    
}


SdifInt4
SdifFsPrintError(char* oErrMess, unsigned int oErrMessLen, 
                 SdifFileT* SdifF, SdifErrorTagET ErrorTag,
		 const char *UserMess, const char *LibFile, int LibLine)
{
    char HeadErrMess [512]  = "";
    char PosErrMess  [64]   = "";
    char TextErrMess [512]  = "";
    char FramErrMess [512]  = "";
    char MtrxErrMess [512]  = "";
    char ErrErrMess  [1024] = "";

#if defined (DEBUG)  ||  defined (_DEBUG)
    snprintf(HeadErrMess, sizeof(HeadErrMess),
	    "*Sdif* %s %d (%s, %d):\n  SdifFile: %s",
	    gSdifErrorLevel [gSdifErrMessFormat[ErrorTag].Level], ErrorTag,
	    LibFile, LibLine, SdifF ? SdifF->Name : "(no sdiffile)");
#else
    snprintf(HeadErrMess, sizeof(HeadErrMess),
	    "*Sdif* %s %d:\n  SdifFile: %s",
	    gSdifErrorLevel [gSdifErrMessFormat[ErrorTag].Level], ErrorTag, 
	    SdifF ? SdifF->Name : "(no sdiffile)");
#endif /* ifdef DEBUG */

    if (SdifF)
    {
        if (SdifF->Stream) 
            SdiffGetPos(SdifF->Stream, &(SdifF->Pos));

	if (SdifF->Pos !=0)
	    snprintf(PosErrMess, sizeof(PosErrMess), " (byte:%6llu=0x%04llx=0%06llo)", 
                     (long long) SdifF->Pos, (long long) SdifF->Pos, (long long) SdifF->Pos);

	if (SdifF->TextStream)
	    snprintf(TextErrMess, sizeof(TextErrMess), ", TextFile: %s\n", SdifF->TextStreamName);
	else
	    snprintf(TextErrMess, sizeof(TextErrMess), "\n");

	if (SdifF->CurrFramH)
	{
	    snprintf(FramErrMess, sizeof(FramErrMess), "  FramH : %s   Size: 0x%04x   NbMatrix: %u   NumID: %u   Time: %g\n",
		    SdifSignatureToString(SdifF->CurrFramH->Signature),
		    SdifF->CurrFramH->Size,  SdifF->CurrFramH->NbMatrix,
		    SdifF->CurrFramH->NumID, SdifF->CurrFramH->Time);
	}

	if (SdifF->CurrMtrxH)
	{
	    if (SdifF->CurrMtrxH->Signature != eEmptySignature)
	    {
		snprintf(MtrxErrMess, sizeof(MtrxErrMess), "  MtrxH :   %s   DataWidth: %04x   Rows: %d   Columns: %d\n",
			SdifSignatureToString(SdifF->CurrMtrxH->Signature),
			SdifF->CurrMtrxH->DataType,
			SdifF->CurrMtrxH->NbRow, SdifF->CurrMtrxH->NbCol);
	    }
	}
    }

    switch(ErrorTag)
    {
      case eNoError:
      case eUnknown:
          snprintf(ErrErrMess, sizeof(ErrErrMess), "%s", gSdifErrMessFormat[ErrorTag].UserMess);
      break;

      case eBadMode:
      case eBadStdFile:
	  snprintf(ErrErrMess, sizeof(ErrErrMess), gSdifErrMessFormat[ErrorTag].UserMess,
		  SdifF ? SdifF->Mode : -1, UserMess);
      break;

      default:
	  snprintf(ErrErrMess, sizeof(ErrErrMess), gSdifErrMessFormat[ErrorTag].UserMess, UserMess);
      break;
    }

    return snprintf(oErrMess, oErrMessLen, "%s%s%s%s%s--> %s\n",
		   HeadErrMess, PosErrMess, TextErrMess, FramErrMess,
		   MtrxErrMess, ErrErrMess);
}


/* moved from now obsolete SdifError.c */

static void 
SdifExit (void)
{
#ifdef USE_XPGUI
    XpExit(1);
#else
    exit(1);
#endif
}


void
SdifSetExitFunc (SdifExitFuncT func)
{
    gSdifExitFunc = func;
}

void
SdifSetErrorFunc (SdifExceptionFuncT func)
{
    gSdifErrorFunc = func;
}

void
SdifSetWarningFunc (SdifExceptionFuncT func)
{
    gSdifWarningFunc = func;
}


void	
SdifEnableErrorOutput  (void)
{
    gSdifErrorOutputEnabled = 1;
}

void
SdifDisableErrorOutput (void)
{
    gSdifErrorOutputEnabled = 0;
}

void
SdifDefaultErrorFunc (SdifErrorTagET errnum, SdifErrorLevelET errlev, 
		      char *msg, SdifFileT *file, SdifErrorT *error,
		      char *sourcefilename, int sourcefileline)
{
    if (gSdifErrorOutputEnabled)
    {
	fprintf(SdifStdErr, msg);
	fflush(SdifStdErr);
    }
}

void
SdifDefaultWarningFunc (SdifErrorTagET errnum, SdifErrorLevelET errlev, 
			char *msg, SdifFileT *file, SdifErrorT *error,
			char *sourcefilename, int sourcefileline)
{
    if (gSdifErrorOutputEnabled)
    {
	fprintf(SdifStdErr, msg);
	fflush(SdifStdErr);
    } 
}
