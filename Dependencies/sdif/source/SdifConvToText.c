/* $Id: SdifConvToText.c,v 3.13 2005-04-07 15:56:46 schwarz Exp $
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
 * functions to convert an sdif file into a pseudo-sdif text file.
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.12  2004/09/06 16:50:58  roebel
 * Fixed typo in USE_XPGUI macro.
 *
 * Revision 3.11  2004/07/22 14:47:55  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.10  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.9  2003/06/24 16:01:32  roebel
 * permanently removed references to UniversalEnvVar.h
 *
 * Revision 3.8  2003/06/24 15:24:55  roebel
 * Removed UniversalEnvVar.h which in fact has never been used in SDIF.
 *
 * Revision 3.7  2001/05/02 09:34:40  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.6  2000/11/15 14:53:24  lefevre
 * no message
 *
 * Revision 3.5  2000/11/14  10:42:26  lefevre
 * no message
 *
 * Revision 3.4  2000/10/27  20:03:24  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.3.2.1  2000/08/21  21:34:56  tisseran
 * *** empty log message ***
 *
 * Revision 3.3  2000/05/12  14:41:42  schwarz
 * On behalf of Adrien, synchronisation with Mac sources, with some slight
 * changes because of cross-platform issues:
 * - Mac only stuff: XpSetFileAttribute XpFileSize
 * - Cross platform wrapper: XpGetenv XpExit
 * - Dangerous: strings.h (and thus bzero, bcopy) is not ANSI and thus doesn't
 *   exist on Mac.  Use string.h and memset, memcpy.
 *
 * Revision 3.2  1999/09/28  13:08:48  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.1  1999/03/14  10:56:31  virolle
 * SdifStdErr add
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
#include "SdifFGet.h"
#include "SdifFRead.h"
#include "SdifErrMess.h"


size_t SdifFConvToTextMatrixData     (SdifFileT *SdifF);
size_t SdifFConvToTextMatrixHeader   (SdifFileT *SdifF);
size_t SdifFConvToTextMatrix         (SdifFileT *SdifF);
size_t SdifFConvToTextFrameData      (SdifFileT *SdifF);
size_t SdifFConvToTextFrameHeader    (SdifFileT *SdifF);
size_t SdifFConvToTextFrame          (SdifFileT *SdifF);
size_t SdifFConvToTextAllFrame       (SdifFileT *SdifF);
size_t SdifFConvToText               (SdifFileT *SdifF);


size_t
SdifFConvToTextMatrixData(SdifFileT *SdifF)
{
  size_t
    SizeR = 0;
  SdifUInt4
    iRow;

  if ( (SdifFCurrNbCol(SdifF) > 0) && (SdifFCurrNbRow(SdifF) > 0) )
    {
      for(iRow=0; iRow<SdifFCurrNbRow(SdifF); iRow++)
	{
	  SizeR += SdifFReadOneRow(SdifF);
	  SdifFPrintOneRow(SdifF);
	}
      
      return SizeR;
    }
  else
    return 0; /* empty matrix */
}







size_t
SdifFConvToTextMatrixHeader(SdifFileT *SdifF)
{
  size_t SizeR = 0;

  SizeR += SdifFReadMatrixHeader(SdifF);
  SdifFPrintMatrixHeader(SdifF);

  return SizeR;
}









size_t
SdifFConvToTextMatrix(SdifFileT *SdifF)
{
  size_t SizeR = 0;

  SizeR += SdifFConvToTextMatrixHeader(SdifF);
  SizeR += SdifFConvToTextMatrixData(SdifF);
  SizeR += SdifFReadPadding(SdifF, SdifFPaddingCalculate(SdifF->Stream, SizeR));

  return SizeR;
}









size_t
SdifFConvToTextFrameData(SdifFileT *SdifF)
{
  size_t SizeR = 0;
  SdifUInt4  iMtrx;

  for(iMtrx=1; iMtrx<=SdifFCurrNbMatrix(SdifF); iMtrx++)
    {
      SizeR += SdifFConvToTextMatrix(SdifF);
    }
  
  return SizeR;
}




size_t
SdifFConvToTextFrameHeader(SdifFileT *SdifF)
{
  size_t SizeR = 0;

  SizeR += SdifFReadFrameHeader(SdifF);

  fprintf(SdifF->TextStream, "\n"); /* only for presentation */
  SdifFPrintFrameHeader(SdifF);

  return SizeR;
}








size_t
SdifFConvToTextFrame(SdifFileT *SdifF)
{
  size_t SizeR = 0;

  SizeR += SdifFConvToTextFrameHeader(SdifF);
  SizeR += SdifFConvToTextFrameData(SdifF);

  return SizeR;
}








size_t
SdifFConvToTextAllFrame(SdifFileT *SdifF)
{
  size_t
    SizeRSign = 0,
    SizeR = 0;
  int CharEnd = 0;

  while (CharEnd != eEof)
    {
      SizeR += SdifFConvToTextFrame(SdifF);
      SdifFCleanCurrSignature(SdifF);
      CharEnd = SdifFGetSignature(SdifF, &SizeRSign);
      if (CharEnd != eEof)
      {  
	SizeR += SizeRSign;
	SizeRSign = 0;
      }
#ifdef USE_XPGUI
      {
	long int fileSize = ftell(SdifF->Stream);
	XpProBarSet((float)fileSize);
      }
#endif
    }

  return SizeR;
}




size_t
SdifFConvToText(SdifFileT *SdifF)
{
  size_t  SizeR = 0;
  

  SizeR += SdifFReadGeneralHeader(SdifF);
  SdifFPrintGeneralHeader(SdifF);

  SizeR += SdifFReadAllASCIIChunks(SdifF);
  SdifFPrintAllASCIIChunks(SdifF);

  if (SdifFCurrSignature(SdifF) != eEmptySignature)
    {
      fprintf(SdifF->TextStream, "\n%s\n", SdifSignatureToString(eSDFC));
      SizeR += SdifFConvToTextAllFrame(SdifF);
      fprintf(SdifF->TextStream, "\n%s\n", SdifSignatureToString(eENDC));
    }
      
  fprintf(SdifF->TextStream, "%s\n", SdifSignatureToString(eENDF));

  return SizeR;
}








/* upper level : open the text in write mode */
size_t
SdifToText(SdifFileT *SdifF, char *TextStreamName)
{
  size_t SizeR = 0;
  char errorMess[_SdifStringLen];
  
  if (SdifF->Mode != eReadFile)
      _SdifFError(SdifF, eBadMode, "it must be eReadFile");

  if (SdifF->TextStream)
    {
      SdiffBinClose(SdifF->TextStream);
      if (SdifF->TextStreamName)
	free(SdifF->TextStreamName);
      _SdifRemark("TextStream Re-initialisation\n");
    }
  
  SdifF->TextStreamName = SdifCreateStrNCpy(TextStreamName, SdifStrLen(TextStreamName)+1);

  if (SdifStrCmp(SdifF->TextStreamName, SdifF->Name) == 0)
    {
      sprintf(errorMess, "Write=%s, Read=%s.", SdifF->TextStreamName, SdifF->Name);
      _SdifFError(SdifF, eReadWriteOnSameFile, errorMess);
      return SizeR;
    }
  else
    {
      SdifFOpenText(SdifF, TextStreamName, eWriteFile);
      if (! SdifF->TextStream)
	    {
	      return  SizeR;
	    }
      else
        {
#ifdef USE_XPGUI
	  {
	    long int fileSize = XpFileSize(SdifF->Name);
	    XpProBarString("Convert Sdif To Text");
	    XpProBarInit((float)fileSize);
	  }
#endif
          SizeR = SdifFConvToText(SdifF);
          fflush(SdifF->TextStream);

	      return  SizeR;
        }
    }
  
  return SizeR;
}

