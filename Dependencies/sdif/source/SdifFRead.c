/* $Id: SdifFRead.c,v 3.33 2007-12-10 10:45:43 roebel Exp $
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
 * F : SdifFileT* SdifF, Read : binary read (SdifF->Stream)
 *
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.32  2006/03/22 22:00:25  roebel
 * use correct format specifier for long int arguments.
 *
 * Revision 3.31  2005/10/21 14:32:29  schwarz
 * protect all static buffers from overflow by using snprintf instead of sprintf
 * move big errorMess buffers into error branch to avoid too large stack allocation
 *
 * Revision 3.30  2005/10/20 16:50:26  schwarz
 * protect all static buffers from overflow by using (v)snprintf instead of (v)sprintf
 * move big buffers into error branch to avoid too large stack allocation
 * check for error before reading padding
 *
 * Revision 3.29  2005/05/24 09:35:18  roebel
 *
 * Fixed last checkin comment which turned out to be the start of
 * a c-comment.
 *
 * Revision 3.28  2005/05/23 19:17:53  schwarz
 * - Sdiffread/Sdiffwrite functions with SdifFileT instead of FILE *
 *   -> eof error reporting makes more sense
 * - more cleanup of sdif.h, above functions are private in SdifRWLowLevel.h
 * - eEof becomes error 4 to be distinguishable from ascii chars
 * - SdifFScanNameValueLCurrNVT reimplemented for ascii only
 *
 * Revision 3.27  2005/05/23 17:52:53  schwarz
 * Unified error handling:
 * - SdifErrorEnum (global errors) integrated into SdifErrorTagET (file errors)
 * - no more SdifError.[ch], everything done by SdifErrMess.[ch]
 *
 * Revision 3.26  2005/05/20 21:13:24  roebel
 * back to returning 0 in case of file read error.
 * Skipping a matrix can never produce a read size of 0
 * so -1 is unnecessary here.
 *  SdifFSkip now uses seekability flag to determine
 * whether to seek or to read.
 * Don't confuse error tags and error enums, which does not compile
 * with c++.
 *
 * Revision 3.25  2005/05/13 15:19:24  schwarz
 * pass read errors to caller as -1 return values
 *
 * Revision 3.24  2005/04/07 15:56:47  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.23  2004/09/14 15:45:33  schwarz
 * use return after severe _SdifFError, because some library caller might not
 * exit in its error handler
 *
 * Revision 3.22  2004/09/10 09:15:56  roebel
 * Added missing prototype for SdifMatrixDataUpdateHeader. This is necessary to compile with c++.
 *
 * Revision 3.21  2004/09/09 17:47:53  schwarz
 * SdifFReadMatrixData allows to read and store a whole matrix's data as
 * one block in an SdifMatrixDataT, with automatic reallocation.
 *
 * Revision 3.20  2004/07/22 14:47:56  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.19  2004/06/03 11:18:00  schwarz
 * Profiling showed some waste of cycles in byte swapping and signature reading:
 * - byte swapping now array-wise, not element-wise in SdifSwap<N>[Copy] routines:   -> from 0.24 s (18.5%) to 0.14s
 * - ASCII signature reading function SdiffGetSignature replaced by new binary
 *   function SdiffReadSignature (also in SdifFGetSignature, so the change is
 *   mostly transparent):
 *   -> from 0.11 s (9.6%)  to 0.01 s
 * - overall run time improvement with test case sdifextractall_a01:
 *   -> from 1.20 s         to 0.86 s (40% faster)
 *
 * Revision 3.18  2004/05/03 18:07:27  schwarz
 * Fixed bugs in padding calculation for ascii chunks:
 * 1. DON'T PAD FRAMES!
 * 2. SdifFReadMatrixHeader already accounts for read signature
 * Now, calculating padding from ftell is redundant, but we leave it in,
 * with a warning, until everyone's code is tested.
 *
 * Revision 3.17  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.16  2003/08/06 15:13:14  schwarz
 * New functions SdifFSkip, SdifFSkipOneRow.
 * Finally removed obsolete functions (like SdifSkip...).
 *
 * Revision 3.15  2002/05/24 19:37:52  ftissera
 * Change code to be compatible with C++
 * Cast pointers to correct type.
 *
 * Revision 3.14  2001/05/02 09:34:42  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.13  2000/11/15 14:53:27  lefevre
 * no message
 *
 * Revision 3.12  2000/10/27  20:03:29  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.11  2000/08/22  13:38:22  schwarz
 * SdifSkip... renamed to SdifFSkip... to follow nomenclature.
 *
 * Revision 3.10  2000/08/07  15:05:45  schwarz
 * Error checking for read general header.
 * Remove double definition of 1GAI matrix type.
 * 
 * Revision 3.10.2.3  2000/08/21  21:35:09  tisseran
 * *** empty log message ***
 *
 * Revision 3.10.2.2  2000/08/21  18:34:09  tisseran
 * Add SdifSkipASCIIUntilfromSdifString function (same as SdifSkipASCIIUntil).
 * Add SdifFSkip for SdifSkip for (functions SdifSkip doesn't respect function nomemclature => obsolete).
 *
 * Revision 3.10.2.1  2000/08/21  14:04:11  tisseran
 * *** empty log message ***
 *
 * Revision 3.9  2000/07/18  15:08:32  tisseran
 * This release implements the New SDIF Specification (june 1999):
 * - Name Values Table are written in a 1NVT frame which contains a 1NVT matrix
 * - Frame and matrix type declaration are written in a 1TYP frame which contains a 1TYP matrix.
 * - Stream ID are written in a 1IDS frame which contains a 1IDS matrix.
 *
 * Read function accept the previous version of the specification (read a text frame without matrix) to be compatible with older SDIF files.
 *
 * SdifString.h and SdifString.c implements some string mangement (creation, destruction, append, test of end of string, getc, ungetc).
 *
 * WATCH OUT:
 *      We don't care about the old SDIF Specification (_SdifFormatVersion < 3)
 * To use _SdifFormatVersion < 3, get the previous release.
 *
 * Revision 3.8  2000/05/12  14:41:46  schwarz
 * On behalf of Adrien, synchronisation with Mac sources, with some slight
 * changes because of cross-platform issues:
 * - Mac only stuff: XpSetFileAttribute XpFileSize
 * - Cross platform wrapper: XpGetenv XpExit
 * - Dangerous: strings.h (and thus bzero, bcopy) is not ANSI and thus doesn't
 *   exist on Mac.  Use string.h and memset, memcpy.
 *
 * Revision 3.7  2000/04/11  14:31:41  schwarz
 * Read/write NVT as frame with 1 text matrix, conforming to SDIF spec.
 * Read rest of general header chunk (might contain additional data).
 *
 * Revision 3.6  2000/03/01  11:19:57  schwarz
 * Assert Padding, added SdifFReadAndIgnore.
 * SdiffSetPos checks for pipe and then uses SdifFReadAndIgnore to seek forward.
 *
 * Revision 3.5  1999/11/03  16:42:32  schwarz
 * Use _SdifNVTStreamID for stream ID of 1NVT frames because of CNMAT
 * restriction of only one frame type per stream.
 * (See SdifNameValuesLNewTable)
 *
 * Revision 3.4  1999/10/15  12:28:43  schwarz
 * Updated writing of types and stream-id chunks to frames.
 * No time parameter for name value tables and stream ID tables, since
 * this decision is better left to the library.  (It uses the _SdifNoTime
 * constant, which happens to be _Sdif_MIN_DOUBLE_.)
 *
 * Revision 3.3  1999/10/13  16:05:41  schwarz
 * Changed data type codes (SdifDataTypeET) to SDIF format version 3, as
 * decided with Matt Wright June 1999, added integer data types.
 * Added writing of 1NVT with real frame header (but data is still not in
 * matrices).
 * The data type handling makes heavy use of code-generating macros,
 * called for all data types with the sdif_foralltypes macro, thus
 * adding new data types is easy.
 *
 * Revision 3.2  1999/09/28  13:08:54  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.1  1999/03/14  10:56:43  virolle
 * SdifStdErr add
 *
 * Revision 2.3  1999/01/23  15:55:43  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:27  virolle
 * General Lists, and special chunk preparation to become frames
 */


#include "sdif_portability.h"

#include "sdif.h"
#include "SdifGlobals.h"
#include "SdifRWLowLevel.h"
#include "SdifFRead.h"
#include "SdifTest.h"
#include "SdifFile.h"
#include "SdifGlobals.h"
#include "SdifHash.h"
#include "SdifMatrix.h"
#include "SdifFrameType.h"
#include "SdifStreamID.h"
#include "SdifErrMess.h"
#include "SdifFGet.h"
#include <assert.h>
#include <errno.h>




size_t
SdifFReadChunkSize(SdifFileT *SdifF)
{
  size_t SizeR = 0;
  SdifInt4 ChunkSizeInt4 = 0;

  SizeR += sizeof(SdifInt4) * SdiffReadInt4(&(ChunkSizeInt4), 1, SdifF);
  SdifF->ChunkSize = (size_t) ChunkSizeInt4;
  return SizeR;
}


size_t 
SdifFReadGeneralHeader(SdifFileT *SdifF)
{
  size_t SizeR = 0;
  size_t SizeS = 0;

  SdiffGetPos(SdifF->Stream, &(SdifF->StartChunkPos));

  SdifFGetSignature(SdifF, &SizeR);
  SizeS = SizeR += SdifFReadChunkSize(SdifF);
  SizeR += SdiffReadUInt4 (&(SdifF->FormatVersion), 1, SdifF) * sizeof(SdifUInt4);
  SizeR += SdiffReadUInt4 (&(SdifF->TypesVersion),  1, SdifF) * sizeof(SdifUInt4);
  
  if (SdifF->CurrSignature != eSDIF)
  {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess), "%s not correctly read", 
	       SdifSignatureToString(eSDIF));
      _SdifFError(SdifF, eBadHeader, errorMess);
      return 0;
  }

  /* read rest of header chunk (might contain additional data) */
  SdifFReadPadding (SdifF, SdifF->ChunkSize - (SizeR - SizeS));

  if (SdifF->FormatVersion != _SdifFormatVersion)
  {
      char errorMess[_SdifStringLen];
      const char *mfmt = SdifF->FormatVersion > _SdifFormatVersion
	? "file is in a newer SDIF format version (%d) than the library (%d)"
	: "File is in an old SDIF format version (%d).  "
	  "The library (version %d) is not backwards compatible.";

      snprintf(errorMess, sizeof(errorMess),
	       mfmt, SdifF->FormatVersion, _SdifFormatVersion);
      _SdifFError(SdifF, eBadFormatVersion, errorMess);
      return 0;
  }
    
  return SizeR;
}


size_t 
SdifFReadNameValueLCurrNVT(SdifFileT *SdifF)
{
  /* Signature of chunck already read and checked for 1NVT */
  size_t SizeR = 0;
  
  SdiffGetPos(SdifF->Stream, &(SdifF->StartChunkPos));
  SdifF->StartChunkPos -= sizeof(SdifSignature);

#if (_SdifFormatVersion >= 3)	
  /* read frame header */
  SizeR += SdifFReadFrameHeader (SdifF);
  SdifF->ChunkSize = SdifF->CurrFramH->Size;
#else
  SizeR += SdifFReadChunkSize(SdifF);
#endif
  SizeR += SdifFGetNameValueLCurrNVT(SdifF);
  
  if (    (SizeR != SdifF->ChunkSize + sizeof(SdifInt4))
       && ((unsigned) SdifF->ChunkSize != (unsigned) _SdifUnknownSize))
  {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess),
	       "erreur size 1NVT: lu: %lu  Attendu: %lu\n",
	       (long unsigned int)SizeR - sizeof(SdifInt4), (long unsigned int)SdifF->ChunkSize);
      _SdifRemark(errorMess);
  }

  return SizeR;
}


size_t
SdifFReadOneMatrixType(SdifFileT *SdifF)
{
  return SdifFGetOneMatrixType(SdifF, 's');
}


size_t SdifFReadOneFrameType(SdifFileT *SdifF)
{
  return SdifFGetOneFrameType(SdifF, 's');
}


/****************************************************************************/
/* THE FOLLOWING FUNCTION DOESN'T TAKE CARE ANYMORE ABOUT THE OLD SDIF SPECIFICATION */
/****************************************************************************/

/* SdifFReadAllType ne lit pas "1TYP" puisque l'on sera aiguillie sur cette fonction 
 * apres lecture de "1TYP"
 */
/* Signature of chunck already read */
size_t
SdifFReadAllType(SdifFileT *SdifF)
{
  size_t  SizeR = 0;
  
  SdiffGetPos(SdifF->Stream, &(SdifF->StartChunkPos));
  SdifF->StartChunkPos -= sizeof(SdifSignature);
  
  SizeR += SdifFReadFrameHeader (SdifF);
  SdifF->ChunkSize = SdifF->CurrFramH->Size;

  if (SdifF->CurrFramH && SdifFCurrNbMatrix(SdifF))
    /* For the new version of SDIF specification (June 1999) */
    {
      SdifStringT *SdifString;

      SdifString = SdifStringNew();
      
      SizeR += SdifFReadTextMatrix(SdifF, SdifString);
      SizeR += SdifFGetAllTypefromSdifString(SdifF, SdifString);

      SdifStringFree(SdifString);
    }
  else
    {
      SizeR += SdifFGetAllType(SdifF, 's');
    }
  
  if (    (SizeR != SdifF->ChunkSize + sizeof(SdifInt4))
       && ((unsigned) SdifF->ChunkSize != (unsigned) _SdifUnknownSize))
    {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess),
	       "erreur size 1TYP: lu: %lu  Attendu: %lu\n",
	       (long unsigned int)SizeR - sizeof(SdifInt4), (long unsigned int)SdifF->ChunkSize);
      _SdifRemark(errorMess);
    }
  
  return SizeR;
}




/*****************************************************************************/
/* THE FOLLOWING FUNCTION DOESN'T TAKE CARE ANYMORE ABOUT THE OLD SDIF SPECIFICATION */
/*****************************************************************************/
  
/* SdifFReadAllStreamID ne lit pas "SSIC" puisque l'on sera aiguillie sur cette fonction 
 * apres lecture de "SSIC"
 */
size_t
SdifFReadAllStreamID(SdifFileT *SdifF)
{
  size_t SizeR = 0;
  
  SdiffGetPos(SdifF->Stream, &(SdifF->StartChunkPos));
  SdifF->StartChunkPos -= sizeof(SdifSignature);
  
  SizeR += SdifFReadFrameHeader (SdifF);
  SdifF->ChunkSize = SdifF->CurrFramH->Size;

  if (SdifF->CurrFramH && SdifFCurrNbMatrix(SdifF))
    {
      /* For the new version of SDIF specification (June 1999) */
      SdifStringT *SdifString;

      SdifString = SdifStringNew();

      SizeR += SdifFReadTextMatrix(SdifF, SdifString);
      SizeR += SdifFGetAllStreamIDfromSdifString(SdifF, SdifString);

      SdifStringFree(SdifString);
    }
  else
    {
      SizeR += SdifFGetAllStreamID(SdifF, 's');
    }
  
  if (    (SizeR != SdifF->ChunkSize + sizeof(SdifInt4))
       && ((unsigned) SdifF->ChunkSize != (unsigned) _SdifUnknownSize))
    {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess),
	       "erreur size 1IDS: lu: %lu  Attendu: %lu\n",
	       (long unsigned int)SizeR - sizeof(SdifInt4),(long unsigned int) SdifF->ChunkSize);
      _SdifRemark(errorMess);
    }

  return SizeR;
}



/* Read from the end of File Header until TO HAVE READ THE FIRST FRAME SIGNATURE
 * Then, the first frame signature is stocked into SdifF->CurrSignature 
 */
size_t
SdifFReadAllASCIIChunks(SdifFileT *SdifF)
{
  size_t    SizeR = 0;
  
  while (  (SdifFGetSignature(SdifF, &SizeR)!=eEof)  )
    {
      switch (SdifF->CurrSignature)
	{
	case e1NVT :
	  SdifNameValuesLNewTable(SdifF->NameValues, _SdifNVTStreamID);
	  SizeR += SdifFReadNameValueLCurrNVT(SdifF);
	  break;
	  
	case e1TYP :
	  SizeR += SdifFReadAllType(SdifF);
	  break;
	  
	case e1IDS :
	  SizeR += SdifFReadAllStreamID(SdifF);
	  break;

	default:
	  return SizeR;
	}
    }

  return SizeR;
}



size_t
SdifFReadMatrixHeader(SdifFileT *SdifF)
{
  size_t newread, SizeR = 0;
  SdifUInt4 UIntTab[3];

  SdifFCreateCurrMtrxH(SdifF); /* create only if it's necessary */
  
  SdiffReadSignature(&SdifF->CurrMtrxH->Signature, SdifF, &SizeR);
  if (SdifF->CurrMtrxH->Signature == eEmptySignature)
      return 0;	/* read error */

  /* read 3 unsigned ints: datatype, nrow, ncol, copy into right fields */
  newread = SdiffReadUInt4(UIntTab, 3, SdifF);
  SizeR += newread * sizeof(SdifUInt4);

  SdifF->CurrMtrxH->DataType  = (SdifDataTypeET) UIntTab[0];
  SdifF->CurrMtrxH->NbRow     = UIntTab[1];
  SdifF->CurrMtrxH->NbCol     = UIntTab[2];
  
  SdifF->CurrOneRow->DataType = SdifF->CurrMtrxH->DataType;

  if ( SdifFTestMatrixHeader(SdifF) )
    {
      SdifReInitOneRow(SdifF->CurrOneRow,
                       SdifF->CurrMtrxH->DataType,
                       SdifF->CurrMtrxH->NbCol);
    }

  return SizeR;
}



size_t
SdifFReadOneRow(SdifFileT *SdifF)
{
    /* case template for type from SdifDataTypeET */
#   define readrowcase(type) \
    case e##type:  return (sizeof (Sdif##type) *			  \
			   SdiffRead##type (SdifF->CurrOneRow->Data.type, \
					    SdifF->CurrOneRow->NbData,    \
					    SdifF));

    switch (SdifF->CurrOneRow->DataType)
    {
        /* generate cases for all types */
	sdif_foralltypes (readrowcase);

	default :
	{
	    char errorMess[_SdifStringLen];

	    snprintf(errorMess, sizeof(errorMess), 
		     "OneRow 0x%04x, then Float4 used", 
		     SdifF->CurrOneRow->DataType);
	    _SdifFError(SdifF, eTypeDataNotSupported, errorMess);
	    return (sizeof(SdifFloat4) * 
		    SdiffReadFloat4(SdifF->CurrOneRow->Data.Float4,
				    SdifF->CurrOneRow->NbData,
				    SdifF));
	}
	break;
    }
}


/* skip one matrix row, when reading row by row with SdifFReadOneRow */
size_t SdifFSkipOneRow(SdifFileT *SdifF)
{
    return (SdifFSkip(SdifF, SdifSizeofDataType(SdifF->CurrMtrxH->DataType) 
		             * SdifF->CurrMtrxH->NbCol));
}


/* Frame Signature read yet, then update CurrFramH->FrameSignature.
 * If it'is the first ReadFrameHeader then Create CurrFramH
 */
size_t
SdifFReadFrameHeader(SdifFileT *SdifF)
{
  size_t SizeR = 0;
  SdifUInt4 UInt4Tab[2];

  SdiffGetPos(SdifF->Stream, &(SdifF->StartChunkPos));
  SdifF->StartChunkPos -= sizeof(SdifSignature);

  /* Create only if it's necessary else update signature */
  SdifFCreateCurrFramH(SdifF, SdifF->CurrSignature);

  SizeR += sizeof(SdifUInt4)  * SdiffReadUInt4 ( &(SdifF->CurrFramH->Size), 1, SdifF);
  SizeR += sizeof(SdifFloat8) * SdiffReadFloat8( &(SdifF->CurrFramH->Time), 1, SdifF);
  SizeR += sizeof(SdifUInt4)  * SdiffReadUInt4(  UInt4Tab, 2, SdifF);

  SdifF->CurrFramH->NumID    = UInt4Tab[0];
  SdifF->CurrFramH->NbMatrix = UInt4Tab[1];

  /*SdifFSetCurrFrameHeader (SdifF, Signature, Size, NbMatrix, NumID, Time);*/

  SdifF->CurrFramT = SdifTestFrameType(SdifF, SdifF->CurrFramH->Signature);
  if (SdifF->CurrFramT)
    SdifFReInitMtrxUsed(SdifF);

  return SizeR;
}



size_t
SdifFReadPadding (SdifFileT *SdifF, size_t Padding)
{
	char sdifString[_SdifStringLen];
    assert (Padding <= _SdifStringLen);
    return sizeof(char) * Sdiffread(sdifString, sizeof(char), Padding, SdifF);
}



size_t
SdifFReadUndeterminatedPadding(SdifFileT *SdifF)
{
  size_t SizeR = 0;
  int c;

  while ((c = fgetc(SdifF->Stream) == (int)' ') && (!feof(SdifF->Stream)))
    SizeR++;
  
  if (feof(SdifF->Stream))
    return SizeR;
  else
    {
      ungetc(c, SdifF->Stream);
      return SizeR;
    }
}


/* skip given number of bytes, either by seeking or by reading bytes */
size_t SdifFSkip (SdifFileT *SdifF, size_t nbytes)
{
  if(SdifF->isSeekable) {
    SdiffPosT Pos = 0;
    SdiffGetPos(SdifF->Stream, &Pos);
    Pos += nbytes;
    if (SdiffSetPos(SdifF->Stream, &Pos) != 0)
      {
#if HAVE_ERRNO_H
	if (errno == ESPIPE)
          {  /* second chance: SdiffSetPos didn't work because we're
                reading from a pipe.  Instead of making the whole thing
                explode, we do the little extra work to read and throw away
                the data. */
	    return (SdifFReadAndIgnore (SdifF, nbytes));
          }
	else
#endif
          return 0;
      }
    else
    {
	return nbytes;
    }
  }
  else
    return (SdifFReadAndIgnore (SdifF, nbytes));
}


/* read and throw away bytes data. */
size_t
SdifFReadAndIgnore (SdifFileT *SdifF, size_t bytes)
{
    size_t numread = 0, portion = _SdifStringLen;

    while (bytes > 0  &&  portion)
    {
	portion  = SdifFReadPadding (SdifF, MIN (bytes, _SdifStringLen));
	numread += portion;
	bytes   -= portion;
    }
    return (numread);
}


/* SdifFSkipMatrix read entire matrix header. */
size_t SdifFSkipMatrix(SdifFileT *SdifF)
{
  size_t
    SizeR = 0;
  
  SizeR =  SdifFReadMatrixHeader(SdifF);
  SizeR += SdifFSkipMatrixData(SdifF);
  return SizeR;
}


/* SdifFSkipMatrixData don't read matrix header. */
size_t SdifFSkipMatrixData(SdifFileT *SdifF)
{
  size_t    NbBytesToSkip;

  NbBytesToSkip =
    SdifF->CurrMtrxH->NbCol 
    * SdifF->CurrMtrxH->NbRow 
    * SdifSizeofDataType(SdifF->CurrMtrxH->DataType);

  NbBytesToSkip += SdifPaddingCalculate(NbBytesToSkip);

  return (SdifFSkip(SdifF, NbBytesToSkip));
}


/* Cette fonction à le même sens que SdifSkipMatrixData mais pour les
   frames. Il faut donc pour l'utiliser avoir au préalable lu la
   signature et l'entête.  */
size_t SdifFSkipFrameData(SdifFileT *SdifF)
{
    size_t    SizeR = 0, Boo, NbBytesToSkip;
    SdifUInt4 iMtrx;
    
    if (SdifF->CurrFramH->Size != _SdifUnknownSize)
    {
	NbBytesToSkip = SdifF->CurrFramH->Size - _SdifFrameHeaderSize;
        Boo = SdifFSkip (SdifF, NbBytesToSkip);
        if (Boo != NbBytesToSkip)
	{
	    char errorMess[_SdifStringLen];

            snprintf(errorMess, sizeof(errorMess),
		     "Skip  FrameData %s ID:%u T:%g\n",  
		     SdifSignatureToString(SdifF->CurrFramH->Signature),
		     SdifF->CurrFramH->NumID, SdifF->CurrFramH->Time);
            _SdifError(eEof, errorMess);
            return 0;
	}
        else
	    return Boo;
    }
    else
    {
	for (iMtrx = 0; iMtrx<SdifF->CurrFramH->NbMatrix; iMtrx++)
	{
	    Boo = SdifFSkipMatrix(SdifF);
	    if (Boo == 0)
	    {
		char errorMess[_SdifStringLen];

		snprintf(errorMess, sizeof(errorMess),
			 "Skip Matrix %d in FrameData %s ID:%u T:%g\n", iMtrx, 
			 SdifSignatureToString(SdifF->CurrFramH->Signature),
			 SdifF->CurrFramH->NumID, SdifF->CurrFramH->Time);
		_SdifError(eEof, errorMess);
		return 0;
	    }
	    else
		SizeR += Boo;
	}
	return SizeR;
    }
}



/*DOC:
  Function to read text matrix.
  Read header.
  Call SdifFReadTextMatrixData.
  Read padding.
*/
size_t SdifFReadTextMatrix(SdifFileT *SdifF, SdifStringT *SdifString)
{
  size_t SizeR = 0;
  FILE *file;

  file = SdifF->Stream;
  
  SizeR += SdifFReadMatrixHeader (SdifF);
  SizeR += SdifFReadTextMatrixData(SdifF, SdifString);

  /* Number of bytes written */
  SizeR += SdifFReadPadding(SdifF, SdifFPaddingCalculate (file, SizeR));
  return SizeR;
}



/*DOC:
  Function to read text matrix data.
  Make reallocation.
  Read data.
*/
size_t SdifFReadTextMatrixData(SdifFileT *SdifF, SdifStringT *SdifString)
{
  int nrow;
  char *str;
  size_t SizeR = 0;
  int success = 1;
  
  nrow = SdifFCurrNbRow(SdifF);
  str = SdifCalloc(char, nrow * SdifFCurrNbCol(SdifF));
  SizeR += SdiffReadChar(str, nrow, SdifF);

  /* Append string in SdifString */
  success = SdifStringAppend(SdifString, str);
  
  SdifFree(str);
  return SizeR;
}


/* read matrix data and padding into field CurrMtrxData.
   matrix header must have been read before */
size_t SdifFReadMatrixData (SdifFileT *file)
{
    SdifMatrixHeaderT *mtxh     = file->CurrMtrxH;
    int		       numelem  = mtxh->NbRow * mtxh->NbCol;  /* elements */
    size_t	       nread    = 0;	/* bytes read */
    int		       ok;

    /* update header pointer in matrix data struct to point to the
       file's current matrix header and see if there's enough space
       for data, if not, grow buffer */
    ok = SdifMatrixDataUpdateHeader(file->CurrMtrxData, mtxh);

    if (!ok)
    { /* problem allocating data, attach last global error to file */
      _SdifFError(file, eAllocFail, 
		  "not enough memory for Matrix Data header update");
	return 0;
    }

    /* case template for type from SdifDataTypeET */
#   define readdatacase(type) \
    case e##type:  nread += (sizeof (Sdif##type) *			     \
			     SdiffRead##type (file->CurrMtrxData->Data.type, \
					      numelem, file));       \
    break;

    switch (mtxh->DataType)
    {
        /* generate cases for all types */
	sdif_foralltypes (readdatacase)

	default:
	{
	    char errorMess  [_SdifStringLen];

	    snprintf(errorMess, sizeof(errorMess),
		     "SdifFReadMatrixData 0x%04x, then Float4 used", 
		     mtxh->DataType);
	    _SdifFError(file, eTypeDataNotSupported, errorMess);

	    nread += (sizeof(SdifFloat4) * 
		    SdiffReadFloat4(file->CurrMtrxData->Data.Float4, 
				    numelem, file));
	}
	break;
    }

    if (nread > 0  ||  numelem == 0)
    {   /* only read padding when data was read ok */
	nread += SdifFReadPadding(file, SdifPaddingCalculate(nread));
    }

    return nread;
}
