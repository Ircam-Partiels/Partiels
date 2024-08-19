/* $Id: SdifRWLowLevel.c,v 3.38 2008-01-11 15:48:29 roebel Exp $
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
 * Read Write Low Level. Machine sex, little-big endian control
 *
 * author: Dominique Virolle 1997
 *
 */

/*
 * $Log: not supported by cvs2svn $
 * Revision 3.37  2007/11/27 12:10:44  roebel
 * Removed unused variables.
 *
 * Revision 3.36  2006/03/22 22:00:41  roebel
 * Fixed wrong sizeof which would work only if
 * sizeof(pointer)  == sizeof(int)
 *
 * Revision 3.35  2005/10/21 14:32:30  schwarz
 * protect all static buffers from overflow by using snprintf instead of sprintf
 * move big errorMess buffers into error branch to avoid too large stack allocation
 *
 * Revision 3.34  2005/05/24 09:36:15  roebel
 *
 * Fixed last checkin comment which turned out to be the start of
 * a c-comment.
 *
 * Revision 3.33  2005/05/23 19:17:53  schwarz
 * - Sdiffread/Sdiffwrite functions with SdifFileT instead of FILE *
 *   -> eof error reporting makes more sense
 * - more cleanup of sdif.h, above functions are private in SdifRWLowLevel.h
 * - eEof becomes error 4 to be distinguishable from ascii chars
 * - SdifFScanNameValueLCurrNVT reimplemented for ascii only
 *
 * Revision 3.32  2005/05/23 17:52:53  schwarz
 * Unified error handling:
 * - SdifErrorEnum (global errors) integrated into SdifErrorTagET (file errors)
 * - no more SdifError.[ch], everything done by SdifErrMess.[ch]
 *
 * Revision 3.31  2005/05/13 16:01:55  schwarz
 * more doc
 *
 * Revision 3.30  2005/04/07 15:56:48  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.29  2004/12/20 12:15:40  roebel
 * Fixed compilation warnings (errors) in gcc (g++) 3.4.2.
 *
 * Revision 3.28  2004/10/07 14:53:10  roebel
 * Fixed SdifStringToNV to only convert \t and \n.
 *
 * Revision 3.27  2004/09/09 17:33:55  schwarz
 * SdiffGetSignaturefromSdifString, SdiffReadSpacefromSdifString,
 * SdiffGetWordUntilfromSdifString:
 * fixed parsing bug that the type definition string had to end with
 * whitespace.
 *
 * At the next bug with the type language parser and tokeniser, it should
 * be rewritten using flex/bison!
 *
 * SdifIsAReservedChar return value changed to boolean flag, much clearer.
 *
 * Revision 3.26  2004/07/22 14:47:56  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.25  2004/07/13 18:03:16  roebel
 * Forget to fix the byteoffset for the return value.
 *
 * Revision 3.24  2004/07/13 15:10:24  roebel
 * Fixed SdiffReadSignature to not override the btesread variable
 * but to add the number of  bytes read.
 *
 * Revision 3.23  2004/06/14 15:52:04  schwarz
 * Fix sdif__foralltypes double-defined error for stricter compilers,
 * such as Codewarrior: Nicolas Ellis redefined that macro locally,
 * without undefining the global definition first.
 *
 * Revision 3.22  2004/06/09 10:54:43  schwarz
 * Fixed compilation problems on Windows (thanks Jean Bresson):
 * - void pointer arithmetic not allowed
 * - open is a Unix system call, use fopen instead
 * - strdup does not exists, so compile with -Dstrdup=_strdup
 *
 * Revision 3.21  2004/06/03 11:18:00  schwarz
 * Profiling showed some waste of cycles in byte swapping and signature reading:
 * - byte swapping now array-wise, not element-wise in SdifSwap* routines:
 *   -> from 0.24 s (18.5%) to 0.14s
 * - ASCII signature reading function SdiffGetSignature replaced by new binary
 *   function SdiffReadSignature (also in SdifFGetSignature, so the change is
 *   mostly transparent):
 *   -> from 0.11 s (9.6%)  to 0.01 s
 * - overall run time improvement with test case sdifextractall_a01:
 *   -> from 1.20 s         to 0.86 s (40% faster)
 *
 * Revision 3.20  2004/06/03 09:16:19  schwarz
 * more efficient byte swapping in SdiffreadLittleEndian[248].
 * realised SdiffReadSignature.
 *
 * Revision 3.19  2004/02/08 14:26:58  ellis
 *
 * now the textual scanner parses correctly character datas
 *
 * Revision 3.18  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.17  2003/07/07 10:27:01  roebel
 * Added support for eInt1 and eUInt1 data types
 *
 * Revision 3.16  2003/05/01 18:50:32  roebel
 * SdifStringToSignature takes now const char * as argument.
 *
 * Revision 3.15  2002/05/24 19:37:52  ftissera
 * Change code to be compatible with C++
 * Cast pointers to correct type.
 *
 * Revision 3.14  2001/05/02 09:34:47  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.13  2000/11/21 16:34:50  roebel
 * New SdifSignatureConst builds integer signature according to
 * endianess of machine. Multicharacter constants are no longer
 * supported by the library. Cleaned up sdif.h/SdifGlobals.h a bit.
 * Test for Multicharacter conversion is removed from configure.in.
 *
 * Revision 3.12  2000/11/15 14:53:34  lefevre
 * no message
 *
 * Revision 3.11  2000/10/27  20:03:42  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.10.2.2  2000/08/21  21:35:44  tisseran
 * *** empty log message ***
 *
 * Revision 3.10.2.1  2000/08/21  18:34:11  tisseran
 * Add SdifSkipASCIIUntilfromSdifString function (same as SdifSkipASCIIUntil).
 * Add SdifFSkip for SdifSkip for (functions SdifSkip doesn't respect function nomemclature => obsolete).
 *
 * Revision 3.10  2000/07/18  15:08:38  tisseran
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
 * Revision 3.9  2000/05/15  16:23:10  schwarz
 * Avoided avoidable warnings.
 *
 * Revision 3.8  1999/10/15  12:23:46  schwarz
 * Added SdifStringToNV.
 *
 * Revision 3.7  1999/10/13  16:05:56  schwarz
 * Changed data type codes (SdifDataTypeET) to SDIF format version 3, as
 * decided with Matt Wright June 1999, added integer data types.
 * Added writing of 1NVT with real frame header (but data is still not in
 * matrices).
 * The data type handling makes heavy use of code-generating macros,
 * called for all data types with the sdif_foralltypes macro, thus
 * adding new data types is easy.
 *
 * Revision 3.6  1999/10/07  15:05:53  schwarz
 * Removed unused SdiffGetPos.
 *
 * Revision 3.5  1999/09/28  13:09:10  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.4  1999/09/23  16:06:50  schwarz
 * Fixed bug with size of signature (was 8 on alpha!).
 *
 * Revision 3.3  1999/09/20  13:23:35  schwarz
 * Optimized SdifStringToSignature.
 *
 * Revision 3.2  1999/08/31  10:05:46  schwarz
 * Extracted function SdifStringToSignature from SdiffGetSignature.
 *
 * Revision 3.1  1999/03/14  10:57:18  virolle
 * SdifStdErr add
 *
 * Revision 2.3  1999/01/23  15:55:59  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:45  virolle
 * General Lists, and special chunk preparation to become frames
 */


#include "sdif_portability.h"

#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <sdif.h>
#include "SdifGlobals.h"
#include "SdifHard_OS.h"
#include "SdifRWLowLevel.h"


extern int gSdifInitialised;		/* can't include SdifFile.h */

#define    _SdifBSLittleE    4096	/* local swap buffers' size */



/* fread encapsulation with error check */
size_t Sdiffread(void *ptr, size_t size, size_t nobj, SdifFileT *file)
{
  size_t nobjread;
  
  nobjread = fread(ptr, size, nobj, file->Stream);
  if (nobjread != nobj)
  {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess), 
	       "Sdiffread %ld", ftell(file->Stream));
      _SdifFError(file, eEof, errorMess);
  }
  
  return nobjread;
}


/* fwrite encapsule */
size_t
Sdiffwrite(void *ptr, size_t size, size_t nobj, SdifFileT *file)
{
  size_t nobjwrite;

  nobjwrite = fwrite(ptr, size, nobj, file->Stream);
  if (nobjwrite != nobj)
  {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess), 
	       "Sdiffwrite %ld", ftell(file->Stream));
      _SdifFError(file, eEof, errorMess);
  }

  return nobjwrite;
}





/****************************************************/
/************ little endian machine *****************/

/* TODO: use hton[sl](3) when available (checked by configure) */

/** fread **/

size_t SdiffreadLittleEndian2 (void *ptr, size_t nobj, SdifFileT *file)
{
    size_t nobjread;
    
    nobjread = Sdiffread(ptr, 2, nobj, file);
    SdifSwap2(ptr, nobjread);

    return nobjread;
}


size_t SdiffreadLittleEndian4 (void *ptr, size_t nobj, SdifFileT *file)
{
    size_t nobjread;
    
    nobjread = Sdiffread(ptr, 4, nobj, file);
    SdifSwap4(ptr, nobjread);

    return nobjread;
}


size_t SdiffreadLittleEndian8 (void *ptr, size_t nobj, SdifFileT *file)
{
    size_t nobjread;
    
    nobjread = Sdiffread(ptr, 8, nobj, file);
    SdifSwap8(ptr, nobjread);

    return nobjread;
}


/* swapping fwrite functions have to copy the data to the 
   buffer sdifLittleToBig and write from there in order to leave the
   caller's data intact */

size_t SdiffwriteLittleEndian2 (void *ptr, size_t nobj, SdifFileT *file)
{
#   define nblock     (_SdifBSLittleE / 2)
    size_t nwritten = 0;
    char sdifLittleToBig [_SdifBSLittleE];

    while (nobj > 0)
    {
	size_t ntowrite = nobj > nblock  ?  nblock  :  nobj;

	SdifSwap2Copy(ptr, sdifLittleToBig, ntowrite);
	nwritten += Sdiffwrite(sdifLittleToBig, 2, ntowrite, file);
	nobj     -= ntowrite; 
	ptr       = (void *)(((char *) ptr) + _SdifBSLittleE);
    }

    return nwritten;
#   undef nblock
}


size_t SdiffwriteLittleEndian4 (void *ptr, size_t nobj, SdifFileT *file)
{
#   define nblock     (_SdifBSLittleE / 4)
    size_t nwritten = 0;
	char sdifLittleToBig [_SdifBSLittleE];

    while (nobj > 0)
    {
	size_t ntowrite = nobj > nblock  ?  nblock  :  nobj;

	SdifSwap4Copy(ptr, sdifLittleToBig, ntowrite);
	nwritten += Sdiffwrite(sdifLittleToBig, 4, ntowrite, file);
	nobj     -= ntowrite; 
	ptr       = (void *)(((char *) ptr) + _SdifBSLittleE);
    }

    return nwritten;
#   undef nblock
}


size_t SdiffwriteLittleEndian8 (void *ptr, size_t nobj, SdifFileT *file)
{
#   define nblock     (_SdifBSLittleE / 8)
    size_t nwritten = 0;
	char sdifLittleToBig [_SdifBSLittleE];

    while (nobj > 0)
    {
	size_t ntowrite = nobj > nblock  ?  nblock  :  nobj;

	SdifSwap8Copy(ptr, sdifLittleToBig, ntowrite);
	nwritten += Sdiffwrite(sdifLittleToBig, 8, ntowrite, file);
	nobj     -= ntowrite; 
	ptr       = (void *)(((char *) ptr) + _SdifBSLittleE);
    }

    return nwritten;
#   undef nblock
}




/***************************************************************/
/***************************************************************/
/* Read */

size_t
SdiffReadChar (SdifChar *ptr, size_t nobj, SdifFileT *file)
{
    return Sdiffread(ptr, sizeof(SdifChar),  nobj, file);
}

size_t
SdiffReadInt1 (SdifInt1 *ptr, size_t nobj, SdifFileT *file)
{
  return Sdiffread(ptr, sizeof(SdifInt1),  nobj, file);
}


size_t
SdiffReadInt2 (SdifInt2 *ptr, size_t nobj, SdifFileT *file)
{
  switch (gSdifMachineType)
    {
    case eLittleEndian   :
    case eLittleEndian64 :
      return SdiffreadLittleEndian2(ptr, nobj, file);
    default :
      return Sdiffread(ptr, sizeof(SdifInt2),  nobj, file);
    }  
}


size_t
SdiffReadUInt1 (SdifUInt1 *ptr, size_t nobj, SdifFileT *file)
{
  return Sdiffread(ptr, sizeof(SdifUInt1),  nobj, file);
}


size_t
SdiffReadUInt2(SdifUInt2 *ptr, size_t nobj, SdifFileT *file)
{
  switch (gSdifMachineType)
    {
    case eLittleEndian   :
    case eLittleEndian64 :
      return SdiffreadLittleEndian2(ptr, nobj, file);
    default :
      return Sdiffread(ptr, sizeof(SdifUInt2),  nobj, file);
    }  
}


size_t
SdiffReadInt4(SdifInt4 *ptr, size_t nobj, SdifFileT *file)
{
  switch (gSdifMachineType)
    {
    case eLittleEndian   :
    case eLittleEndian64 :
      return SdiffreadLittleEndian4(ptr, nobj, file);
    default :
      return Sdiffread(ptr, sizeof(SdifInt4),  nobj, file);
    }  
}


size_t
SdiffReadUInt4(SdifUInt4 *ptr, size_t nobj, SdifFileT *file)
{
  switch (gSdifMachineType)
    {
    case eLittleEndian   :
    case eLittleEndian64 :
      return SdiffreadLittleEndian4(ptr, nobj, file);
    default :
      return Sdiffread(ptr, sizeof(SdifUInt4),  nobj, file);
    }  
}


/*
 *size_t
 *SdiffReadUInt8(SdifUInt8 *ptr, size_t nobj, SdifFileT *file)
 *{
 * switch (gSdifMachineType)
 *   {
 *   case eLittleEndian   :
 *   case eLittleEndian64 :
 *   case eLittleEndianLittleConst :
 *   case eLittleEndianLittleConst64 :
 *     return SdiffreadLittleEndian8(ptr, nobj, file);
 *   default :
 *     return Sdiffread(ptr, sizeof(SdifUInt8),  nobj, file);
 *   }  
 *}
 */


size_t
SdiffReadFloat4(SdifFloat4 *ptr, size_t nobj, SdifFileT *file)
{
  switch (gSdifMachineType)
    {
    case eLittleEndian   :
    case eLittleEndian64 :
      return SdiffreadLittleEndian4(ptr, nobj, file);
    default :
      return Sdiffread(ptr, sizeof(SdifFloat4),  nobj, file);
    }  
}


size_t
SdiffReadFloat8(SdifFloat8 *ptr, size_t nobj, SdifFileT *file)
{
  switch (gSdifMachineType)
    {
    case eLittleEndian   :
    case eLittleEndian64 :
      return SdiffreadLittleEndian8(ptr, nobj, file);
    default :
      return Sdiffread(ptr, sizeof(SdifFloat8),  nobj, file);
    }  
}


SdifErrorTagET SdiffReadSignature (SdifSignature *Signature, SdifFileT *file, size_t *nread)
{
  size_t localread =  fread(Signature, sizeof(SdifSignature), 1, file->Stream);
  
  if (localread  &&  !feof(file->Stream))
    {
      switch (gSdifMachineType)
    	{
	case eLittleEndian   :
	case eLittleEndian64 :
	  SdifSwap4(Signature, 1);
	  break;
    	}
      
      *nread += localread * sizeof(SdifSignature);
      
      /* return success */
      return eNoError;
    }
  else
    { 
      *Signature = eEmptySignature;
      return eEof;
    }
}



/* Write */

size_t
SdiffWriteChar (SdifChar *ptr, size_t nobj, SdifFileT *file)
{
    return Sdiffwrite(ptr, sizeof(SdifChar),  nobj, file);
}

size_t
SdiffWriteInt1 (SdifInt1 *ptr, size_t nobj, SdifFileT *file)
{
    return Sdiffwrite(ptr, sizeof(SdifInt1),  nobj, file);
}

size_t
SdiffWriteInt2 (SdifInt2 *ptr, size_t nobj, SdifFileT *file)
{
  switch (gSdifMachineType)
    {
      
    case eLittleEndian   :
    case eLittleEndian64 :
      return SdiffwriteLittleEndian2(ptr, nobj, file);
      
    default :
      return Sdiffwrite(ptr, sizeof(SdifInt2),  nobj, file);
    }  
}

size_t
SdiffWriteUInt1 (SdifUInt1 *ptr, size_t nobj, SdifFileT *file)
{
    return Sdiffwrite(ptr, sizeof(SdifUInt1),  nobj, file);
}

size_t
SdiffWriteUInt2(SdifUInt2 *ptr, size_t nobj, SdifFileT *file)
{
  switch (gSdifMachineType)
    {
      
    case eLittleEndian   :
    case eLittleEndian64 :
      return SdiffwriteLittleEndian2(ptr, nobj, file);
    default :
      return Sdiffwrite(ptr, sizeof(SdifUInt2),  nobj, file);
    }
    
}


size_t
SdiffWriteInt4(SdifInt4 *ptr, size_t nobj, SdifFileT *file)
{
  switch (gSdifMachineType)
    {
      
    case eLittleEndian   :
    case eLittleEndian64 :
      return SdiffwriteLittleEndian4(ptr, nobj, file);
    default :
      return Sdiffwrite(ptr, sizeof(SdifInt4),  nobj, file);
    }  
}


size_t
SdiffWriteUInt4(SdifUInt4 *ptr, size_t nobj, SdifFileT *file)
{
  switch (gSdifMachineType)
    {
      
    case eLittleEndian   :
    case eLittleEndian64 :
      return SdiffwriteLittleEndian4(ptr, nobj, file);
    default :
      return Sdiffwrite(ptr, sizeof(SdifUInt4),  nobj, file);
    }  
}


/*
 *size_t
 *SdiffWriteUInt8(SdifUInt8 *ptr, size_t nobj, SdifFileT *file)
 *{
 * switch (gSdifMachineType)
 *   {
 *     
 *   case eLittleEndian   :
 *   case eLittleEndian64 :
 *     return SdiffwriteLittleEndian8(ptr, nobj, file);
 *   default :
 *     return Sdiffwrite(ptr, sizeof(SdifUInt8),  nobj, file);
 *   }  
 *}
 */


size_t
SdiffWriteFloat4(SdifFloat4 *ptr, size_t nobj, SdifFileT *file)
{
  switch (gSdifMachineType)
    {
      
    case eLittleEndian   :
    case eLittleEndian64 :
      return SdiffwriteLittleEndian4(ptr, nobj, file);
    default :
      return Sdiffwrite(ptr, sizeof(SdifFloat4),  nobj, file);
    }  
}


size_t
SdiffWriteFloat8(SdifFloat8 *ptr, size_t nobj, SdifFileT *file)
{
  switch (gSdifMachineType)
    {
      
    case eLittleEndian   :
    case eLittleEndian64 :
      return SdiffwriteLittleEndian8(ptr, nobj, file);
    default :
      return Sdiffwrite(ptr, sizeof(SdifFloat8),  nobj, file);
    }  
}


size_t SdiffWriteSignature(SdifSignature *Signature, SdifFileT *file)
{
    SdifSignature SignW;

    switch (gSdifMachineType)
    {     
      case eLittleEndian :
      case eLittleEndian64 :
	  SdifSwap4Copy(Signature, &SignW, 1);
      break;

      default :
	  SignW = *Signature;
      break;
    }

    return Sdiffwrite(&SignW, sizeof(SdifSignature), 1, file);
}


size_t
SdiffWriteString(char* ptr, SdifFileT *file)
{
  return Sdiffwrite(ptr, sizeof(char), SdifStrLen(ptr), file);
}




/*************** ASCII **************/


/* Return true if c is a reserved char. */
int 
SdifIsAReservedChar(char c)
{
  if (memchr(_SdifReservedChars, c, SdifStrLen(_SdifReservedChars)))
      return eTrue;
  else
      return eFalse; 
}


/* Convert str <strong>in place</strong> so that it doesn't contain
  the two reserved charsof NVTs '\t' and '\n' that will be transformed
  to spaces.  
*/
char *
SdifStringToNV (/*in out*/ char *str)
{
    char *s = str;
#if 1
    while (*s)
    {
        if (*s =='\n'  ||  *s =='\t')
	    *s = ' ';
	s++;
    }
#else
    while (*s)
    {
        if (isspace (*s)  ||  iscntrl (*s))
	    *s = '_';
	else if (SdifIsAReservedChar(*s))
	    *s = '.';
	s++;
    }
#endif
    return (str);
}


/* SdiffGetString lit un fichier jusqu'a un caractere reserve,
 * ne rempli s que des caracteres non-espacement,
 * renvoie le caractere reserve,
 * saute les premiers caracteres espacement lus.
 * Il y a erreur 
 *     si fin de fichier
 *  ou si un caractere non-espacement et non-reseve
 *        est lu apres un caractere espacement.
 * ncMax est typiquement strlen(s)+1.
 */
int
SdiffGetString(FILE* fr, char* s, size_t ncMax, size_t *NbCharRead)
{
  int cint;
  char c;
  char *cs;
  int lbool;

  cs = s;
  lbool = 1;
  
  do
    {
      cint = fgetc(fr);
      c = (char) cint;
      (*NbCharRead)++;
    }
  while ( isspace(c) && (!feof(fr)) );


  while ( (ncMax-- > 0) && (!feof(fr)) )
    {
      if (SdifIsAReservedChar(c))
	{
	  *cs++ = '\0';
	  return cint;
	}
      else
	if (isspace(c)) /* end of word */
	  lbool = 0;
	else
	  if (lbool == 0) /* word finished and another word : word cut */
	    {
	      *cs++ = '\0';
	      _SdifError(eWordCut, s);
	      return -1;
	    }
          else
	    *cs++ = c;

      cint=fgetc(fr);
      c = (char) cint;
      (*NbCharRead)++;
    }
  

  if (feof(fr))
    return eEof;
  else
    {
      *cs = '\0';
      _SdifError(eTokenLength, s);
      return eTokenLength;
    }
}


/* Unsafe but optimized version of SdifStringToSignature:
   Exactly 4 chars are considered, so make sure *str has at least that many! 
   The str pointer MUST be longword aligned (to the nearest 4 byte boundary).
*/
SdifSignature
_SdifStringToSignature (const char *str)
{
  assert (gSdifInitialised  &&  "SDIF library not initialised!");

  switch (gSdifMachineType)
  {     
    case eLittleEndian :
    case eLittleEndian64 :
    {
	SdifSignature Signature = *((SdifSignature *) str);
	SdifSwap4((void *) &Signature, 1);
	return (Signature);
    }
	
    default:
	return (*((SdifSignature *) str));
  }
}  


/* Safe version of SdifStringToSignature: str can point to any string
   position of any length.  
*/
SdifSignature
SdifStringToSignature (const char *str)
{
  SdifSignature sigarr = eEmptySignature;  /* force 4 byte alignment */
  char		*sig   = (char *) &sigarr;

  if ((*sig++ = *str++))
    if ((*sig++ = *str++))
      if ((*sig++ = *str++))
	if ((*sig++ = *str++))
	  /* do nothing */;
  return _SdifStringToSignature ((char *) &sigarr);
}  


/* retourne le caractere d'erreur
 *
 * <b>Only to be used for ascii reading in text-to-sdif conversion!</b> 
 * (see SdifTextConv)
 * <b>For binary reading, use SdiffReadSignature!</b>
 */
int
SdiffGetSignature(FILE* fr, SdifSignature *Signature, size_t *NbCharRead)
{
  SdifSignature sig   = eEmptySignature;  /* force 4 byte alignment */
  char		*Name = (char *) &sig;
  char c = 0;
  int cint = 0;
  unsigned int i;
  
  /* skip spaces */
  do
    {
      cint = fgetc(fr); 
      c = (char) cint;
      (*NbCharRead)++;
    }
  while (isspace(c) && (!feof(fr)) );
  
  for (i=0; ((i<4) && (!feof(fr))); i++)
    {
      if ( (SdifIsAReservedChar(c)) || (isspace(c)) )
	break;
      else
	{
	  Name[i] = c;
	  if (i < 4-1)
	    {
	      cint =fgetc(fr);
          c = (char) cint;
	      (*NbCharRead)++;
	    }
	}
    }
  
  if (feof(fr))
    {
      *Signature = eEmptySignature;
      (*NbCharRead)--; /* to not count EOF read */
      return eEof;
    }
  else
    {
      *Signature = _SdifStringToSignature (Name);
      return cint;
    }
}


/*DOC:
  Function return the signature in a SdifString
*/
int
SdiffGetSignaturefromSdifString (SdifStringT *SdifString, 
				 SdifSignature *Signature)
{
  SdifSignature sig = eEmptySignature; /* force 4 byte alignment */
  char *Name = (char *) &sig;
  int   c = eEof;
  int   i;

  /* skip whitespace */
  SdiffReadSpacefromSdifString(SdifString);

  /* string starts now with first non-whitespace character, or EOS:
     read up to 4 characters into sig */
  for (i = 0; i < 4; i++)
  {
      /* get next char */
      if (SdifStringIsEOS(SdifString))
	  break;	/* exit loop on EOS, return last sig char */
      else
	  c = SdifStringGetC(SdifString);

      if (SdifIsAReservedChar(c)  ||  isspace(c))
	  break;	/* exit loop on non-sig char, return it */
      else
	  Name[i] = c;
  }

  /* return all we have as signature */
  *Signature = _SdifStringToSignature(Name);
  return c;	/* last char of signature, if it was complete 4 bytes
		   or on EOS, or the non-sig char after signature */
}


size_t
SdiffReadSpace(FILE* fr)
{
  size_t NbCharRead = 0;
  char c;

  while ( isspace(c= (char) fgetc(fr)) )
  {
    NbCharRead++;
  }

  if (feof(fr))
    _SdifError(eEof, "SdiffReadSpace");
  else
    {
/*
      SdiffPosT Pos = 0;
      SdiffGetPos(fr, &Pos);
      Pos --;
      if (SdiffSetPos(fr, &Pos)==0) ::: it is a bad solution with stdin, virolle 99/01 
*/
      if ((c = (char) ungetc(c, fr)))
	  return NbCharRead;
      else
      {
	  char errorMess[_SdifStringLen];

	  snprintf(errorMess, sizeof(errorMess), 
		   "ungetc failed : (%d,%c) ", c, c);
	  _SdifError(eEof, errorMess);
      }
    }
  return NbCharRead;
}


/* skip whitespace in string,

   POSTCONDITION:
   string starts with first non-whitespace character,
   or EOS condition is set
*/
size_t
SdiffReadSpacefromSdifString(SdifStringT *SdifString)
{
  char   c;
  int    eos;
  size_t NbCharRead = 0;
  
  /* need to store end-of-string, because it is changed by subsequent getc */
  while (!(eos = SdifStringIsEOS(SdifString))  &&  
	 isspace(c = SdifStringGetC(SdifString)))
  {
      NbCharRead++;
  }

  if (eos)	/* end of string, there were no non-whitespace characters */
      return NbCharRead; 
  else
  {
      if (SdifStringUngetC(SdifString))
	  return NbCharRead;
      else
      {
	  char   errorMess[_SdifStringLen];

	  snprintf(errorMess, sizeof(errorMess), 
		   "ungetc failed : (%d,%c) ", c, c);
	  _SdifError(eEof, errorMess);
      }
  }
  return NbCharRead;
}


/* fGetWordUntil : read word until a char which is a member of "CharsEnd".
 * the file "fr" must be set at the begining of the word (spaces before not read).
 * the spaces chars between the word and final char are ignored.
 * it return the final char.
 * "(*NbCharRead)" is increased by each byte read into "fr".
 */
int
SdiffGetWordUntil(FILE* fr, char* s, size_t ncMax, size_t *NbCharRead, const char *CharsEnd)
{
  char c=0;
  char *cs;
  int CharsEndLen;
  
  CharsEndLen = SdifStrLen(CharsEnd);
  cs = s;
  
  while( (c = (char) fgetc(fr)) && (ncMax-- > 0) && (!feof(fr)))
    {
      (*NbCharRead)++;
      if (memchr(CharsEnd, c, CharsEndLen))
	{
	  *cs = '\0';
	  return (int) c;
	}

      if (isspace(c))
	{
	  *NbCharRead += SdiffReadSpace(fr);
	  c = (char) fgetc(fr);
	  if (memchr(CharsEnd, c, CharsEndLen))
	    {
	      *cs = '\0';
	      return (int) c;
	    }
	  else
	    {
	      *cs++ = '\0';
	      /*_SdifError(eWordCut, s);*/
	      return -1;
	    }
	}

      *cs++ = c;
    }
    
  if (feof(fr))
    return eEof;
  
  if (ncMax <= 0)
    {
      *cs = '\0';
      _SdifError(eTokenLength, s);
      return eTokenLength;
    }
  return eFalse;
}


/* fGetWordUntil : read word until a char which is a member of "CharsEnd".
 * the string (SdifString->str) must be set at the begining of the word
 * (spaces before not read).
 * the spaces chars between the word and final char are ignored.
 * it return the final char.
 */
int
SdiffGetWordUntilfromSdifString(SdifStringT *SdifString, char* s, size_t ncMax,const char *CharsEnd)
{
  char c=0;
  char *cs;
  int CharsEndLen;
  size_t SizeR = 0;
  
  CharsEndLen = SdifStrLen(CharsEnd);
  cs = s;
  
  while (!SdifStringIsEOS(SdifString)
	 &&  (c = (char) SdifStringGetC(SdifString))
	 &&  ncMax-- > 0)
  {
      if (memchr(CharsEnd, c, CharsEndLen))
      {
	  *cs = '\0';
	  return (int) c;
      }

      if (isspace(c))
      {
	  SizeR += SdiffReadSpacefromSdifString(SdifString);
	  c = (char) SdifStringGetC(SdifString);

	  if (memchr(CharsEnd, c, CharsEndLen))
	  {
	      *cs = '\0';
	      return (int) c;
	  }
	  else
	  {
	      *cs++ = '\0';
	      /*_SdifError(eWordCut, s);*/
	      return -1;
	  }
      }
      
      *cs++ = c;
  }
    
  if (SdifStringIsEOS(SdifString))
      return eEof;
  
  if (ncMax <= 0)
  {
      *cs = '\0';
      _SdifError(eTokenLength, s);
      return eTokenLength;
  }

  return eFalse;
}


/* fGetStringUntil : is like fGetWordUntil.
 * but the space chars before the word are ignored.
 */
int
SdiffGetStringUntil(FILE* fr, char* s, size_t ncMax, size_t *NbCharRead, const char *CharsEnd)
{
  *NbCharRead += SdiffReadSpace(fr);
  return SdiffGetWordUntil(fr, s, ncMax, NbCharRead, CharsEnd);
}

/* fGetStringUntil : is like fGetWordUntil.
 * but the space chars before the word are ignored.
 */
int
SdiffGetStringUntilfromSdifString(SdifStringT *SdifString, char *s, size_t ncMax, const char *CharsEnd)
{
  size_t SizeR = 0;
  SizeR += SdiffReadSpacefromSdifString(SdifString);
  return SdiffGetWordUntilfromSdifString(SdifString, s, ncMax, CharsEnd);
}


/* fGetStringWeakUntil : is like fGetWordUntil.
 * but all chars (spaces too) are valid before CharsEnd.
 */
int
SdiffGetStringWeakUntil(FILE* fr, char* s, size_t ncMax, size_t *NbCharRead, const char *CharsEnd)
{
  char c=0;
  char *cs;
  int CharsEndLen;
  
  CharsEndLen = SdifStrLen(CharsEnd);
  cs = s;
  
  while( (c = (char) fgetc(fr)) && (ncMax-- > 0) && (!feof(fr)))
    {
      (*NbCharRead)++;
      
      if (memchr(CharsEnd, c, CharsEndLen))
	{
	  *cs = '\0';
	  return (int) c;
	}
      *cs++ = c;
    }
  
  if (feof(fr))
    return eEof;
  
  if (ncMax <= 0)
    {
      *cs = '\0';
      _SdifError(eTokenLength, s);
      return eTokenLength;
    }
  return eFalse;
}


int
SdiffGetStringWeakUntilfromSdifString(SdifStringT *SdifString, char* s,
				      size_t ncMax, const char *CharsEnd)
{
  char c=0;
  char *cs;
  int CharsEndLen;
  
  CharsEndLen = SdifStrLen(CharsEnd);
  cs = s;
  
  while( (c = (char) SdifStringGetC(SdifString))
	 && (ncMax-- > 0)
	 && (!SdifStringIsEOS(SdifString)))
    {
      if (memchr(CharsEnd, c, CharsEndLen))
	{
	  *cs = '\0';
	  return (int) c;
	}
      *cs++ = c;
    }
  
  if (SdifStringIsEOS(SdifString))
    return eEof;
  
  if (ncMax <= 0)
    {
      *cs = '\0';
      _SdifError(eTokenLength, s);
      return eTokenLength;
    }
  return eFalse;
}




/* read ASCII */

/* static const char *formatText	  = "%c";   todo */
static const char *formatChar     = " '%c'";	/*TODO: this scan format doesn't work for spaces in char matrices!!! */
static const char *formatFloat4   = "%f";
static const char *formatFloat8   = "%lf";
static const char *formatInt1     = "%hhi";
static const char *formatInt2     = "%hi";
static const char *formatInt4     = "%i";
static const char *formatUInt1    = "%hhu";
static const char *formatUInt2    = "%hu";
static const char *formatUInt4    = "%u";
/* l or ll?
static const char *formatInt8     = "%li";
static const char *formatUInt8    = "%lu";
*/

/* scan function template for type of SdifDataTypeET */
#define scan(type)   sdif_scanproto(type)			    \
{ 								    \
    size_t iobj, NbObjR = 0;					    \
								    \
    for (iobj = 0; iobj < nobj; iobj++)				    \
        NbObjR += fscanf (stream, format##type, &(ptr [iobj]));	    \
    return NbObjR;						    \
}

/* generate scan functions */

/* define new local generating macro, called by sdif_foralltypes:
   Not for all types, char is special */
#undef  sdif__foralltypes
#define sdif__foralltypes(macro, post)  macro(Float4)post \
                                        macro(Float8)post \
                                        macro(Int1  )post \
                                        macro(Int2  )post \
                                        macro(Int4  )post \
                                        macro(UInt1 )post \
                                        macro(UInt2 )post \
                                        macro(UInt4 )post \
                                        macro(Char  )post 
/* generate functions */
sdif_foralltypes (scan)

/* hide new local definition, to avoid misunderstandings */
#undef  sdif__foralltypes


int
SdifSkipASCIIUntil(FILE* fr, size_t *NbCharRead, char *CharsEnd)
{
  int c=0;
  int CharsEndLen;
  
  CharsEndLen = SdifStrLen(CharsEnd);
  
  while(   (c = fgetc(fr)) && (!feof(fr))   )
    {
      (*NbCharRead)++;
      
      if (memchr(CharsEnd, (char) c, CharsEndLen))
	return c;
      else
	if ((!isspace(c)) && (!isprint(c)))
	  {
	    break;
	  }
    }

  if (feof(fr))
    return eEof;
  else
    return c;

}


int
SdifSkipASCIIUntilfromSdifString(SdifStringT *SdifString, size_t *NbCharRead, char *CharsEnd)
{
  int c=0;
  int CharsEndLen;
  
  CharsEndLen = SdifStrLen(CharsEnd);
  
  while(   (c = SdifStringGetC(SdifString)) && (!SdifStringIsEOS(SdifString))   )
    {
      (*NbCharRead)++;
      
      if (memchr(CharsEnd, (char) c, CharsEndLen))
	return c;
      else
	if ((!isspace(c)) && (!isprint(c)))
	  {
	    break;
	  }
    }

  if (SdifStringIsEOS(SdifString))
    return eEof;
  else
    return c;
}
