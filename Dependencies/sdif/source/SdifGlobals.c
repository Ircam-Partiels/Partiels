/* $Id: SdifGlobals.c,v 3.22 2007-11-26 18:19:49 roebel Exp $
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
 * SdifGlobals.c
 *
 *
 * author: Dominique Virolle 1997
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.21  2007/03/21 19:44:15  roebel
 * Don't use global variables without initialization. These globals are treated differently
 * on MacOSX and they are not allowed in dynamic libraries without extra flags.
 * To simplify the situation I now initialized all global variables
 * or make them static.
 *
 * Revision 3.20  2005/04/07 15:56:47  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.19  2004/09/09 17:38:38  schwarz
 * moved SdifSizeOf* functions from SdifFWrite.c to SdifGlobals.c
 * padding test with ftell now only when compiling in debug mode
 *
 * Revision 3.18  2004/07/28 14:56:58  roebel
 * Simplified global list initialisation.
 *
 * Revision 3.17  2004/07/27 18:58:37  roebel
 * Fixed infinite recursion in FreeGlobals.
 *
 * Revision 3.16  2004/07/22 14:47:56  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.15  2004/06/03 11:18:00  schwarz
 * Profiling showed some waste of cycles in byte swapping and signature reading:
 * - byte swapping now array-wise, not element-wise in SdifSwap<N>[Copy] routines:   -> from 0.24 s (18.5%) to 0.14s
 * - ASCII signature reading function SdiffGetSignature replaced by new binary
 *   function SdiffReadSignature (also in SdifFGetSignature, so the change is
 *   mostly transparent):
 *   -> from 0.11 s (9.6%)  to 0.01 s
 * - overall run time improvement with test case sdifextractall_a01:
 *   -> from 1.20 s         to 0.86 s (40% faster)
 *
 * Revision 3.14  2004/05/03 18:07:26  schwarz
 * Fixed bugs in padding calculation for ascii chunks:
 * 1. DON'T PAD FRAMES!
 * 2. SdifFReadMatrixHeader already accounts for read signature
 * Now, calculating padding from ftell is redundant, but we leave it in,
 * with a warning, until everyone's code is tested.
 *
 * Revision 3.13  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.12  2003/07/18 21:08:55  roebel
 * Added new datatypes eInt1 and eUInt1 to SdifDataTypeKnown.
 *
 * Revision 3.11  2001/05/02 09:34:43  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.10  2001/02/08 15:26:56  tisseran
 * Add a test on memory allocation in SdifStringNew (call perror if malloc return a NULL pointer).
 * Note on sdifextract.c:
 * 	- Rechange SdifSkipFrameData to SdifFSkipFrameData (change by axel have disappeared)
 *
 * Revision 3.9  2000/11/21 16:34:49  roebel
 * New SdifSignatureConst builds integer signature according to
 * endianess of machine. Multicharacter constants are no longer
 * supported by the library. Cleaned up sdif.h/SdifGlobals.h a bit.
 * Test for Multicharacter conversion is removed from configure.in.
 *
 * Revision 3.8  2000/11/21 14:51:49  schwarz
 * - sdif.h is now included by all sdif/Sdif*.c files.
 * - Removed all public typedefs, enums, structs, and defines from the
 *   individual sdif/Sdif*.h files, because they were duplicated in sdif.h.
 * - Todo: Do the same for the function prototypes, decide which types and
 *   prototypes really need to be exported.
 * - Removed SdifFileStruct.h.
 * - Preliminary new version of SdiffGetPos, SdiffSetPos.  They used the
 *   type fpos_t, which is no longer a long on RedHat 7 Linux.
 *
 * Revision 3.7  2000/11/15 14:53:29  lefevre
 * no message
 *
 * Revision 3.6  2000/10/27  20:03:33  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.5.2.1  2000/08/21  21:35:23  tisseran
 * *** empty log message ***
 *
 * Revision 3.5  2000/04/11  14:44:10  schwarz
 * Use Char for Text in SDIF types.
 *
 * Revision 3.4  1999/10/13  16:05:48  schwarz
 * Changed data type codes (SdifDataTypeET) to SDIF format version 3, as
 * decided with Matt Wright June 1999, added integer data types.
 * Added writing of 1NVT with real frame header (but data is still not in
 * matrices).
 * The data type handling makes heavy use of code-generating macros,
 * called for all data types with the sdif_foralltypes macro, thus
 * adding new data types is easy.
 *
 * Revision 3.3  1999/09/28  13:09:00  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.2  1999/06/18  16:23:55  schwarz
 * SdifSignatureCmpNoVersion dropped LAST byte on alpha, because the mask
 * 0x00ffffff was not byteswapped.  Introduced gSdifSignatureVersionMask,
 * which is initialised in SdifInitMachineType to appropriate value.
 *
 * Revision 3.1  1999/03/14  10:56:57  virolle
 * SdifStdErr add
 *
 * Revision 2.3  1999/01/23  15:55:49  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:34  virolle
 * General Lists, and special chunk preparation to become frames
 *
 * Revision 2.1  1998/12/21  18:27:22  schwarz
 * Inserted copyright message.
 *
 * Revision 2.0  1998/11/29  11:41:47  virolle
 * - New management of interpretation errors.
 * - Alignement of frames with CNMAT (execpt specials Chunk 1NVT, 1TYP, 1IDS).
 * _ Sdif Header File has a Sdif format version.
 * - Matrices order in frames is not important now. (only one occurence of
 *   a Matrix Type in a Frame Type declaration )
 * - Hard coded predefined types more dynamic management.
 * - Standart streams (stdin, stdout, stderr) set as binary for Windows32 to
 *   have exactly the same result on each plateforme.
 *
 * Revision 1.3  1998/04/24  12:40:33  schwarz
 * Made char * arguments constant for SdifNameValuesLPut and functions called by it.
 *
 */

#include "sdif_portability.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "SdifGlobals.h"
#include "SdifHard_OS.h"


/* backward compatibility; these global buffers should not be used anymore */
char gSdifString[_SdifStringLen] = "unused buffer";
char gSdifString2[_SdifStringLen] = "unused buffer";
char gSdifErrorMess[_SdifStringLen] = "unused buffer";

/* these are not used */
/*
char gSdifStringSignature[_SdifNbMaxPrintSignature][5];
int CurrStringPosSignature = 0;
*/

#ifdef HAVE_PTHREAD

static pthread_key_t tGlobalsKey;

#else

struct SdifGlobals gGlobals;

/*char *gSdifStringSignature[_SdifNbMaxPrintSignature] = gGlobals.stringSignature;
int  CurrStringPosSignature = gGlobals.currStringPosSignature;*/

#endif

/* [_SdifNbMaxPrintSignature] : it's possible to print  _SdifNbMaxPrintSignature
 * signatures on stream in an unique expression but not more.
 */

/*static*/ 


struct SdifGlobals* GetSdifGlobals(){
#ifndef HAVE_PTHREAD
	return &gGlobals;
#else
	void* ptr;
	if((ptr = pthread_getspecific(tGlobalsKey)) == NULL){
		
		ptr = calloc(1,sizeof(struct SdifGlobals));
		pthread_setspecific(tGlobalsKey,ptr);
		SdifInitListNStock(&(((struct SdifGlobals*)ptr)->sdifListNodeStock),
				   _SdifListNodeStockSize);		
	}
	return (struct SdifGlobals *) ptr;
#endif
}

void FreeGlobals(void *inGlobals){
	struct SdifGlobals* globals = (struct SdifGlobals*) inGlobals;	
	SdifListNStockMakeEmpty(&globals->sdifListNodeStock);
	free(globals);
}

#ifdef HAVE_PTHREAD
void    SdifCreateThreadKeys() {
  pthread_key_create(&tGlobalsKey,FreeGlobals);
}
#endif

char *SdifSignatureToString (SdifSignature Signature)
{
    char *pS, *Ret;

	struct SdifGlobals *globals = GetSdifGlobals();

    switch (gSdifMachineType)
    {     
      case eLittleEndian :
      case eLittleEndian64 :
	  SdifSwap4(&Signature, 1);
      break;
    }

    pS = (char *) &Signature;
    globals->stringSignature[globals->currStringPosSignature][0] = pS[0];
    globals->stringSignature[globals->currStringPosSignature][1] = pS[1];
    globals->stringSignature[globals->currStringPosSignature][2] = pS[2];
    globals->stringSignature[globals->currStringPosSignature][3] = pS[3];
    globals->stringSignature[globals->currStringPosSignature][4] = 0;

    Ret = globals->stringSignature[globals->currStringPosSignature];
    globals->currStringPosSignature = (globals->currStringPosSignature + 1) % _SdifNbMaxPrintSignature;
    return Ret;
}


int
SdifSignatureCmpNoVersion(SdifSignature Signature1, SdifSignature Signature2)
{
  SdifUInt4 S1, S2;

  S1 = Signature1 & gSdifSignatureVersionMask;
  S2 = Signature2 & gSdifSignatureVersionMask;

  return (S1 == S2);
}


SdifUInt4 SdifSizeofDataType (SdifDataTypeET DataType)
{
    /* last byte of data type code defines element size in bytes */
    return (DataType & 0xff);
}


int 
SdifDataTypeKnown (SdifDataTypeET DataType)
{
    switch (DataType)
    {
        case eChar  :	  /* same as case eText  : */
        case eFloat4:
        case eFloat8:
        case eInt1  :
        case eInt2  :
        case eInt4  :
        case eUInt1 :
        case eUInt2 :
        case eUInt4 :
	    return eTrue;

	/* defined in sdif format standard ver. 3 but not yet implemented */
        case eInt8  :
        case eUInt8 :
	    _SdifRemark ("standard datatypes Int8, UInt8 not yet handled");
	/* completely unknown */
	default:
	    return eFalse;
    }
}



size_t SdifPaddingCalculate(size_t NbBytes)
{
  size_t mod = NbBytes & _SdifPaddingBitMask;

  return mod ? (_SdifPadding - mod) : 0;
}




size_t SdifFPaddingCalculate(FILE *f, size_t NbBytes)
{

#if defined(DEBUG)  &&  DEBUG
  if ((f != stdin) && (f != stdout) && (f != stderr))
  {
      /* calculating padding from ftell is redundant, but we leave it in,
	 with a warning, until everyone's code is tested.
	 calculating from NbBytes is more efficient
      */
      int efficient = SdifPaddingCalculate(NbBytes);
      int redundant = SdifPaddingCalculate(ftell(f));

      if (efficient != redundant)
	  _Debug("Attention: Your code calculates the wrong padding!\n"
		 "Please check calls to SdifFPaddingCalculate\n");

      return redundant;
  }
  else
#endif
      return SdifPaddingCalculate(NbBytes);
}


int
SdifFloat8Equ(SdifFloat8 f1, SdifFloat8 f2)
{
  if (f1 >= f2)
    if ((f1 - f2)  < _SdifFloatEps)
      return 1;
    else
      return 0;
  else
    return SdifFloat8Equ(f2, f1);
}
