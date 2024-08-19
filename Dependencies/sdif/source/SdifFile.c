/* $Id: SdifFile.c,v 3.66 2012-01-02 23:49:08 roebel Exp $
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
 * SdifFile.c
 *
 * Sdif File Management
 * High Level.
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.65  2011/06/11 23:58:04  roebel
 * Added missing include file io.h.
 *
 * Revision 3.64  2011/06/11 17:29:06  roebel
 * Fixed ftruncate implementation on windows.
 *
 * Revision 3.63  2011/04/12 20:17:32  roebel
 * Fixed large file support to properly work on linux as well.
 *
 * Revision 3.62  2011/04/12 14:18:18  roebel
 * Fixed Sdif[fF]GetPos and Sdif[fF]SetPos to correctly support large files (>2GB).
 *
 * Revision 3.61  2007/12/10 10:47:08  roebel
 * Use const char* for read only function arguments.
 *
 * Revision 3.60  2007/11/26 18:52:01  roebel
 * Changed data types to have less casts and
 * less compilation warnings with MSVC.
 * renamed some functions that are deprecated in MSVC.
 *
 * Revision 3.59  2007/03/21 19:44:15  roebel
 * Don't use global variables without initialization. These globals are treated differently
 * on MacOSX and they are not allowed in dynamic libraries without extra flags.
 * To simplify the situation I now initialized all global variables
 * or make them static.
 *
 * Revision 3.58  2005/10/21 14:32:29  schwarz
 * protect all static buffers from overflow by using snprintf instead of sprintf
 * move big errorMess buffers into error branch to avoid too large stack allocation
 *
 * Revision 3.57  2005/07/05 10:44:08  roebel
 * changed to be able to compile with mingw32
 *
 * Revision 3.56  2005/05/23 17:52:53  schwarz
 * Unified error handling:
 * - SdifErrorEnum (global errors) integrated into SdifErrorTagET (file errors)
 * - no more SdifError.[ch], everything done by SdifErrMess.[ch]
 *
 * Revision 3.55  2005/05/19 14:07:47  roebel
 * new pipe condition needs to be negated.
 *
 * Revision 3.54  2005/05/18 20:20:31  roebel
 * Fixed detection of pipes
 *
 * Revision 3.53  2005/05/13 15:25:20  schwarz
 * file name in identstring
 *
 * Revision 3.52  2005/04/19 15:30:14  schwarz
 * make sdifcpp compile again for easdif:
 * - removed deleted files from makefiles
 * - fixed some includes that were missing (but only for C++ compilation!?)
 *
 * Revision 3.51  2005/04/07 15:56:47  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.50  2005/04/07 13:57:53  schwarz
 * removed debug code used to find struct member alignment problem
 * (--> do this at init by comparing sizeof(SdifFileT) in lib and caller?)
 *
 * Revision 3.49  2005/04/06 16:21:18  schwarz
 * project for MS Visual Studio C++ 7, configured for building with FTM (struct member align 2)
 *
 * Revision 3.48  2004/09/14 15:44:18  schwarz
 * protect from wrong file or no file errors
 *
 * Revision 3.47  2004/09/09 17:41:41  schwarz
 * Whole matrix data can be read into field CurrMtrxData and accessed
 * with access functions SdifFCurrMatrixData, SdifFCurrMatrixDataPointer.
 *
 * Revision 3.46  2004/07/22 14:47:56  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.45  2004/06/09 10:54:43  schwarz
 * Fixed compilation problems on Windows (thanks Jean Bresson):
 * - void pointer arithmetic not allowed
 * - open is a Unix system call, use fopen instead
 * - strdup does not exists, so compile with -Dstrdup=_strdup
 *
 * Revision 3.44  2004/05/27 13:35:53  ellis
 * if SdifGenKill called while Sdif not being initialised, directly returning
 * (allows to call SdifGenKill more than once)
 *
 * Revision 3.43  2004/02/11 16:24:11  roebel
 * And once again, fixed conditions for warning message.
 *
 * Revision 3.42  2004/02/11 15:44:05  roebel
 * Again an update of handling of the warning for missing types file.
 *
 * Revision 3.41  2004/02/10 14:55:41  roebel
 * Removed remark (warning) for the case where types file name is "".
 *
 * Revision 3.40  2004/02/08 14:26:58  ellis
 *
 * now the textual scanner parses correctly character datas
 *
 * Revision 3.39  2003/12/15 13:14:50  schwarz
 * Added SdifFileT based functions SdifFSetPos, SdifFGetPos around the
 * Sdiff* Macros, to be callable from OpenMusic.
 *
 * Revision 3.38  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.37  2003/11/07 15:29:28  tisseran
 * Removed inclusion of SdifFile.h
 * Used SDIF_VERSION now.
 *
 * Revision 3.36  2003/11/07 12:09:08  ellis
 * Added the declaration of of two functions in the header file
 * SdifFAllFrameTypeToSdifString and SdifFAllMatrixTypeToSdifString
 *
 * Revision 3.35  2003/08/06 15:10:46  schwarz
 * Finally removed obsolete functions (like SdifSkip...).
 *
 * Revision 3.34  2003/06/24 16:01:32  roebel
 * permanently removed references to UniversalEnvVar.h
 *
 * Revision 3.33  2003/06/24 15:24:58  roebel
 * Removed UniversalEnvVar.h which in fact has never been used in SDIF.
 *
 * Revision 3.32  2003/06/06 10:25:44  schwarz
 * Added eReadWriteFile that eventually opens a file in read-write mode.
 *
 * Revision 3.31  2003/05/30 17:42:05  schwarz
 * Added SdifFGetMatrixType and SdifFGetFrameType.
 *
 * Revision 3.30  2003/05/27 16:10:42  schwarz
 * Added SdifFGetMatrixTypesTable and SdifFGetFrameTypesTable.
 *
 * Revision 3.29  2003/05/26 16:12:47  roebel
 * replaced all references to VERSIOn macro by SDIF_VERSION_STRING
 *
 * Revision 3.28  2003/01/16 15:31:16  tisseran
 * Check if host have unistd.h before included it
 *
 * Revision 3.27  2002/11/28 19:57:39  roebel
 * Fixed some const arguments.
 * Make SdifFtruncte return SDIF_FTRUNCATE_NOT_AVAILABLE if
 * this is the case.
 *
 * Revision 3.26  2002/11/27 17:54:46  roebel
 * ftruncate only included if available.
 *
 * Revision 3.25  2002/08/28 17:08:54  roebel
 * Fixed missing include file for function ftruncate.
 *
 * Revision 3.24  2002/08/28 14:07:27  schwarz
 * New function SdifFRewind.
 * Unified return values for SdifFTruncate.
 *
 * Revision 3.23  2002/08/27 10:53:48  schwarz
 * New file truncate function.
 *
 * Revision 3.22  2002/05/24 19:37:52  ftissera
 * Change code to be compatible with C++
 * Cast pointers to correct type.
 *
 * Revision 3.21  2001/05/02 09:34:42  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.20  2001/04/20 14:04:07  tisseran
 * SDIF VERSION 3.3
 * Create function: SdifTakeCodedPredefinedTypesfromString (SdifFile.[hc])
 * This function is use to read types from the define string SDIFTYPES_STRING
 * This string is generated by perl script xmltostyp.pl
 * Types definition are in the sdiftypes.xml file.
 * Change in sdif_portability.h to NOT INCLUDE host_architecture.h when using configure
 * script.
 * project_preinclude.h and configure.in have been changed according to new version number.
 *
 * Revision 3.19  2000/11/15 14:53:28  lefevre
 * no message
 *
 * Revision 3.18  2000/11/14  10:42:26  lefevre
 * no message
 *
 * Revision 3.17  2000/11/09  15:59:15  schwarz
 * Added lost function SdifFStreamIDTable that returns the file's
 * stream ID table.
 *
 * Revision 3.16  2000/10/27 18:55:49  roebel
 * moved identstring to SdifFile again
 * otherwise no identification of linked programs
 *
 * Revision 3.15  2000/10/27  15:04:53  roebel
 * Moved version to automatically generated SDifVersion.h
 *
 * Revision 3.14  2000/08/22  13:17:24  schwarz
 * Centralised error report function SdifFError, called from _SdifFError
 * macro.  Error level names (gSdifErrorLevel) are printed, so that users
 * can tell warnings from errors.  Errors are counted by level in
 * SdifFileT.ErrorCount[].  Error counts can be queried with SdifFNumErrors.
 *
 * Revision 3.13  2000/08/21  10:02:50  tisseran
 * Add information about compilation when use SdifPrintVersion:
 * - Which SdifTypes.STYP is used.
 * - Who made the compilation.
 *
 * Revision 3.12  2000/08/07  15:05:45  schwarz
 * Error checking for read general header.
 * Remove double definition of 1GAI matrix type.
 * 
 * Revision 3.13.2.3  2000/10/25  18:35:56  roebel
 * moved version variable to SdifVersion.h
 *
 * Revision 3.13.2.2  2000/08/21  21:35:15  tisseran
 * *** empty log message ***
 *
 * Revision 3.13.2.1  2000/08/21  14:04:15  tisseran
 * *** empty log message ***
 *
 * Revision 3.11  2000/05/12  14:41:47  schwarz
 * On behalf of Adrien, synchronisation with Mac sources, with some slight
 * changes because of cross-platform issues:
 * - Mac only stuff: XpSetFileAttribute XpFileSize
 * - Cross platform wrapper: XpGetenv XpExit
 * - Dangerous: strings.h (and thus bzero, bcopy) is not ANSI and thus doesn't
 *   exist on Mac.  Use string.h and memset, memcpy.
 *
 * Revision 3.10  2000/05/04  15:05:47  schwarz
 * SDIF Selection is now parsed automatically on opening a file,
 * and placed in file->Selection.
 * Moved SdifCheckFileFormat and SdifSignatureTab functions into
 * separate files SdifCheck and SdifSignatureTab.
 *
 * Revision 3.9  2000/04/26  15:31:23  schwarz
 * Added SdifGenInitCond for conditional initialisation.
 *
 * Revision 3.8  2000/03/01  11:19:36  schwarz
 * Tough check for pipe on open.
 * Added SdifFCurrDataType.
 *
 * Revision 3.7  1999/11/17  16:22:37  schwarz
 * SdifCheckFileFormat now only reads the first 4 bytes.  This avoids a
 * subtle bug if the file was shorter than 16 bytes, as e.g. with f0
 * test files:  The library would bail out on EOF (not a good idea to do
 * that anyway).
 *
 * Revision 3.6  1999/10/13  16:05:45  schwarz
 * Changed data type codes (SdifDataTypeET) to SDIF format version 3, as
 * decided with Matt Wright June 1999, added integer data types.
 * Added writing of 1NVT with real frame header (but data is still not in
 * matrices).
 * The data type handling makes heavy use of code-generating macros,
 * called for all data types with the sdif_foralltypes macro, thus
 * adding new data types is easy.
 *
 * Revision 3.5  1999/10/07  15:12:22  schwarz
 * Added isSeekable flag in SdifFileT struct.  This allows to simplify the
 * many tests for stdio on opening the stream.
 * Added SdifStrEq utility function.
 *
 * Revision 3.4  1999/09/28  13:08:57  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.3  1999/09/28  10:36:59  schwarz
 * Added SdifCheckFileFormat to test if a file is in SDIF.
 *
 * Revision 3.2  1999/09/20  13:21:57  schwarz
 * Introduced user data and access functions SdifFAddUserData/GetUserData.
 *
 * Revision 3.1  1999/03/14  10:56:49  virolle
 * SdifStdErr add
 *
 * Revision 2.5  1999/02/28  12:16:39  virolle
 * memory report
 *
 * Revision 2.4  1999/01/23  15:55:47  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.3  1999/01/23  13:57:32  virolle
 * General Lists, and special chunk preparation to become frames
 *
 * Revision 2.2  1998/12/21  18:27:15  schwarz
 * Inserted copyright message.
 *
 * Revision 2.1  1998/12/09  15:05:25  virolle
 * no segmentation fault when opening failed.
 *
 * Revision 2.0  1998/11/29  11:41:40  virolle
 * - New management of interpretation errors.
 * - Alignement of frames with CNMAT (execpt specials Chunk 1NVT, 1TYP, 1IDS).
 * _ Sdif Header File has a Sdif format version.
 * - Matrices order in frames is not important now. (only one occurence of
 *   a Matrix Type in a Frame Type declaration )
 * - Hard coded predefined types more dynamic management.
 * - Standart streams (stdin, stdout, stderr) set as binary for Windows32 to
 *   have exactly the same result on each plateforme.
 *
 * Revision 1.4  1998/05/14  09:50:34  schwarz
 * Added SdifCurrOneRowData to return a pointer to the raw data.
 * This can subsequently be used for SdifSetCurrOneRow.
 */


#include "sdif_portability.h"
#ifdef USE_XPGUI
#include "XpGuiCalls.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>		/* use for ftruncate */
#endif
#include <string.h>
#include <assert.h>
#include "SdifFScan.h"

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

/* #include <sys/types.h> */
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "SdifGlobals.h"
#include "SdifHard_OS.h"	/* SdifInitMachineType */
#include "SdifFile.h"
#include "SdifTest.h"
#include "SdifSelect.h"
#include "SdifErrMess.h"
#include "SdifFRead.h"		/* for SdifFReadGeneralHeader */

/* Include all Frame Type */
#include "sdiftypes.h"

#if defined( _WIN32) || defined(WIN32)
#include <io.h>
#endif

/* include file version in object code, to be extracted with "ident libsdif.a" */
#ifndef lint
    static char identstring[] = "$File: " __FILE__ " $  $SDIFVersion: " SDIF_VERSION_STRING " $ IRCAM $SDIFcompiled: " __DATE__ " $";
#endif



SdifFileT*
SdifFOpen(const char* Name, SdifFileModeET Mode)
{
  SdifFileT* SdifF;


  assert (gSdifInitialised  &&  "SDIF library not initialised!");

  SdifF = SdifMalloc(SdifFileT);
  if (SdifF)
    {
      /* Check if doing standard i/o or file i/o.  
	 stdio holds the standard i/o type as eBinaryModeStd(Input|Output),
	 eBinaryModeUnknown means file i/o */
      SdifBinaryModeET    stdio;   

      /* Split name and selection */
      SdifF->Selection        = SdifCreateSelection ();
      SdifF->Name             = SdifGetFilenameAndSelection (Name, 
							     SdifF->Selection);

      if (SdifF->Name == NULL)
	  SdifF->Name = "";
      if (SdifF->Name [0] == 0  ||  SdifStrEq (SdifF->Name, "-"))
	  stdio = Mode == eReadFile		    ?  eBinaryModeStdInput
						    :  eBinaryModeStdOutput;
      else
	  stdio = SdifStrEq(SdifF->Name, "stdin")   ?  eBinaryModeStdInput   :
		  SdifStrEq(SdifF->Name, "stdout")  ?  eBinaryModeStdOutput  :
		  SdifStrEq(SdifF->Name, "stderr")  ?  eBinaryModeStdError   : 
						       eBinaryModeUnknown;

      SdifF->Mode             = Mode;
      SdifF->FormatVersion    = _SdifFormatVersion; /* default */
      SdifF->TypesVersion     = _SdifTypesVersion;  /* default */
      
      SdifF->NameValues       = SdifCreateNameValuesL (_SdifNameValueHashSize);
      SdifF->MatrixTypesTable = SdifCreateHashTable   (_SdifGenHashSize, 
						       eHashInt4, 
						       (void (*)(void *))SdifKillMatrixType);
      SdifF->FrameTypesTable  = SdifCreateHashTable   (_SdifGenHashSize, 
						       eHashInt4,
						       (void (*)(void *))SdifKillFrameType);
/*      SdifF->StreamIDsTable   = SdifCreateHashTable(1, eHashInt4, SdifKillStreamID);*/
      SdifF->StreamIDsTable   = SdifCreateStreamIDTable(1);
      SdifF->TimePositions    = SdifCreateTimePositionL();

      SdifF->CurrSignature = eEmptySignature;
      SdifF->CurrFramH     = NULL;
      SdifF->CurrMtrxH     = NULL;
      SdifF->CurrFramT     = NULL;
      SdifF->CurrMtrxT     = NULL;
      SdifF->PrevTime      = _Sdif_MIN_DOUBLE_;
      SdifF->MtrxUsed      = SdifCreateSignatureTab (_SdifSignatureTabGranule);
      /*SdifF->MtrxUsed      = SdifCreateSignatureTab(1);*/
      SdifF->CurrFramPos   = 0;
      SdifF->FileSize      = 0;
      SdifF->ChunkSize     = 0;
      SdifF->StartChunkPos = 0;
      SdifF->Pos           = 0;
      SdifF->TypeDefPass   = eNotPass;
      SdifF->StreamIDPass  = eNotPass;
 
      /* One _SdifGranule allocated --> 256 floats */
      SdifF->CurrOneRow    = SdifCreateOneRow(eFloat4, 1);
      SdifF->CurrMtrxData  = SdifCalloc(SdifMatrixDataT, 1);

      SdifF->TextStreamName   = NULL;
      SdifF->TextStream       = NULL;
      SdifF->Stream           = NULL;
 
      SdifF->Errors		   = SdifCreateErrorL(SdifF);
      SdifF->ErrorCount	[eFatal]   = 0;
      SdifF->ErrorCount	[eError]   = 0;
      SdifF->ErrorCount	[eWarning] = 0;
      SdifF->ErrorCount	[eRemark]  = 0;
      SdifF->ErrorCount	[eNoLevel] = 0;

      SdifF->NbUserData    = 0;
      

      switch (Mode)
      {
          case eReadFile:
	      switch (stdio)
	      {
	          case eBinaryModeStdInput:
		      SdifF->Stream = SdiffBinOpen (SdifF->Name, 
						    eBinaryModeStdInput);
		  break;

		  case eBinaryModeUnknown:
		      SdifF->Stream = SdiffBinOpen (SdifF->Name, 
						    eBinaryModeRead);
		  break;

	          default:
		      _SdifFError(SdifF, eBadStdFile, SdifF->Name);
		  break;
	      }
	  break;

          case eWriteFile :
	      switch (stdio)
	      {
	          case eBinaryModeStdOutput:
		      SdifF->Stream = SdiffBinOpen (SdifF->Name, 
						    eBinaryModeStdOutput);
		  break;

		  case eBinaryModeUnknown:
		      SdifF->Stream = SdiffBinOpen (SdifF->Name, 
						    eBinaryModeWrite);
		  break;

	          default:
		      _SdifFError(SdifF, eBadStdFile, SdifF->Name);
		  break;
              }
	  break;

          case eReadWriteFile:
	      switch (stdio)
	      {
		  case eBinaryModeUnknown:
		      SdifF->Stream = SdiffBinOpen (SdifF->Name, 
						    eBinaryModeReadWrite);
		  break;

	          default:
		      _SdifFError(SdifF, eBadStdFile, SdifF->Name);
		  break;
	      }
	  break;

          case ePredefinedTypes: /* special case:		  */
	      return SdifF;	 /* stream is NULL, but that's ok */

          default:
	      _SdifFError(SdifF, eBadMode, "this mode doesn't exist");
      }

      if (!SdifF->Stream)
      {
	  _SdifError(eFileNotFound, Name);
	  SdifFClose (SdifF);
	  SdifF = NULL;
      }
      else
      {   
        /* check if we can perform seeks (with Sdiffsetpos) on this file */
#if HAVE_SYS_STAT_H && ! defined( _MSC_VER)
        struct stat sb;
        if (fstat (fileno (SdifF->Stream), &sb) == -1)
          {	
            /* Default to maximum safety. */
            SdifF->isSeekable = 0 ;
          }
        else
#ifndef __MINGW32__
	  SdifF->isSeekable = !((S_ISFIFO (sb.st_mode) || S_ISSOCK (sb.st_mode)));
#else
          SdifF->isSeekable = !(S_ISFIFO (sb.st_mode));
#endif
#else
        SdifF->isSeekable  =  stdio == eBinaryModeUnknown;
#endif
      }
  }
  else
  {
      _SdifError(eAllocFail, "SdifFile");
  }
      
  return SdifF;
}





SdifFileT*
SdifFOpenText(SdifFileT *SdifF, const char* Name, SdifFileModeET Mode)
{
  SdifF->TextStreamName = SdifCreateStrNCpy(Name, SdifStrLen(Name)+1);

  switch (Mode)
  {
	case eReadFile :
      if (SdifStrEq(Name, "stdin"))
	  {
	    SdifF->TextStream = SdiffBinOpen (Name, eBinaryModeStdInput);
	    return SdifF;
	  }
	  else
	  {
	    if (    (SdifStrEq(Name, "stdout"))
			 || (SdifStrEq(Name, "stderr"))   )
		{
		  _SdifFError(SdifF, eBadStdFile, Name);
		  return NULL;
	    }
	    else
		{
	      SdifF->TextStream = SdiffBinOpen (Name, eBinaryModeRead);
	      if (! SdifF->TextStream)
		  {
		    _SdifError(eFileNotFound, Name);
		    return NULL;
		  }
	      else
		  {
		    return SdifF;
		  }
		}
	  }

	case eWriteFile :
	  if (SdifStrEq(Name, "stdout"))
	  {
	    SdifF->TextStream = SdiffBinOpen (Name, eBinaryModeStdOutput);
	    return SdifF;
	  }
	  else
	  {
        if (SdifStrEq(Name, "stderr"))
		{
	      SdifF->TextStream = SdiffBinOpen (Name, eBinaryModeStdError);
	      return SdifF;
		}
		else
		{
	      if (SdifStrEq(Name, "stdin"))
		  {
		    _SdifFError(SdifF, eBadStdFile, Name);
		    return NULL;
		  }
	      else
		  {
		    SdifF->TextStream = SdiffBinOpen (Name, eBinaryModeWrite);
		    if (! SdifF->TextStream)
		      {
			_SdifError(eAllocFail, Name);	  
			return NULL;
		      }
		    else
		      {
			return SdifF;
		      }
		  }
		}
	  }
	case ePredefinedTypes:
      if (SdifStrEq(Name, "stdin"))
	  {
	    SdifF->TextStream = SdiffBinOpen (Name, eBinaryModeStdInput);
	    return SdifF;
	  }
	  else
	  {
	    if (    (SdifStrEq(Name, "stdout"))
			 || (SdifStrEq(Name, "stderr"))   )
		{
		  _SdifFError(SdifF, eBadStdFile, Name);
		  return NULL;
	    }
	    else
		{
	      SdifF->TextStream = SdiffBinOpen (Name, eBinaryModeRead);
	      if (! SdifF->TextStream)
		  {
		    _SdifError(eFileNotFound, Name);
		    return NULL;
		  }
	      else
		  {
		    return SdifF;
		  }
		}
	  }
	default :
	  _SdifFError(SdifF, eBadMode, "this mode doesn't exist or isn't appropriated");
	  return NULL;
	}
}




void
SdifFClose(SdifFileT* SdifF)
{
    if (SdifF)
    {
	/* name is now part of the selection and freed with it.
	   if (SdifF->Name)          SdifFree (SdifF->Name);
	   else                     _SdifError (eFreeNull, "SdifFile->Name");
	*/
      if (SdifF->NameValues)         SdifKillNameValuesL (SdifF->NameValues);
        else                         _SdifError (eFreeNull, "SdifFile->NameValues");
      if (SdifF->MatrixTypesTable)   SdifKillHashTable (SdifF->MatrixTypesTable);
        else                         _SdifError (eFreeNull, "SdifFile->MatrixTypesTable");
      if (SdifF->FrameTypesTable)    SdifKillHashTable (SdifF->FrameTypesTable);
        else                         _SdifError (eFreeNull, "SdifFile->FrameTypesTable");
/*    if (SdifF->StreamIDsTable)     SdifKillHashTable (SdifF->StreamIDsTable);
        else                         _SdifError (eFreeNull, "SdifFile->StreamIDsTable");*/
      if (SdifF->StreamIDsTable)     SdifKillStreamIDTable (SdifF->StreamIDsTable);
        else                         _SdifError (eFreeNull, "SdifFile->StreamIDsTable");
      if (SdifF->TimePositions)      SdifKillTimePositionL (SdifF->TimePositions);
        else                         _SdifError (eFreeNull, "SdifFile->TimePositions");
      if (SdifF->Selection)          SdifFreeSelection (SdifF->Selection);
        else                         _SdifError (eFreeNull, "SdifFile->Selection");
      if (SdifF->CurrOneRow)         SdifKillOneRow(SdifF->CurrOneRow);
        else                         _SdifError (eFreeNull, "SdifFile->CurrOneRow");
      if (SdifF->CurrMtrxData)       SdifKillMatrixData(SdifF->CurrMtrxData);
        else                         _SdifError (eFreeNull, "SdifFile->CurrMtrxData");
      if (SdifF->Errors)             SdifKillErrorL(SdifF->Errors);
        else                         _SdifError (eFreeNull, "SdifFile->Errors");


      if (SdifF->CurrFramH)
        SdifKillFrameHeader(SdifF->CurrFramH);
      if (SdifF->CurrMtrxH)
        SdifKillMatrixHeader(SdifF->CurrMtrxH);
      if (SdifF->MtrxUsed)
	SdifKillSignatureTab(SdifF->MtrxUsed);

      if (SdifF->TextStreamName)
        SdifFree(SdifF->TextStreamName);


      if (SdifF->Stream)
		if (    (SdifF->Stream != stdout)
			 && (SdifF->Stream != stdin)
			 && (SdifF->Stream != stderr)  )
          SdiffBinClose (SdifF->Stream);


      if (SdifF->TextStream)
		if (    (SdifF->TextStream != stdout)
			 && (SdifF->TextStream != stdin)
			 && (SdifF->TextStream != stderr)  )
          SdiffBinClose(SdifF->TextStream);

      SdifFree(SdifF);
    }
}




SdifFrameHeaderT*
SdifFCreateCurrFramH(SdifFileT* SdifF, SdifSignature Signature)
{
  if (! (SdifF->CurrFramH))
    SdifF->CurrFramH = SdifCreateFrameHeaderEmpty(Signature);
  else
    SdifF->CurrFramH->Signature = Signature;

  return SdifF->CurrFramH;
}





SdifMatrixHeaderT*
SdifFCreateCurrMtrxH(SdifFileT* SdifF)
{
  if (! (SdifF->CurrMtrxH))
    SdifF->CurrMtrxH = SdifCreateMatrixHeaderEmpty();

  return SdifF->CurrMtrxH;
}










FILE*
SdifFGetFILE_SwitchVerbose(SdifFileT* SdifF, int Verbose)
{
  switch (Verbose)
  {
    case 't' :
      return SdifF->TextStream;
    case 's' :
      return SdifF->Stream;
    default :
    {
	char errorMess[_SdifStringLen];

	sprintf(errorMess, "*Sdif* %c not a verbose ('t': text; 's':SdifFile)\n", Verbose);
	_Debug(errorMess);
	return NULL;
    }
  }
}









void
SdifTakeCodedPredefinedTypes(SdifFileT *SdifF)
{
  SdifCreatePredefinedTypes(SdifF->MatrixTypesTable, SdifF->FrameTypesTable);
}


void 
SdifTakeCodedPredefinedTypesfromString(SdifFileT *SdifF)
{
    size_t SizeR = 0;
    char *typesDefinition;
    int result;

    SdifStringT *SdifString;
    
    SdifString = SdifStringNew();

    typesDefinition = SDIFTYPES_STRING;
    result = SdifStringAppend(SdifString, typesDefinition);

    if (result == 0)
    {
	fprintf(stderr, "CANNOT APPEND THE TYPE DEFINITION STRING !!!! \n");
    }
    SizeR += SdifFGetAllTypefromSdifString(SdifF, SdifString);

    SdifStringFree(SdifString);
}





/* TextStream becomes temporaly the file whih contains predefined types.
 */
void
SdifFLoadPredefinedTypes(SdifFileT *SdifF, const char *TypesFileName)
{
  if (SdifStrEq(TypesFileName, ""))
    {

      /* No types requested so do not emit warning! */
#if 0
      _SdifRemark("Load Coded Predefinied Types, it can be incomplete (file name null)\n");
#endif
      /*
	Old version (before version SDIF 3.3)
	SdifTakeCodedPredefinedTypes(SdifF);
      */
      /* Call */
      SdifTakeCodedPredefinedTypesfromString(SdifF);
      /* */
    }
  else
    {
      SdifFOpenText(SdifF, TypesFileName, ePredefinedTypes);
      if (! SdifF->TextStream)
        {
          _SdifRemark("Load Coded Predefinied Types, it can be incomplete (file not found)\n");
          /*
	    Old version (before version SDIF 3.3)
	    SdifTakeCodedPredefinedTypes(SdifF);
	  */
	  /* Call  */
	  SdifTakeCodedPredefinedTypesfromString(SdifF);
	  /* */
        }
      else
        {
            SdifFScanGeneralHeader   (SdifF);
            SdifFScanAllASCIIChunks  (SdifF);
        }
    }
}


/* Get table of matrix type definitions, 
   useful for SdifGetMatrixType.  */
SdifHashTableT *SdifFGetMatrixTypesTable(SdifFileT *file)
{
    return (file->MatrixTypesTable);
}


/* Get table of frame type definitions, 
   useful for SdifGetFrameType.  */
SdifHashTableT *SdifFGetFrameTypesTable(SdifFileT *file)
{
    return (file->FrameTypesTable);
}

/* Get table of matrix type definitions, 
   useful for SdifGetMatrixType.  */
SdifMatrixTypeT *SdifFGetMatrixType(SdifFileT *file, SdifSignature sig)
{
    return (SdifGetMatrixType(file->MatrixTypesTable, sig));
}


/* Get table of frame type definitions, 
 * useful for SdifGetFrameType.  */
SdifFrameTypeT* SdifFGetFrameType(SdifFileT *file, SdifSignature sig)
{
    return (SdifGetFrameType(file->FrameTypesTable, sig));
}





int        gSdifInitialised      = 0;
SdifFileT *gSdifPredefinedTypes  = 0;

/* _SdifTypesFileName is normaly defined
 * in the Makefile with -D_SdifTypesFileName="<FileNameWithPath>"
 * then default _SdifTypesFileName is not used.
 */
#ifndef _SdifTypesFileName
#define _SdifTypesFileName  "SdifTypes.STYP"
#endif

/* Attention: Handling of the warning for sdiftypes file changed.
 * Warning will be given only if sdiftypes file has been specified by
 * means of environment variable.
 * A non existing _SdifTypesFileName will no longer be communicated to the user.
 */
void
SdifGenInit(const char *PredefinedTypesFile)
{
    char *PreTypesEnvVar=NULL;
    char *local_types = NULL;
    int   use_default_file =0;

    assert (!gSdifInitialised  &&  "SDIF library already initialised");
    gSdifInitialised = 1;
    
#ifdef HAVE_PTHREAD    
    SdifCreateThreadKeys();
#endif

    if (SdifStdErr == NULL)
	SdifStdErr = stderr;

    SdifInitMachineType();
    SdifSetStdIOBinary (); /* only WIN32 */
    SdifInitListNodeStock(_SdifListNodeStockSize);
    SdifInitSelect ();

    gSdifPredefinedTypes = SdifFOpen("Predefined", ePredefinedTypes);

    if (!PredefinedTypesFile  ||  strlen(PredefinedTypesFile) == 0)
    {
	char * copy;

#ifdef HAVE_GETENV
	PreTypesEnvVar = getenv(_SdifEnvVar);
#endif
	if (!PreTypesEnvVar)
	{
	    PreTypesEnvVar   = _SdifTypesFileName;
	    use_default_file = 1;
	}

#ifdef _MSC_VER
	copy        = _strdup(PreTypesEnvVar);
	local_types = _strdup(SdifBaseName(copy));
#else
	copy        = strdup(PreTypesEnvVar);
	local_types = strdup(SdifBaseName(copy));
#endif

#if HAVE_SYS_STAT_H
	{   /* stat files */
	    struct stat sb;
	    int	        rs;
	   	   
	    /* Give priority to any local type file */
	    rs = stat(local_types, &sb);	   

	    if (rs == 0) 
	    {
		PreTypesEnvVar = local_types; 
	    }
	    else 
	    { 
		/* check existence of global types file. Warning will
		   be given only if it is specified by means of
		   environment variable SDIFTYPES.  In this case, if
		   not available do not request to load it to prevent
		   warnings. */
	       
		if (use_default_file) 
		{
		    rs = stat(PreTypesEnvVar, &sb);
		     
		    if (rs != 0)
			PreTypesEnvVar = "";
		}
	    }
	}
#else
	{   /* try to open files */
	    FILE *rf = fopen(local_types, "r");

	    if (rf != NULL) 
	    {
		PreTypesEnvVar = local_types; 
		fclose(rf);
	    }
	    else 
	    {   /* check existence of global types file. Warning will be
		   given only if it is specified by means of environment
		   variable SDIFTYPES.  if not available do not request
		   to load it to prevent warnings. */
	     
		if (use_default_file) 
		{
		    rf = fopen(PreTypesEnvVar, "r");
		    if (rf == NULL)
			PreTypesEnvVar = "";
		    else
			fclose(rf);
		}
	    }
	}
#endif
	SdifFLoadPredefinedTypes(gSdifPredefinedTypes, PreTypesEnvVar);

	free(copy);
	free(local_types);
    }
    else
    {
        SdifFLoadPredefinedTypes(gSdifPredefinedTypes, PredefinedTypesFile);
    }
}


void 
SdifGenInitCond (const char *pfile)
{
    if (!gSdifInitialised)
        SdifGenInit (pfile);
}




void
SdifGenKill(void)
{

  if( ! gSdifInitialised ) return  ;

  SdifFClose(gSdifPredefinedTypes);
  SdifDrainListNodeStock();

#ifdef _SdifMemoryReport
  SdifMrDrainBlockList(&SdifMrReport);
#endif

  gSdifInitialised = 0;
}



void SdifPrintVersion(void)
{
    if (SdifStdErr == NULL)
	SdifStdErr = stderr;

    fprintf(SdifStdErr, "SDIF Library\n");
    fprintf(SdifStdErr, "Format version : %d\n", _SdifFormatVersion);

    fprintf(SdifStdErr, "Release: %s, %s\n", SDIF_VERSION_STRING, __DATE__);
}




SdifFrameHeaderT*
SdifFSetCurrFrameHeader(SdifFileT     *SdifF,
		       SdifSignature Signature,
		       SdifUInt4     Size,
		       SdifUInt4     NbMatrix,
		       SdifUInt4     NumID,
		       SdifFloat8    Time)
{
  SdifFCreateCurrFramH(SdifF, Signature);
  SdifF->CurrFramH->Size      = Size;
  SdifF->CurrFramH->NbMatrix  = NbMatrix;
  SdifF->CurrFramH->NumID     = NumID;
  SdifF->CurrFramH->Time      = Time;
  return SdifF->CurrFramH;
}





SdifMatrixHeaderT*
SdifFSetCurrMatrixHeader(SdifFileT     *SdifF,
			SdifSignature  Signature,
			SdifDataTypeET DataType,
			SdifUInt4      NbRow,
			SdifUInt4      NbCol)
{
  SdifFCreateCurrMtrxH(SdifF);
  SdifF->CurrMtrxH->Signature = Signature;
  SdifF->CurrMtrxH->DataType  = DataType;
  SdifF->CurrMtrxH->NbRow     = NbRow;
  SdifF->CurrMtrxH->NbCol     = NbCol;

  SdifReInitOneRow(SdifF->CurrOneRow, DataType, NbCol);

  return SdifF->CurrMtrxH;
}







SdifOneRowT*
SdifFSetCurrOneRow(SdifFileT *SdifF, void *Values)
{
#if (_SdifFormatVersion > 2)

    memcpy (SdifF->CurrOneRow->Data.Void, (void *) Values, 
	    SdifSizeofDataType (SdifF->CurrOneRow->DataType) * SdifF->CurrOneRow->NbData);

#else

  switch (SdifF->CurrOneRow->DataType)
    {
    case eFloat4:
      memcpy(SdifF->CurrOneRow->Data.Float4, (char*) Values, sizeof(SdifFloat4)* SdifF->CurrOneRow->NbData);
      break;
    case eFloat8:
      memcpy(SdifF->CurrOneRow->Data.Float8, (char*) Values, sizeof(SdifFloat8)* SdifF->CurrOneRow->NbData);
      break;
    default:
      return NULL;
    }
#endif

  return SdifF->CurrOneRow;
}


SdifOneRowT*
SdifFSetCurrOneRowCol(SdifFileT *SdifF, SdifUInt4 numCol, SdifFloat8 Value)
{
  return SdifOneRowPutValue(SdifF->CurrOneRow, numCol, Value);
}



SdifFloat8
SdifFCurrOneRowCol(SdifFileT *SdifF, SdifUInt4 numCol)
{
  return SdifOneRowGetValue(SdifF->CurrOneRow, numCol);
}

SdifFloat8
SdifFCurrOneRowColName(SdifFileT *SdifF, SdifMatrixTypeT *MatrixType, const char *NameCD)
{
  return SdifOneRowGetValue(SdifF->CurrOneRow,
			    SdifMatrixTypeGetNumColumnDef(MatrixType, NameCD));
}

SdifSignature
SdifFCurrSignature(SdifFileT *SdifF)
{
  return SdifF->CurrSignature;
}

SdifSignature
SdifFCleanCurrSignature(SdifFileT *SdifF)
{
  SdifF->CurrSignature = eEmptySignature;
  return SdifF->CurrSignature;
}

SdifSignature
SdifFCurrFrameSignature(SdifFileT *SdifF)
{
  return SdifF->CurrFramH->Signature;
}

SdifSignature
SdifFCurrMatrixSignature(SdifFileT *SdifF)
{
  return SdifF->CurrMtrxH->Signature;
}

/* Renvoie la ligne temporaire de SdifF.  */
SdifOneRowT*
SdifFCurrOneRow(SdifFileT *SdifF)
{
  return SdifF->CurrOneRow;
}


/*  DS: Added SdifCurrOneRowData to return a pointer to the raw data.
    This can subsequently be used for SdifSetCurrOneRow.
 */
void*
SdifFCurrOneRowData(SdifFileT *SdifF)
{
#if (_SdifFormatVersion >= 3)
    return SdifF->CurrOneRow->Data.Void;
#else
  switch (SdifF->CurrOneRow->DataType)
    {
    case eFloat4:
      return SdifF->CurrOneRow->Data.Float4;
    case eFloat8:
      return SdifF->CurrOneRow->Data.Float8;
    default:
      return NULL;
    }
#endif
}


/* return pointer to current matrix data structure */
SdifMatrixDataT *SdifFCurrMatrixData (SdifFileT *file)
{
  return file->CurrMtrxData;
}


/* return pointer to current raw matrix data, 
   specified by current matrix header */
void *SdifFCurrMatrixDataPointer (SdifFileT *file)
{
  return file->CurrMtrxData->Data.Void;
}


SdifUInt4
SdifFCurrNbCol(SdifFileT *SdifF)
{
  return SdifF->CurrMtrxH->NbCol;
}

SdifUInt4
SdifFCurrNbRow(SdifFileT *SdifF)
{
  return SdifF->CurrMtrxH->NbRow;
}

SdifDataTypeET
SdifFCurrDataType (SdifFileT *SdifF)
{
  return SdifF->CurrMtrxH->DataType;
}

SdifUInt4
SdifFCurrNbMatrix(SdifFileT *SdifF)
{
  return SdifF->CurrFramH->NbMatrix;
}

SdifUInt4
SdifFCurrID(SdifFileT *SdifF)
{
  return SdifF->CurrFramH->NumID;
}

SdifFloat8
SdifFCurrTime(SdifFileT *SdifF)
{
  return SdifF->CurrFramH->Time;
}




SdifFileT*
SdifFReInitMtrxUsed (SdifFileT *SdifF)
{
  SdifReInitSignatureTab(SdifF->MtrxUsed, SdifF->CurrFramH->NbMatrix);
  return SdifF; 
}

SdifSignature
SdifFIsInMtrxUsed (SdifFileT *SdifF, SdifSignature Sign)
{
  return SdifIsInSignatureTab (SdifF->MtrxUsed, Sign);
}

SdifFileT*
SdifFPutInMtrxUsed (SdifFileT *SdifF, SdifSignature Sign)
{
  SdifPutInSignatureTab (SdifF->MtrxUsed,  Sign);
  return SdifF; 
}


/* Error management */
int SdifFNumErrors (SdifFileT *f, SdifErrorLevelET upto)
{
    int ret = 0;
    switch (upto)	/* switch is much faster than loop */
    {	/* sum no. of errors of level upto or more severe */
    	case eNoLevel:	ret += f->ErrorCount [eNoLevel]; /* fallthrough! */
    	case eRemark:	ret += f->ErrorCount [eRemark];  /* fallthrough! */
    	case eWarning:	ret += f->ErrorCount [eWarning]; /* fallthrough! */
    	case eError:	ret += f->ErrorCount [eError];   /* fallthrough! */
    	case eFatal:	ret += f->ErrorCount [eFatal];
    	break;
    
    	default:	assert (!"illegal argument for SdifErrorLevelET");
    }
    return (ret);
}


/* Return pointer to last error of file or NULL if there are no errors
   present.  */
SdifErrorT*
SdifFLastError (SdifFileT *SdifF)
{
  return SdifLastError(SdifF->Errors);
}



SdifErrorTagET
SdifFLastErrorTag (SdifFileT *SdifF)
{
  return SdifLastErrorTag(SdifF->Errors);
}



/* Return list of NVTs for querying. */
SdifNameValuesLT *
SdifFNameValueList (SdifFileT *file)
{
    return (file->NameValues);
}

/* Return number of NVTs present. */
int SdifFNameValueNum (SdifFileT *file)
{
    return (SdifListGetNbData (file->NameValues->NVTList));
}


SdifStreamIDTableT *SdifFStreamIDTable (SdifFileT *file)
{
    return (file->StreamIDsTable);
}


/* Add user data, return index added */
int
SdifFAddUserData (SdifFileT *file, void *data)
{
    assert (file->NbUserData < MaxUserData);
    file->UserData [file->NbUserData] = data;
    return (file->NbUserData++);
}

/* Get user data by index */
void *
SdifFGetUserData (SdifFileT *file, int index)
{
    if (index >= 0  &&  index < file->NbUserData)
	return (file->UserData [index]);
    else
	return (NULL);
}

/* Rewind to start of file (before header!) */
int SdifFRewind(SdifFileT *file)
{
    SdiffPosT zero = 0;
    return (SdiffSetPos(file->Stream, &zero) == 0);
}


/* Truncate file at current position */
int SdifFTruncate(SdifFileT *file)
{
  SdiffPosT pos = 0;
  SdiffGetPos(file->Stream, &pos);
#ifdef HAVE_FTRUNCATE
  return (ftruncate(fileno(file->Stream), pos) == 0);
#else
#  if defined( _WIN32) || defined(WIN32)
#    if defined(_MSC_VER) && _MSC_VER >= 1400
  // 64bit pos
  return (_chsize_s(_fileno(file->Stream), pos) == 0);
#    else
  // 32bit pos
  return (_chsize(_fileno(file->Stream), pos) == 0);
#    endif 
#  else
  return SDIFFTRUNCATE_NOT_AVAILABLE;
#  endif
#endif

}



/* Get position in file.
   [return] file offset or -1 for error.
   SdiffPosT is a plattform dependent type that should correctly work with long files. 
 */
int SdifFGetPos(SdifFileT *file, SdiffPosT *pos)
{
    return SdiffGetPos(file->Stream, pos);
}



/* Set absolute position in file.
   SdiffPosT is a plattform dependent type that should correctly work with long files. 
   [Return] 0 on success, 
	   -1 on error (errno is set, see fseek(3) for details)

	   On Windows, seeking on a stream is always considered
	   successful (return 0), even if no seek was done!
 */
int SdifFSetPos(SdifFileT *file, SdiffPosT *pos)
{
    return SdiffSetPos(file->Stream, pos);
}

