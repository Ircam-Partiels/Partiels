/* $Id: SdifHard_OS.c,v 3.18 2012-01-02 23:49:08 roebel Exp $
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
 */

/* author: Dominique Virolle 1998
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.17  2007/03/21 19:44:15  roebel
 * Don't use global variables without initialization. These globals are treated differently
 * on MacOSX and they are not allowed in dynamic libraries without extra flags.
 * To simplify the situation I now initialized all global variables
 * or make them static.
 *
 * Revision 3.16  2005/05/23 17:52:53  schwarz
 * Unified error handling:
 * - SdifErrorEnum (global errors) integrated into SdifErrorTagET (file errors)
 * - no more SdifError.[ch], everything done by SdifErrMess.[ch]
 *
 * Revision 3.15  2005/04/07 15:56:47  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.14  2004/09/09 17:42:43  schwarz
 * swapping code alternatives
 *
 * Revision 3.13  2004/06/03 11:18:00  schwarz
 * Profiling showed some waste of cycles in byte swapping and signature reading:
 * - byte swapping now array-wise, not element-wise in SdifSwap<N>[Copy] routines:   -> from 0.24 s (18.5%) to 0.14s
 * - ASCII signature reading function SdiffGetSignature replaced by new binary
 *   function SdiffReadSignature (also in SdifFGetSignature, so the change is
 *   mostly transparent):
 *   -> from 0.11 s (9.6%)  to 0.01 s
 * - overall run time improvement with test case sdifextractall_a01:
 *   -> from 1.20 s         to 0.86 s (40% faster)
 *
 * Revision 3.12  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.11  2003/06/06 10:25:44  schwarz
 * Added eReadWriteFile that eventually opens a file in read-write mode.
 *
 * Revision 3.10  2002/05/24 19:37:52  ftissera
 * Change code to be compatible with C++
 * Cast pointers to correct type.
 *
 * Revision 3.9  2001/05/02 09:34:44  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.8  2000/11/21 16:34:49  roebel
 * New SdifSignatureConst builds integer signature according to
 * endianess of machine. Multicharacter constants are no longer
 * supported by the library. Cleaned up sdif.h/SdifGlobals.h a bit.
 * Test for Multicharacter conversion is removed from configure.in.
 *
 * Revision 3.7  2000/11/15 14:53:30  lefevre
 * no message
 *
 * Revision 3.6  2000/10/27  20:03:34  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.5.2.2  2000/08/21  21:35:25  tisseran
 * *** empty log message ***
 *
 * Revision 3.5.2.1  2000/08/21  14:04:19  tisseran
 * *** empty log message ***
 *
 * Revision 3.5  2000/07/06  19:01:48  tisseran
 * Add function for frame and matrix type declaration
 * Remove string size limitation for NameValueTable
 * TODO: 1TYP and 1IDS frame must contain an 1NVT (text) matrix
 *       Actually, data are written with fprintf.
 *
 * Revision 3.4  1999/10/07  15:12:24  schwarz
 * Added isSeekable flag in SdifFileT struct.  This allows to simplify the
 * many tests for stdio on opening the stream.
 * Added SdifStrEq utility function.
 *
 * Revision 3.3  1999/09/28  13:09:01  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.2  1999/06/18  16:23:57  schwarz
 * SdifSignatureCmpNoVersion dropped LAST byte on alpha, because the mask
 * 0x00ffffff was not byteswapped.  Introduced gSdifSignatureVersionMask,
 * which is initialised in SdifInitMachineType to appropriate value.
 *
 * Revision 3.1  1999/03/14  10:56:59  virolle
 * SdifStdErr add
 *
 * Revision 2.4  1999/02/28  12:16:47  virolle
 * memory report
 *
 * Revision 2.3  1999/01/23  15:55:51  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:36  virolle
 * General Lists, and special chunk preparation to become frames
 *
 *
 */


#include "sdif_portability.h"

#include "SdifGlobals.h"
#include "SdifHard_OS.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#if defined( _WIN32) || defined(WIN32)
#include <io.h>
#include <fcntl.h>
#endif


/* To know what hardware we have (check endian and sizeof long ) 
   Endian and size of long are checked in ./configure => config.h */
SdifMachineET
SdifGetMachineType(void)
{
  unsigned int MachineType;
/*    long *LongTest; */
/*    char CharTab[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}; */

  /* for 2nd part of tests if Little */
/*    const char ConstCharTab[] = {'A', 'B', 'C', 'D'}; */
/*    const unsigned int ConstDCBA = 'DCBA'; */
/*    unsigned int *ConstUIntTest; */

/*    LongTest = (long*) CharTab; */

/*#ifdef _LONG64BITS_
  switch (*LongTest)
  {
  case 0x0102030405060708 :
  MachineType = eBigEndian64;
  break;
  case 0x0807060504030201 :
  MachineType = eLittleEndian64;
  break;
  default :
  MachineType = eUndefinedMachine;
  break;
  }
  
  #else
  switch (*LongTest)
  {
  case 0x01020304 :
  MachineType = eBigEndian;
  break;
  case 0x04030201 :
  MachineType = eLittleEndian;
  break;
  default :
  MachineType = eUndefinedMachine;
  break;
  }
  #endif */ /* _LONG64BITS */
  
/* WORDS_BIG_ENDIAN and SIZEOF_LONG are defined in config.h */
#ifdef WORDS_BIGENDIAN
  if (SIZEOF_LONG == 4)
      MachineType = eBigEndian;
  else
      MachineType = eBigEndian64;
#else /* LITTLEENDIAN */
  if (SIZEOF_LONG == 4)
      MachineType = eLittleEndian;
  else
      MachineType = eLittleEndian64;
#endif /* WORDS_BIGENDIAN */
  
/*    if (    (MachineType == eLittleEndian) */
/*         || (MachineType == eLittleEndian64)  ) */
/*      { */
/*        there is supposed that sizeof(int)== 4 */
/*        ConstUIntTest = (unsigned int*) ConstCharTab; */
/*        if ((*ConstUIntTest) == ConstDCBA) */
/*          { */
/*            if (MachineType == eLittleEndian) */
/*              MachineType = eLittleEndianLittleConst; */
/*            else */
/*            if (MachineType == eLittleEndian64) */
/*              MachineType = eLittleEndianLittleConst64; */
/*          } */
/*      } */
  return (SdifMachineET) MachineType;
}




SdifMachineET gSdifMachineType          = eUndefinedMachine;
SdifUInt4     gSdifSignatureVersionMask = 0;



SdifMachineET
SdifInitMachineType(void)
{
  gSdifMachineType = SdifGetMachineType();

  switch (gSdifMachineType)
  {     
    case eLittleEndian :
    case eLittleEndian64 :
      gSdifSignatureVersionMask = 0xffffff00;
    break;

    default :
      gSdifSignatureVersionMask = 0x00ffffff;
    break;
  }

  return gSdifMachineType;
}




/*
 *  Big endian <-> little endian byte swapping
 */

#define SWAP2(x)  ((((x) >> 8) & 0xFF) + (((x) & 0xFF) << 8))

/* 2 byte array swapping in place */
void SdifSwap2 (void *ptr, size_t num)
{
    SdifUInt2  temp;
    SdifUInt2 *iptr = (SdifUInt2 *) ptr;

    while (num > 0)
    {
	num--;
	temp      = iptr[num];
	iptr[num] = SWAP2(temp);
    }
}


/* 2 byte array swapping with copy */
void SdifSwap2Copy (void *src, void *dest, size_t num)
{
    SdifUInt2 *isrc  = (SdifUInt2 *) src;
    SdifUInt2 *idest = (SdifUInt2 *) dest;

    while (num > 0)
    {
	num--;
	idest[num] = SWAP2(isrc[num]);
    }
}


/* testing revealed that swap with '+' is slightly faster than with '|'! */
#define SWAP4(x)    ((((x) >> 24) & 0xFF)  + (((x) >> 8) & 0xFF00) + \
		     (((x) & 0xFF00) << 8) + (((x) & 0xFF) << 24))

/* 4 byte array swapping in place */
void SdifSwap4 (void *ptr, size_t num)
{
    SdifUInt4  temp;
    SdifUInt4 *iptr = (SdifUInt4 *) ptr;

    while (num > 0)
    {
	num--;
	temp      = iptr[num];
	iptr[num] = SWAP4(temp);
    }
}


/* 4 byte array swapping with copy */
void SdifSwap4Copy (void *src, void *dest, size_t num)
{
    SdifUInt4 *isrc  = (SdifUInt4 *) src;
    SdifUInt4 *idest = (SdifUInt4 *) dest;

    while (num > 0)
    {
	num--;
	idest[num] = SWAP4(isrc[num]);
    }
}


#define SWAP8CHARWISE 1

/* 8 byte array swapping in place 
   There is no clear speed difference between char swapping and int swapping */
void SdifSwap8 (void *ptr, size_t num)
{
#if SWAP8CHARWISE
#   define SWAP(i,j)	temp    = cptr[i]; \
			cptr[i] = cptr[j]; \
			cptr[j] = temp

    unsigned char  temp;
    unsigned char *cptr = (unsigned char *) ptr + 8 * num;

    while (num > 0)
    {
	num--;
	cptr -= 8;

	SWAP(0, 7);
	SWAP(1, 6);
	SWAP(2, 5);
	SWAP(3, 4);
    }
#else
    SdifUInt4  temp;
    SdifUInt4 *iptr = (SdifUInt4 *) ptr;

    num *= 2;

    while (num > 0)
    {
	num -= 2;
	temp	      = SWAP4(iptr[num + 1]);
	iptr[num + 1] = SWAP4(iptr[num]);
	iptr[num]     = temp;
    }
   
#endif
}


/* 8 byte array swapping with copy */
void SdifSwap8Copy (void *src, void *dest, size_t num)
{
    unsigned char *psrc  = (unsigned char *) src  + 8 * num;
    unsigned char *pdest = (unsigned char *) dest + 8 * num;

    while (num > 0)
    {
	num--;
	psrc  -= 8;
	pdest -= 8;

	pdest[0] = psrc[7];
	pdest[1] = psrc[6];
	pdest[2] = psrc[5];
	pdest[3] = psrc[4];
	pdest[4] = psrc[3];
	pdest[5] = psrc[2];
	pdest[6] = psrc[1];
	pdest[7] = psrc[0];
    }
}




/*
 *  Standard C-lib utility function wrappers
 */

int
SdifStrLen(const char *s)
{
  return (int) strlen(s);
}


int
SdifStrCmp(const char *s1, const char *s2)
{
  return strcmp(s1, s2);
}


int
SdifStrEq(const char *s1, const char *s2)
{
  return strcmp(s1, s2) == 0;
}


int
SdifStrNCmp(const char *s1, const char *s2, unsigned int n)
{
  return strncmp(s1, s2, n);
}



char*
SdifStrNCpy(char *s1, const char *s2, unsigned int n)
{
  return strncpy (s1, s2, n);
}




char*
SdifCreateStrNCpy(const char* Source, size_t Size)
{
  char *NewString;

  NewString = SdifCalloc(char, Size);
  if (NewString)
    {
      SdifStrNCpy(NewString, Source, Size);
      
      return NewString;
    }
  else
    {
      _SdifError(eAllocFail, Source);
      return NULL;
    }
}






void
SdifKillStr(char* String)
{
  if (String)
    SdifFree(String);
  else
    _SdifError(eFreeNull, "String free");
}





/* Only for WIN32 */
void SdifSetStdIOBinary (void)
{
#if defined( _WIN32) || defined(WIN32)
    _setmode( _fileno( stdin  ), _O_BINARY );
    _setmode( _fileno( stdout ), _O_BINARY );
    _setmode( _fileno( stderr ), _O_BINARY );
#endif
}







FILE* SdiffBinOpen(const char * Name, SdifBinaryModeET Mode)
{
    switch (Mode)
    {
    case eBinaryModeRead :
        return fopen (Name, "rb");
    case eBinaryModeWrite :
        return fopen (Name, "wb");
    case eBinaryModeReadWrite :
        return fopen (Name, "rb+");
    case eBinaryModeStdInput:
        return stdin;
    case eBinaryModeStdOutput:
        return stdout;
    case eBinaryModeStdError:
        return stderr;
    default :
        return NULL;
    }
}

    
SdifInt4 SdiffBinClose(FILE *f)
{
    return (SdifInt4) fclose (f);
}

