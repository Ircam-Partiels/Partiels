/* $Id: SdifFGet.c,v 3.24 2011-12-19 17:36:08 roebel Exp $
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
 * F : SdifFileT*, Get : ascii frames reading,
 * common to read a sdif file and a pseudo-sdif text file
 * 'verbose' allows to choise if the file is a sdif file or a pseudo-sdif text file
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.23  2007/11/26 18:51:53  roebel
 * Changed data types to have less casts and
 * less compilation warnings with MSVC.
 * renamed some functions that are deprecated in MSVC.
 *
 * Revision 3.22  2005/10/21 14:32:29  schwarz
 * protect all static buffers from overflow by using snprintf instead of sprintf
 * move big errorMess buffers into error branch to avoid too large stack allocation
 *
 * Revision 3.21  2005/06/10 12:45:11  roebel
 * Fixed SdifFGetOneNameValue for the case where the file is stored in
 * ASCII with windows or macintosh line feed conventions.
 *
 * Revision 3.20  2005/05/24 09:33:40  roebel
 * Fixed last checkin comment which turned out to be the start of
 * a c-comment.
 *
 * Revision 3.19  2005/05/23 19:17:53  schwarz
 * - Sdiffread/Sdiffwrite functions with SdifFileT instead of FILE *
 *   -> eof error reporting makes more sense
 * - more cleanup of sdif.h, above functions are private in SdifRWLowLevel.h
 * - eEof becomes error 4 to be distinguishable from ascii chars
 * - SdifFScanNameValueLCurrNVT reimplemented for ascii only
 *
 * Revision 3.18  2005/04/07 15:56:46  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.17  2004/10/07 14:52:24  roebel
 * Fixed SdifFGetOneNameValue to properly read NVTs from text files.
 *
 * Revision 3.16  2004/09/09 17:29:00  schwarz
 * better syntax error messages and code reformatted to help finding
 * parsing bug that the type definition string has to end with whitespace.
 * At the next bug with the type language parser, it should be rewritten using
 * flex/bison!
 *
 * Revision 3.15  2004/07/22 14:47:56  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.14  2004/06/03 11:18:00  schwarz
 * Profiling showed some waste of cycles in byte swapping and signature reading:
 * - byte swapping now array-wise, not element-wise in SdifSwap<N>[Copy] routines:   -> from 0.24 s (18.5%) to 0.14s
 * - ASCII signature reading function SdiffGetSignature replaced by new binary
 *   function SdiffReadSignature (also in SdifFGetSignature, so the change is
 *   mostly transparent):
 *   -> from 0.11 s (9.6%)  to 0.01 s
 * - overall run time improvement with test case sdifextractall_a01:
 *   -> from 1.20 s         to 0.86 s (40% faster)
 *
 * Revision 3.13  2004/05/03 18:07:27  schwarz
 * Fixed bugs in padding calculation for ascii chunks:
 * 1. DON'T PAD FRAMES!
 * 2. SdifFReadMatrixHeader already accounts for read signature
 * Now, calculating padding from ftell is redundant, but we leave it in,
 * with a warning, until everyone's code is tested.
 *
 * Revision 3.12  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.11  2003/08/06 15:11:45  schwarz
 * Finally removed obsolete functions (like SdifSkip...).
 *
 * Revision 3.10  2001/05/02 09:34:41  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.9  2000/11/15 14:53:25  lefevre
 * no message
 *
 * Revision 3.8  2000/10/27  20:03:26  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.7  2000/08/22  13:37:55  schwarz
 * Replaced short by int, because that's faster and less error-prone.
 *
 * Revision 3.6  2000/08/21  10:02:47  tisseran
 * Add information about compilation when use SdifPrintVersion:
 * - Which SdifTypes.STYP is used.
 * - Who made the compilation.
 * 
 * Revision 3.6.2.3  2000/08/21  21:35:03  tisseran
 * *** empty log message ***
 *
 * Revision 3.6.2.2  2000/08/21  18:34:08  tisseran
 * Add SdifSkipASCIIUntilfromSdifString function (same as SdifSkipASCIIUntil).
 * Add SdifFSkip for SdifSkip for (functions SdifSkip doesn't respect function nomemclature => obsolete).
 *
 * Revision 3.6.2.1  2000/08/21  14:04:06  tisseran
 * *** empty log message ***
 *
 * Revision 3.5  2000/07/18  15:08:28  tisseran
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
 * Revision 3.4  2000/05/12  14:41:45  schwarz
 * On behalf of Adrien, synchronisation with Mac sources, with some slight
 * changes because of cross-platform issues:
 * - Mac only stuff: XpSetFileAttribute XpFileSize
 * - Cross platform wrapper: XpGetenv XpExit
 * - Dangerous: strings.h (and thus bzero, bcopy) is not ANSI and thus doesn't
 *   exist on Mac.  Use string.h and memset, memcpy.
 *
 * Revision 3.3  2000/04/11  14:31:19  schwarz
 * Read/write NVT as frame with 1 text matrix, conforming to SDIF spec.
 *
 * Revision 3.2  1999/09/28  13:08:51  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.1  1999/03/14  10:56:37  virolle
 * SdifStdErr add
 */



#include "sdif_portability.h"

#include <string.h>
#include "SdifRWLowLevel.h"
#include "SdifFRead.h"
#include "SdifFGet.h"
#include "SdifTest.h"
#include "SdifFile.h"
#include "SdifHash.h"
#include "SdifMatrixType.h"
#include "SdifFrameType.h"
#include "SdifStreamID.h"
#include "SdifErrMess.h"
#include <ctype.h>
#include <stdlib.h>



/* binary signature read */
int SdifFGetSignature (SdifFileT *SdifF, size_t *nread)
{
    return SdiffReadSignature(&SdifF->CurrSignature, SdifF, nread);
}




int
SdifFGetOneNameValue(SdifFileT *SdifF, int Verbose, size_t *SizeR)
{

  FILE         *file;

#if 1
  char str[_SdifStringLen];
  char *cs;
  int ncMax=_SdifStringLen;
  int c;
  file = SdifFGetFILE_SwitchVerbose(SdifF, Verbose);

  cs = str;
  
  while( (c = (char) fgetc(file)) != EOF && (ncMax-- > 0))
    {
      SizeR++;
      /* replace carrige return by line feed, which is the character
       * that has to be used to end NVT values in SdifFNameValueLCurrNVTfromString*/
      if(c=='\r')
        c = '\n';
      *cs++ = c;
      if ('\n'== c ) {
        /* skip over empty lines */
        while((c = (char) fgetc(file))!=EOF) {
          if(c != '\n' && c != '\r' ){
            ungetc(c,file);
            break;
          }
        }
        break;
      }
    }
  *cs = '\0';        

  /* NVT is finished if name starts with } */
  if(str[0] == '}') {
    return str[0];
  }
  else {
    /* remove final semicolon that represents end of value in text file */
    if(Verbose =='t') {
      if(cs > str) --cs;
      while(cs>str && isspace(*cs)) *cs-- = '\0';
      if(*cs!=';')
        ++cs;
      *cs++ = '\n'; 
      *cs = '\0';              
    }
    SdifFNameValueLCurrNVTfromString (SdifF, str);
  }
  return '\n';

#else
  int          CharEnd;
  static const char  CharsEnd[] = " \t\n\f\r\v{},;:";
  char sdifString[_SdifStringLen];
  char sdifString2[_SdifStringLen];

  file = SdifFGetFILE_SwitchVerbose(SdifF, Verbose);

  /* Name */

  CharEnd = SdiffGetStringUntil(file, sdifString, _SdifStringLen, SizeR, CharsEnd);

  if ( (CharEnd == '}') && (SdifStrLen(sdifString) == 0) ) /* no more NameValue */
    return  CharEnd;
  if (! isspace(CharEnd))
    {
      char errorMess[_SdifStringLen];

      sprintf(errorMess,
	      "Wait a space_char after '%s', read char : (%d) '%c'",
	      sdifString,
	      CharEnd,
	      CharEnd);
      _SdifFError(SdifF, eSyntax, errorMess);
      return  CharEnd;
    }
  
  if (SdifNameValuesLGetCurrNVT(SdifF->NameValues, sdifString))
    {
      char errorMess[_SdifStringLen];

      sprintf(errorMess, "NameValue : %s ", sdifString);
      _SdifFError(SdifF, eReDefined, errorMess);
      CharEnd = SdiffGetStringUntil(file, sdifString, _SdifStringLen, SizeR, ";");
      return  CharEnd;
    }
  
  
  
  /* Value */
  CharEnd = SdiffGetStringUntil(file, sdifString2, _SdifStringLen, SizeR, _SdifReservedChars);

  if (CharEnd != (unsigned) ';')
    {
      char errorMess[_SdifStringLen];

      sprintf(errorMess,
	      "Attempt to read ';' : '%s%c' ",
	      sdifString2,
	      CharEnd);
      _SdifFError(SdifF, eSyntax, errorMess);
      return  CharEnd;
    }
  
  SdifNameValuesLPutCurrNVT(SdifF->NameValues, sdifString, sdifString2);
  return  CharEnd;

#endif
}





size_t
SdifFGetNameValueLCurrNVT(SdifFileT *SdifF)
{
  size_t    SizeR = 0;
  
  /* nb of matrices > 0: proper SDIF, read text matrices
     else: intermediate format, read pure data in frame */
  if (SdifF->CurrFramH  &&  SdifFCurrNbMatrix (SdifF) > 0) 
    {
      SdifUInt4   i;
      int   nrow;
      char *str;
      
      for (i = 0; i < SdifFCurrNbMatrix (SdifF); i++)
	{
	  SizeR += SdifFReadMatrixHeader (SdifF);
	  nrow   = SdifFCurrNbRow (SdifF);
	  if (SdifFCurrNbCol (SdifF) != 1)
	      _SdifFError (SdifF, eSyntax, "Name-Value Table text matrix must "
			   "have exactly one column!");
          /* force a zero at the end of the string 
           * hacky use of the old writesdif function has created NVTs for that the  
           * NVT matrices do not inlcude a trailing \000.
           */
	  str    = SdifCalloc (char, nrow * SdifFCurrNbCol (SdifF) + 1);
	  SizeR += SdiffReadChar (str, nrow, SdifF);
	  SizeR += SdifFReadPadding(SdifF, SdifFPaddingCalculate (SdifF->Stream, SizeR));
	  SdifFNameValueLCurrNVTfromString (SdifF, str);
	  SdifFree (str);
	}
    }
  else
    {
      int CharEnd;
      char sdifString[_SdifStringLen];
      
      _SdifRemark ("Warning, this file uses an intermediate format for "
		   "the Name-Value Table.  Portablity with programs not "
		   "using the IRCAM SDIF library is not guaranteed.  "
		   "Tip: Use 'sdifextract file newfile' to convert to "
		   "compliant format.");
      CharEnd = SdiffGetStringUntil (SdifF->Stream, sdifString, _SdifStringLen, 
				     &SizeR, _SdifReservedChars);
      if (SdifTestCharEnd (SdifF, CharEnd, '{', sdifString, 
			   SdifStrLen (sdifString) != 0,
			   "Begin of NameValue Table declarations") != eFalse)
	{
	    while (SdifFGetOneNameValue(SdifF, 's', &SizeR) != (int) '}')
	    /*loop*/;
	}
    }
  
  return SizeR;
}



/* changes str! */
int
SdifFNameValueLCurrNVTfromString (SdifFileT *SdifF, char *str)
{
  char *name, *value;

  while (*str)
  {   /* get name */
      name  = str;
      str   = strchr (name, '\t');
      if (!str)  return (0);
      *str++  = (char) 0;

      /* get value */
      value = str;
      str   = strchr (value, '\n');
      if (!str)  return (0);
      *str++  = (char) 0;
      
      /* check if name already used */
      if (SdifNameValuesLGetCurrNVT (SdifF->NameValues, name))
      {
	  char errorMess[_SdifStringLen];

	  sprintf(errorMess, "NameValue : %s ", name);
	  _SdifFError(SdifF, eReDefined, errorMess);
      }
      else
	  SdifNameValuesLPutCurrNVT (SdifF->NameValues, name, value);
  }
  return (1);
}



size_t
SdifFGetOneMatrixType(SdifFileT *SdifF, int Verbose)
{
  SdifMatrixTypeT  *MatrixType;
  size_t           SizeR = 0;
  int              CharEnd;
  SdifSignature    Signature = 0;
  FILE             *file;
  char sdifString[_SdifStringLen];


  file = SdifFGetFILE_SwitchVerbose(SdifF, Verbose);

  CharEnd = SdiffGetSignature(file, &Signature, &SizeR);

  if (SdifTestSignature(SdifF, CharEnd, Signature, "Matrix")== eFalse)
    return SizeR;  
 

  /* Matrix type Creation, Put or Recuperation from SdifF->MatrixTypesTable */
  MatrixType = SdifGetMatrixType(SdifF->MatrixTypesTable, Signature);
  if (! MatrixType)
    {
      MatrixType = SdifCreateMatrixType(Signature,
					SdifGetMatrixType(gSdifPredefinedTypes->MatrixTypesTable, Signature));
      SdifPutMatrixType(SdifF->MatrixTypesTable, MatrixType);
    }
  else
    {
      if (SdifTestMatrixTypeModifMode(SdifF, MatrixType)== eFalse)
	{
	  /* Skip matrix type def, search '}' */
	  SdifTestCharEnd(SdifF,
			  SdifSkipASCIIUntil(file, &SizeR, "}:[];"),
			  '}', "", eFalse,
			  "end of matrix type skiped missing");
	  return SizeR;
	}
    }


  /* ColumnDefs */
  CharEnd = SdiffGetStringUntil(file, sdifString, _SdifStringLen, &SizeR, _SdifReservedChars);
  if (SdifTestCharEnd(SdifF, CharEnd, '{', sdifString,
              SdifStrLen(sdifString) != 0, "Matrix Type") == eFalse)
    return SizeR;
  else
    {
      while (   (CharEnd = SdiffGetStringUntil(file, sdifString, _SdifStringLen, &SizeR, _SdifReservedChars))
	     == (int) ','  )
	{
	  SdifMatrixTypeInsertTailColumnDef(MatrixType, sdifString);
	}

      if (SdifTestCharEnd(SdifF, CharEnd, '}', sdifString, eFalse, "end of matrix type missing") == eFalse)
	return SizeR;
      else
	if (SdifStrLen(sdifString) != 0)
	  SdifMatrixTypeInsertTailColumnDef(MatrixType, sdifString);
    }

  MatrixType->ModifMode = eNoModif;

  return SizeR;
}



/*DOC:
  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
  Return the current matrix type from a SdifStringT
*/
size_t
SdifFGetOneMatrixTypefromSdifString(SdifFileT *SdifF, SdifStringT *SdifString)
{
  SdifMatrixTypeT  *MatrixType;
  int              CharEnd;
  SdifSignature    Signature = 0;
  size_t SizeR = 0;
	char sdifString[_SdifStringLen];
  
  CharEnd = SdiffGetSignaturefromSdifString(SdifString, &Signature);

  if (SdifTestSignature(SdifF, CharEnd, Signature, "Matrix")== eFalse)
    return SizeR;
  
  
  /* Matrix type Creation, Put or Recuperation from SdifF->MatrixTypesTable */
  MatrixType = SdifGetMatrixType(SdifF->MatrixTypesTable, Signature);
  if (! MatrixType)
    {
      MatrixType = SdifCreateMatrixType(Signature,
					SdifGetMatrixType(gSdifPredefinedTypes->MatrixTypesTable, Signature));
      SdifPutMatrixType(SdifF->MatrixTypesTable, MatrixType);
    }
  else
    {
      if (SdifTestMatrixTypeModifMode(SdifF, MatrixType)== eFalse)
	{
	  /* Skip matrix type def, search '}' */
	  SdifTestCharEnd(SdifF,
			  SdifSkipASCIIUntilfromSdifString(SdifString, &SizeR, "}:[];"),
			  '}', "", eFalse,
			  "end of matrix type skiped missing");
	  return SizeR;
	}
    }

  /* ColumnDefs */
  CharEnd = SdiffGetStringUntilfromSdifString(SdifString, sdifString, _SdifStringLen,
					      _SdifReservedChars);
  if (SdifTestCharEnd(SdifF, CharEnd, '{', sdifString,
              SdifStrLen(sdifString) != 0, "Matrix Type") == eFalse)
    return SizeR;
  else
    {
      while ((CharEnd = SdiffGetStringUntilfromSdifString(SdifString, sdifString,
					    _SdifStringLen, _SdifReservedChars))
	     == (int) ','  )
	{
	  SdifMatrixTypeInsertTailColumnDef(MatrixType, sdifString);
	}

      if (SdifTestCharEnd(SdifF, CharEnd, '}', sdifString, eFalse, "end of matrix type missing") == eFalse)
	return SizeR;
      else
	if (SdifStrLen(sdifString) != 0)
	  SdifMatrixTypeInsertTailColumnDef(MatrixType, sdifString);
    }

  MatrixType->ModifMode = eNoModif;

  return SizeR;
}





int
SdifFGetOneComponent(SdifFileT *SdifF, int Verbose,
		     SdifSignature *MatrixSignature, char *ComponentName, size_t *SizeR)
{
  int   CharEnd;
  FILE *file;

  file = SdifFGetFILE_SwitchVerbose(SdifF, Verbose);
  
  
  ComponentName[0]= '\0';
  *MatrixSignature = eEmptySignature;

  /* Matrix Signature */
  CharEnd = SdiffGetSignature(file, MatrixSignature, SizeR); 
  
  if (CharEnd == (unsigned) '}')
    {
      /* no more Component */
      if  (*MatrixSignature == 0) 
	return  CharEnd;
      else
	{
	  char errorMess[_SdifStringLen];

	  sprintf(errorMess,
		  "Incomplete Component : '%s%c'",
		  SdifSignatureToString(*MatrixSignature),
		  CharEnd);
	  _SdifFError(SdifF, eSyntax, errorMess);
	  return CharEnd;
	}
    }
  else
    if (SdifTestSignature(SdifF, CharEnd, *MatrixSignature, "matrix signature of Component")== eFalse)
      return CharEnd;
  
  /* Component Name */
  CharEnd = SdiffGetStringUntil(file, ComponentName,
				_SdifStringLen, SizeR, _SdifReservedChars);
  if (SdifTestCharEnd(SdifF, CharEnd, ';', ComponentName, eFalse,
		      "Component must be finished by ';'") == eFalse)
    return  CharEnd;
  
  return CharEnd;
}


/*DOC:
  Return the current component from a SdifStringT:
  parses signature componentname ; 

  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
*/
int
SdifFGetOneComponentfromSdifString(SdifFileT *SdifF, SdifStringT *SdifString,
				   SdifSignature *MatrixSignature, 
				   char *ComponentName)
{
  int   CharEnd;

  ComponentName[0]= '\0';
  *MatrixSignature = eEmptySignature;

  /* Matrix Signature */
  CharEnd = SdiffGetSignaturefromSdifString(SdifString, MatrixSignature); 
  
  if (CharEnd == (unsigned) '}')
  {
      /* no more Component */
      if (*MatrixSignature == 0) 
	  return  CharEnd;
      else
      {
	  char errorMess[_SdifStringLen];

	  sprintf(errorMess, "Incomplete Component : '%s%c'",
		  SdifSignatureToString(*MatrixSignature), CharEnd);
	  _SdifFError(SdifF, eSyntax, errorMess);
	  return CharEnd;
      }
  }
  else
      if (SdifTestSignature(SdifF, CharEnd, *MatrixSignature, 
			    "matrix signature of Component") == eFalse)
      {
	  return CharEnd;
      }

  /* Component Name */
  CharEnd = SdiffGetStringUntilfromSdifString(SdifString, ComponentName,
		_SdifStringLen, _SdifReservedChars);
  if (SdifTestCharEnd(SdifF, CharEnd, ';', ComponentName, eFalse,
		      "Component must be finished by ';'") == eFalse)
  {
      return  CharEnd;
  }

  return CharEnd;
}


size_t
SdifFGetOneFrameType(SdifFileT *SdifF, int Verbose)
{
  size_t          SizeR = 0;
  int             CharEnd;
  FILE           *file;
  SdifFrameTypeT *FramT;
  char sdifString[_SdifStringLen];
  SdifSignature
    FramSignature = 0,
    MtrxSignature = 0;  


  file = SdifFGetFILE_SwitchVerbose(SdifF, Verbose);
  

  /* FramSignature */
  CharEnd = SdiffGetSignature(file, &FramSignature, &SizeR);
  if (SdifTestSignature(SdifF, CharEnd, FramSignature, "Frame")== eFalse)
    {
      return SizeR;
    }
 

  /* Frame type Creation, Put or Recuperation from SdifF->FrameTypesTable */
  FramT =  SdifGetFrameType(SdifF->FrameTypesTable, FramSignature);
  if (! FramT)
    {
      FramT = SdifCreateFrameType(FramSignature,
				  SdifGetFrameType(gSdifPredefinedTypes->FrameTypesTable, FramSignature));
      SdifPutFrameType(SdifF->FrameTypesTable, FramT);
    }
  else
    if (SdifTestFrameTypeModifMode(SdifF, FramT)== eFalse)
      {
	/* Skip frame type def, search '}' */
	SdifTestCharEnd(SdifF,
			SdifSkipASCIIUntil(file, &SizeR, "}:[]"),
			'}', "", eFalse,
			"end of frame type skiped missing");
	return SizeR;
      }



  /* Components */
  CharEnd = SdiffGetStringUntil(file, sdifString,
				_SdifStringLen, &SizeR, _SdifReservedChars);
  if (   SdifTestCharEnd(SdifF, CharEnd, '{',
			 sdifString, SdifStrLen(sdifString) != 0,
			 "Frame")
	 ==eFalse   )
    {
      return SizeR;
    }
  else
    {
      while ( SdifFGetOneComponent(SdifF,
                                   Verbose,
                                   &MtrxSignature,
                                   sdifString, &SizeR)
              != (unsigned) '}'  )
        {
          if (SdifTestMatrixType(SdifF, MtrxSignature))
	        {
	          SdifFrameTypePutComponent(FramT, MtrxSignature, sdifString);
	          MtrxSignature = 0;
	        }
	    }
    }


  FramT->ModifMode = eNoModif;
    
  return SizeR;
}



/*DOC:
  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
  Return the current frame type from a SdifStringT
*/
size_t
SdifFGetOneFrameTypefromSdifString(SdifFileT *SdifF, SdifStringT *SdifString)
{
  size_t          SizeR = 0;
  int             CharEnd;
  SdifFrameTypeT  *FramT;
  SdifSignature   FramSignature = 0;
  SdifSignature   MtrxSignature = 0;  
  char sdifString[_SdifStringLen];


  /* FramSignature */
  CharEnd = SdiffGetSignaturefromSdifString(SdifString, &FramSignature);
  if (SdifTestSignature(SdifF, CharEnd, FramSignature, "Frame")== eFalse)
  {
      return SizeR;
  }
 

  /* Frame type Creation, Put or Recuperation from SdifF->FrameTypesTable */
  FramT =  SdifGetFrameType(SdifF->FrameTypesTable, FramSignature);
  if (! FramT)
  {
      FramT = SdifCreateFrameType(FramSignature,
		  SdifGetFrameType(gSdifPredefinedTypes->FrameTypesTable, 
				   FramSignature));
      SdifPutFrameType(SdifF->FrameTypesTable, FramT);
  }
  else
  {
      if (SdifTestFrameTypeModifMode(SdifF, FramT) == eFalse)
      {
	  /* Skip frame type def, search '}' */
	  SdifTestCharEnd(SdifF,
	      SdifSkipASCIIUntilfromSdifString(SdifString, &SizeR, "}:[]"),
	      '}', "", eFalse, "end of frame type skiped missing");
	  return SizeR;
      }
  }


  /* Components */
  CharEnd = SdiffGetStringUntilfromSdifString(SdifString, sdifString,
				_SdifStringLen, _SdifReservedChars);
  if (SdifTestCharEnd(SdifF, CharEnd, '{', sdifString, 
		      SdifStrLen(sdifString) != 0, "Frame") == eFalse)
  {
      return SizeR;
  }
  else
  {
      while (SdifFGetOneComponentfromSdifString(SdifF, 
	         SdifString, &MtrxSignature, sdifString) != (unsigned) '}')
      {
          if (SdifTestMatrixType(SdifF, MtrxSignature))
	  {
	      SdifFrameTypePutComponent(FramT, MtrxSignature, sdifString);
	      MtrxSignature = 0;
	  }
      }
  }

  FramT->ModifMode = eNoModif;
  
  return SizeR;
}



/* SdifFGetAllType ne lit pas "1TYP" puisque l'on sera aiguillie sur cette fonction 
 * apres lecture de "1TYP"
 */
size_t
SdifFGetAllType(SdifFileT *SdifF, int Verbose)
{
  int            CharEnd;
  size_t         SizeR = 0;
  SdifSignature  TypeOfType = 0;
  FILE          *file;
  char sdifString[_SdifStringLen];

  file = SdifFGetFILE_SwitchVerbose(SdifF, Verbose);

  if (Verbose != 't')
    _SdifRemark ("Warning, this file uses an intermediate format for "
		 "the user defined types.  Portablity with programs not "
		 "using the IRCAM SDIF library is not guaranteed.");
  
  
  if (SdifF->TypeDefPass != eNotPass)
    _SdifFError(SdifF, eOnlyOneChunkOf, SdifSignatureToString(e1TYP));
  /* Read anyway */
  
  CharEnd = SdiffGetStringUntil(file, sdifString, _SdifStringLen, &SizeR, _SdifReservedChars);
  if (SdifTestCharEnd(SdifF,     CharEnd,    '{',    sdifString, 
		      SdifStrLen(sdifString) != 0,
		      "Begin of Types declarations") == eFalse)
    {
      return SizeR;
    }
  
  
  while( (CharEnd = SdiffGetSignature(file, &TypeOfType, &SizeR)) != (unsigned) '}' )
    {      
      switch (TypeOfType)
      {
	case e1MTD :
	  SizeR += SdifFGetOneMatrixType(SdifF, Verbose);
	  break;
	case e1FTD :
	  SizeR += SdifFGetOneFrameType(SdifF, Verbose);
	  break;
	default :
	{
	  char errorMess[_SdifStringLen];

	  sprintf(errorMess, "Wait '%s' or '%s' : '%s'",
		  SdifSignatureToString(e1MTD),
		  SdifSignatureToString(e1FTD),
		  SdifSignatureToString(TypeOfType));
	  _SdifFError(SdifF, eSyntax, errorMess);
	  SdifTestCharEnd(SdifF,
			  SdifSkipASCIIUntil(file, &SizeR, "}:[]"),
			  '}', "", eFalse,
			  "end of unknown type def skiped missing");
	}
	  break;
      }
      TypeOfType = 0;
    }
  
  if (TypeOfType != 0)
    {
      _SdifFError(SdifF, eSyntax, SdifSignatureToString(TypeOfType));
      return SizeR;
    }
  
  SdifF->TypeDefPass = eReadPass;
  
  return SizeR;
}

  
/*DOC:
  Parse Matrix or Frame type definitions from SdifString 

  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
*/
size_t 
SdifFGetAllTypefromSdifString(SdifFileT *SdifF, SdifStringT *SdifString)
{
    size_t SizeR = 0;
    int CharEnd;
    SdifSignature TypeOfType = 0;

    /* check for alternative interpretation of 1TYP syntax with opening brace */
    if (!SdifStringIsEOS(SdifString)  &&  SdifStringGetC(SdifString) == (unsigned) '{')
	/* first char was '{' from Spear files, skip it */;
    else
	SdifStringUngetC(SdifString);	/* put it back */
	
    /* read until end of string or closing brace */
    while ((CharEnd = SdiffGetSignaturefromSdifString(SdifString, &TypeOfType))
	   != (unsigned) '}'  &&  !SdifStringIsEOS(SdifString))
    {
	switch (TypeOfType)
	{
	  case e1MTD:
	    SizeR += SdifFGetOneMatrixTypefromSdifString(SdifF, SdifString);
	  break;

	  case e1FTD:
	    SizeR += SdifFGetOneFrameTypefromSdifString(SdifF, SdifString);
	  break;

	  default:
	  {
	    char errorMess[_SdifStringLen];

	    sprintf(errorMess, "Waiting for signature '%s' or '%s', read '%s' (end char '%c'=%d) at position %d, \nremaining input '%s'",
		    SdifSignatureToString(e1MTD),
		    SdifSignatureToString(e1FTD),
		    SdifSignatureToString(TypeOfType), CharEnd, CharEnd,
		    SdifString->NbCharRead, 
		    SdifString->str + SdifString->NbCharRead);
	    _SdifFError(SdifF, eSyntax, errorMess);
	    /* return SizeR; */
	  }
	  break;
	}
	TypeOfType = 0;
    }
    return SizeR;
}





int
SdifFGetOneStreamID(SdifFileT *SdifF, int Verbose, size_t *SizeR)
{
  SdifUInt4        NumID;
  char             CharEnd;
  static const char CharsEnd[] = " \t\n\f\r\v{},;:";
  FILE            *file;
	char sdifString[_SdifStringLen];
	char sdifString2[_SdifStringLen];
    char errMess[_SdifStringLen];


  file = SdifFGetFILE_SwitchVerbose(SdifF, Verbose);
  
  
    
  CharEnd = (char) SdiffGetStringUntil(file, sdifString, _SdifStringLen, SizeR, CharsEnd);

  /* test if it's the last or not */
  if ( (CharEnd == '}') && (SdifStrLen(sdifString) == 0) )
    {
      /* no more IDStream */
      return  CharEnd;
    }

  if (! isspace(CharEnd))
    {
      sprintf(errMess,
	      "Wait a space_char after NumId '%s', read char: (%d) '%c'",
	      sdifString, CharEnd, CharEnd);
      _SdifFError(SdifF, eSyntax, errMess);
      if (CharEnd != (unsigned)';')
	SdifTestCharEnd(SdifF,
			SdifSkipASCIIUntil(file, SizeR, ";"),
			';',
			"", eFalse, "end of Stream ID skiped missing");
      return  CharEnd;
    }


  /* ID */
  NumID = atoi(sdifString);
  if (SdifStreamIDTableGetSID(SdifF->StreamIDsTable, NumID))
    {
      sprintf(errMess, "StreamID : %u ", NumID);
      _SdifFError(SdifF, eReDefined, errMess);
      if (CharEnd != (unsigned)';')
	SdifTestCharEnd(SdifF,
			SdifSkipASCIIUntil(file, SizeR, ";"),
			';',
			"", eFalse, "end of Stream ID skiped missing");
      return  CharEnd;
    }
  


  /* source */
  CharEnd = (char) SdiffGetStringUntil(file, sdifString, _SdifStringLen, SizeR, CharsEnd);
  if (SdifTestCharEnd(SdifF, CharEnd, ':', sdifString, eFalse, "Stream ID Source") == eFalse)
    {
      if (CharEnd != (unsigned) ';')
	SdifTestCharEnd(SdifF,
			SdifSkipASCIIUntil(file, SizeR, ";"),
			';',
			"", eFalse, "end of Stream ID skiped missing");
      return  CharEnd;
    }
  

  /* TreeWay : simple string pour le moment */
  CharEnd = (char) SdiffGetStringWeakUntil(file, sdifString2, _SdifStringLen, SizeR, ";");
  /*CharEnd = SdiffGetStringUntil    (file, gSdifString2, _SdifStringLen, SizeR, _SdifReservedChars);*/
  if (SdifTestCharEnd(SdifF, CharEnd, ';', sdifString2, eFalse, "end of Stream ID TreeWay") == eFalse)
    {
      return  CharEnd;
    }
  

  SdifStreamIDTablePutSID(SdifF->StreamIDsTable, NumID, sdifString, sdifString2);
  return  CharEnd;
}


/*DOC:
  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
  Get the current Stream ID from a SdifStringT
*/
int
SdifFGetOneStreamIDfromSdifString(SdifFileT *SdifF, SdifStringT *SdifString)
{
  SdifUInt4        NumID;
  char             CharEnd;
  int              ReturnChar;
  static const char CharsEnd[] =  " \t\n\f\r\v{},;:";
  size_t SizeR = 0;

	char sdifString[_SdifStringLen];
	char sdifString2[_SdifStringLen];
	char errMess[_SdifStringLen];

  ReturnChar = SdiffGetStringUntilfromSdifString(SdifString, sdifString,
					      _SdifStringLen, CharsEnd);




  /* test if it's the last or not */
  if ((ReturnChar == eEof))
    {
      /* no more IDStream */
	CharEnd = (char) eEof;
	return  CharEnd;
    }
  
  CharEnd = (char) ReturnChar;
  if (! isspace(CharEnd))
    {
      sprintf(errMess,
	      "Wait a space_char after NumId '%s', read char: (%d) '%c'",
	      sdifString, CharEnd, CharEnd);
      _SdifFError(SdifF, eSyntax, errMess);
      if (CharEnd != (unsigned)';')
	SdifTestCharEnd(SdifF,
			SdifSkipASCIIUntilfromSdifString(SdifString, &SizeR, ";"),
			';',
			"", eFalse, "end of Stream ID skiped missing");
      return  CharEnd;
    }

  /* ID */
  NumID = atoi(sdifString);
  if (SdifStreamIDTableGetSID(SdifF->StreamIDsTable, NumID))
    {
      sprintf(errMess, "StreamID : %u ", NumID);
      _SdifFError(SdifF, eReDefined, errMess);
      if (CharEnd != (unsigned)';')
	SdifTestCharEnd(SdifF,
			SdifSkipASCIIUntilfromSdifString(SdifString, &SizeR, ";"),
			';',
			"", eFalse, "end of Stream ID skiped missing");
      return  CharEnd;
    }
  


  /* source */
  CharEnd = (char) SdiffGetStringUntilfromSdifString(SdifString, sdifString,
						     _SdifStringLen, CharsEnd);
  if (SdifTestCharEnd(SdifF, CharEnd, ':', sdifString, eFalse, "Stream ID Source") == eFalse)
    {
      if (CharEnd != (unsigned) ';')
	SdifTestCharEnd(SdifF,
			SdifSkipASCIIUntilfromSdifString(SdifString, &SizeR, ";"),
			';',
			"", eFalse, "end of Stream ID skiped missing");
      return  CharEnd;
    }
  

  /* TreeWay : simple string pour le moment */
  CharEnd = (char) SdiffGetStringWeakUntilfromSdifString(SdifString, sdifString2,
							 _SdifStringLen, ";");
  /*CharEnd = SdiffGetStringUntil    (file, gSdifString2, _SdifStringLen, SizeR, _SdifReservedChars);*/
  if (SdifTestCharEnd(SdifF, CharEnd, ';', sdifString2, eFalse, "end of Stream ID TreeWay") == eFalse)
    {
      return  CharEnd;
    }
  

  SdifStreamIDTablePutSID(SdifF->StreamIDsTable, NumID, sdifString, sdifString2);
  return  CharEnd;
}


/*DOC:
  Remark:
         This function implements the old SDIF Specification (before June 1999)
  Get all Stream ID from a file
*/
size_t
SdifFGetAllStreamID(SdifFileT *SdifF, int Verbose)
{
	size_t     SizeR = 0;
	int        CharEnd;
	FILE      *file;

	char sdifString[_SdifStringLen];
 
  file = SdifFGetFILE_SwitchVerbose(SdifF, Verbose);

  if (Verbose != 't')
    _SdifRemark ("Warning, this file uses an intermediate format for "
		 "the stream ID table.  Portablity with programs not "
		 "using the IRCAM SDIF library is not guaranteed.");

  if (SdifF->StreamIDPass != eNotPass)
    _SdifFError(SdifF, eOnlyOneChunkOf, SdifSignatureToString(e1IDS));
  /* Read anyway */
  
  CharEnd = SdiffGetStringUntil(file, sdifString, _SdifStringLen, &SizeR, _SdifReservedChars);
  if (SdifTestCharEnd(SdifF,     CharEnd,    '{',    sdifString, 
		      SdifStrLen(sdifString) != 0,
		      "Begin of StreamID declarations") == eFalse)
    {
      return SizeR;
    }
  else
    {
      while (SdifFGetOneStreamID(SdifF, Verbose, &SizeR) != (int) '}')
	;
    }
  
  SdifF->StreamIDPass = eReadPass; 

  return SizeR;
}


/*DOC:
  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
  Get all Stream ID from a SdifStringT
*/
size_t
SdifFGetAllStreamIDfromSdifString(SdifFileT *SdifF, SdifStringT *SdifString)
{
  size_t SizeR = 0;

  while (!SdifStringIsEOS(SdifString))
    SdifFGetOneStreamIDfromSdifString(SdifF, SdifString);
	 

  
  SdifF->StreamIDPass = eReadPass; 
  
  return SizeR;
}
