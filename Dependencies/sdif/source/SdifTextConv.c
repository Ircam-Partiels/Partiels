/* $Id: SdifTextConv.c,v 3.14 2005-10-21 14:32:30 schwarz Exp $
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
 * Convert a pseudo-sdif text file to a sdif file
 *
 * author: Dominique Virolle 1997
 *
 *
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.13  2005/04/07 15:56:48  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.12  2004/07/22 14:47:56  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.11  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.10  2003/06/24 16:01:32  roebel
 * permanently removed references to UniversalEnvVar.h
 *
 * Revision 3.9  2003/06/24 15:25:01  roebel
 * Removed UniversalEnvVar.h which in fact has never been used in SDIF.
 *
 * Revision 3.8  2002/08/05 14:21:38  roebel
 * Removed unused variable.
 *
 * Revision 3.7  2001/05/02 09:34:48  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.6  2000/11/15 14:53:37  lefevre
 * no message
 *
 * Revision 3.5  2000/11/14  10:42:27  lefevre
 * no message
 *
 * Revision 3.4  2000/10/27  20:03:45  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.3.2.1  2000/08/21  21:35:55  tisseran
 * *** empty log message ***
 *
 * Revision 3.3  2000/05/12  14:41:49  schwarz
 * On behalf of Adrien, synchronisation with Mac sources, with some slight
 * changes because of cross-platform issues:
 * - Mac only stuff: XpSetFileAttribute XpFileSize
 * - Cross platform wrapper: XpGetenv XpExit
 * - Dangerous: strings.h (and thus bzero, bcopy) is not ANSI and thus doesn't
 *   exist on Mac.  Use string.h and memset, memcpy.
 *
 * Revision 3.2  1999/09/28  13:09:16  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.1  1999/03/14  10:57:25  virolle
 * SdifStdErr add
 *
 * Revision 2.4  1999/02/28  12:16:56  virolle
 * memory report
 *
 * Revision 2.3  1999/01/23  15:56:02  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:48  virolle
 * General Lists, and special chunk preparation to become frames
 */


#include <stdlib.h>

#include "sdif_portability.h"
#ifdef USE_XPGUI
#include "XpGuiCalls.h"
#endif

#include <sdif.h>
#include "SdifGlobals.h"
#include "SdifFile.h"
#include "SdifTest.h"
#include "SdifFScan.h"
#include "SdifFWrite.h"
#include "SdifErrMess.h"


size_t SdifFTextConvMatrixData     (SdifFileT *SdifF);
size_t SdifFTextConvMatrix         (SdifFileT *SdifF);
size_t SdifFTextConvFrameData      (SdifFileT *SdifF);
size_t SdifFTextConvFrameHeader    (SdifFileT *SdifF);
size_t SdifFTextConvFrame          (SdifFileT *SdifF);
size_t SdifFTextConvAllFrame       (SdifFileT *SdifF);
size_t SdifFTextConvFramesChunk    (SdifFileT *SdifF);
size_t SdifFTextConv               (SdifFileT *SdifF);


size_t
SdifFTextConvMatrixData(SdifFileT *SdifF)
{
  size_t
    SizeW = 0;
  SdifUInt4
    iRow;

  if ( (SdifFCurrNbCol(SdifF) > 0) && (SdifFCurrNbRow(SdifF) > 0) )
    {      
      for(iRow=0; iRow<SdifFCurrNbRow(SdifF); iRow++)
        {
	      SdifFScanOneRow(SdifF);
	      SizeW += SdifFWriteOneRow(SdifF);
          if (SdifFLastErrorTag (SdifF)  != eNoError)
            return SizeW;
	    }
      
      return SizeW;
    }
  else
    return 0; /* empty matrix */
}








size_t
SdifFTextConvMatrix(SdifFileT *SdifF)
{
  size_t SizeW = 0;

  SdifFScanMatrixHeader(SdifF);
  if (SdifFLastErrorTag (SdifF)  != eNoError)
     return SizeW;

  SizeW += SdifFWriteMatrixHeader(SdifF);
  SizeW += SdifFTextConvMatrixData(SdifF);
  SizeW += SdifFWritePadding(SdifF, SdifFPaddingCalculate(SdifF->Stream, SizeW));
  
  return SizeW;
}









size_t
SdifFTextConvFrameData(SdifFileT *SdifF)
{
  size_t  SizeW = 0;
  SdifUInt4  iMtrx;

  for(iMtrx=1; iMtrx<=SdifFCurrNbMatrix(SdifF); iMtrx++)
    {
      SizeW += SdifFTextConvMatrix(SdifF);
      if (SdifFLastErrorTag (SdifF)  != eNoError)
        return SizeW;
    }
  
  return SizeW;
}





size_t
SdifFTextConvFrameHeader(SdifFileT *SdifF)
{
  size_t  SizeW = 0;

  SdifFScanFrameHeader(SdifF);
  SdifF->CurrFramH->Size  = _SdifUnknownSize;

  SizeW = SdifFWriteFrameHeader(SdifF);

  return SizeW;
}





size_t
SdifFTextConvFrame(SdifFileT *SdifF)
{
  size_t SizeW;
  
  SizeW = SdifFTextConvFrameHeader(SdifF);
  if (SdifFLastErrorTag (SdifF)  != eNoError)
    return SizeW;
  SizeW += SdifFTextConvFrameData(SdifF);
  if (SdifFLastErrorTag (SdifF)  != eNoError)
    return SizeW;
  SdifF->CurrFramH->Size = (SdifUInt4) SizeW;

  SdifUpdateChunkSize(SdifF, SizeW - sizeof(SdifSignature) - sizeof(SdifInt4));
  
  return SdifF->CurrFramH->Size;
}








size_t
SdifFTextConvAllFrame(SdifFileT *SdifF)
{
  size_t
    SizeR = 0,
    SizeW = 0;
  int CharEnd = 0;

  while ((CharEnd != eEof) && (SdifFCurrSignature(SdifF) != eENDC))
    {
      SizeW += SdifFTextConvFrame(SdifF);
      if (SdifFLastErrorTag (SdifF)  != eNoError)
         return SizeW;
      SdifFCleanCurrSignature(SdifF);
      CharEnd = SdiffGetSignature (SdifF->TextStream, &(SdifF->CurrSignature), &SizeR);
#ifdef USE_XPGUI
      {
	long int fileSize = ftell(SdifF->TextStream);
	XpProBarSet((float)fileSize);
      }
#endif
    }

  if (CharEnd == eEof)
    _SdifError(eEof, "SdifFTextConvFrame");

  return SizeW;
}








size_t
SdifFTextConvFramesChunk(SdifFileT *SdifF)
{
  size_t
    SizeW = 0,
    SizeR = 0;  

  SdiffGetSignature(SdifF->TextStream, &(SdifF->CurrSignature), &SizeR);
  SdifF->ChunkSize = SdifFTextConvAllFrame(SdifF);
  SizeW += SdifF->ChunkSize;
  if (SdifFLastErrorTag (SdifF)  != eNoError)
     return SizeW;

  if (SdifFCurrSignature(SdifF) ==  eENDC)
    /*SdifUpdateChunkSize(SdifF, SdifF->ChunkSize)
     */
      ;
  else
    _SdifFError(SdifF, eSyntax, "Attempt to read 'ENDC' failed");
  
  return SizeW;
}








size_t
SdifFTextConv(SdifFileT *SdifF)
{
  size_t  SizeW = 0;
    
  SdifFScanGeneralHeader(SdifF);  
  
  SdifF->FileSize = 0;
  SdifF->FileSize += SdifFWriteGeneralHeader(SdifF);

  SdifFScanAllASCIIChunks(SdifF);
  SdifF->FileSize += SdifFWriteAllASCIIChunks(SdifF);

  switch (SdifFCurrSignature(SdifF))
  {
    case eSDFC:
      SizeW = SdifFTextConvFramesChunk(SdifF);
      SdifF->FileSize += SizeW;
      break;

    case eENDF:
      return SdifF->FileSize;

    default:
    {
	char errorMess[_SdifStringLen];

	snprintf(errorMess, sizeof(errorMess),
	      "It is not a chunk signature : '%s'",
	      SdifSignatureToString(SdifFCurrSignature(SdifF)));
	_SdifFError(SdifF, eSyntax, errorMess);
    }
    break;
  }
    
  if (SdifFLastErrorTag (SdifF)  != eNoError)
      return SizeW;
  if (SdifFCurrSignature(SdifF) != eENDC)
    _SdifFError(SdifF, eSyntax, "Attempt to read 'ENDF' failed");
  
  return SdifF->FileSize;
}








/* upper level : open the text in read mode */
size_t
SdifTextToSdif(SdifFileT *SdifF, char *TextStreamName)
{
  size_t FileSizeW = 0;

  if (SdifF->Mode != eWriteFile)
    _SdifFError(SdifF, eBadMode, "it must be eWriteFile");

  if (SdifF->TextStream)
    {
      SdiffBinClose(SdifF->TextStream);
      if (SdifF->TextStreamName)
	  SdifFree(SdifF->TextStreamName);
      _SdifRemark("TextStream Re-initialisation\n");
    }
  
  if (SdifStrCmp(TextStreamName, SdifF->Name) == 0)
    {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess), 
	       "Read=%s, Write=%s.", TextStreamName, SdifF->Name);
      _SdifFError(SdifF, eReadWriteOnSameFile, errorMess);
      return FileSizeW;
    }
  else
    {
      SdifFOpenText(SdifF, TextStreamName, eReadFile);
      if (! SdifF->TextStream)
	    {
	      return  FileSizeW;
	    }
      else
        {
#ifdef USE_XPGUI
	  {
	    long int fileSize = XpFileSize(TextStreamName);
	    XpProBarString("Convert Text To Sdif");
	    XpProBarInit((float)fileSize);
	  }
#endif
          FileSizeW = SdifFTextConv(SdifF);
          fflush(SdifF->Stream);
 
          return FileSizeW;
        }
    }
  
  /*  return FileSizeW; */
}

