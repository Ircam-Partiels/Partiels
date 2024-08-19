/******************************************************************************

  $Id: host_architecture.h,v 3.5 2015-10-14 16:00:38 diemo Exp $

  host_architecture.h provides the following conditionals:
  
  HOST_ARCH_ALPHA       - Current architecture is Alpha
  HOST_ARCH_SGI         - Current architecture is SGI
  HOST_ARCH_SUN         - Current architecture is Sun
  HOST_ARCH_NEXT        - Current architecture is NeXT
  HOST_ARCH_PPC         - Current architecture is PowerPC
  HOST_ARCH_68K         - Current architecture is 680x0
  HOST_ARCH_X86         - Current architecture is Intel PC (386..Pentium)
  HOST_ARCH_X86_64      - Current architecture is Intel 64bit
  HOST_ARCH_n86         - Current architecture is Intel (3|4)86, Pentium/K5 (586), 
                                                        or Pentium II (686)
  HOST_ARCH_ARMv7	- Current architecture is ARM as in iPhone/iPad

  HOST_OS_UNIX          - Generated code will run under Unix or Mac OSX
  HOST_OS_MAC           - Generated code will run under Mac OS 9
  HOST_OS_IOS           - Generated code will run under iOS
  HOST_OS_ANDROID       - Generated code will run under Android
  HOST_OS_WIN32         - Generated code will run under 32-bit Windows
                        
  HOST_ENDIAN_BIG       - Generated code uses big endian format for integers
  HOST_ENDIAN_LITTLE    - Generated code uses little endian format for integers
  
  The following is used to parse pathnames on different platforms which use
  different characters as dividers. On Unix, for instance, a slash is used whereas
  Mac OS uses colons.
  
  HOST_DIRECTORY_DIVIDER
  HOST_DIRECTORY_DIVIDER_STR
  
  The following conditionals are used to specify the calling
  convention of a function pointer:
  
  HOST_CALLBACK_API_C
  
  This conditional does the proper byte swapping to assue that a four character
  code (e.g. 'TEXT') is compiled down to the correct value on all compilers.
  
  HOST_FOUR_CHAR_CODE('abcd')   - Convert a four-char-code to the correct 32-bit value
  
  
  Alberto Ricci, 19990315

  $Log: not supported by cvs2svn $
  Revision 3.4  2013/07/16 11:13:10  diemo
  add detection of iOS and android (untested)

  Revision 3.3  2012/01/02 23:49:08  roebel
  Base selection of WIN32 specific implementation on definition of macros  WIN32 OR _WIN32. The latter being standard in
  Visual C++ it is most important to have it.

  Revision 3.2  2004/02/08 14:26:58  ellis

  now the textual scanner parses correctly character datas

  Revision 1.17  2002/05/30 08:31:35  roebel
  First attempt to extend host_architecture.h to MAC OSX.

  Revision 1.16  2000/12/06 13:42:34  lefevre
  Mix HostArchiteture and AutoConfigure mechanisms

 * Revision 1.15  2000/11/23  10:11:10  roebel
 * Reimplemented standard types for standard size integers
 * in Unix
 *
  Revision 1.14  2000/11/15 18:10:08  lefevre
  no message

 * Revision 1.13  2000/11/06  12:13:42  schwarz
 * Merged last parallel changes Linux/Mac.
 *
 * Revision 1.12  2000/05/11  13:36:34  lefevre
 * *** empty log message ***
 *
 * Revision 1.11  2000/02/29  17:54:34  schwarz
 * Added definitions for PowerPC (PPC) for Linux.
 *
 * Revision 1.10  2000/02/23  17:20:45  schwarz
 * Added symbols for intel processors for Linux.
 *
 * Revision 1.9  1999/03/18  15:49:54  ricci
 * *** empty log message ***
 *
 * Revision 1.8  1999/03/18  15:47:49  ricci
 * *** empty log message ***
 *
 * Revision 1.7  1999/03/15  16:35:15  ricci
 * Added HOST_DIRECTORY_DIVIDER_STR.
 *
 * Revision 1.6  1999/03/15  14:37:40  ricci
 * Added HOST_DIRECTORY_DIVIDER.
 *
 * Revision 1.5  1999/03/10  15:53:07  ricci
 * Added base integer types for Unix.
 *
 * Revision 1.4  1999/03/10  15:20:42  ricci
 * Corrected problem with endianness macros.
 *
 * Revision 1.3  1999/03/05  13:19:47  ricci
 * Added endianness macros.
 *
 * Revision 1.2  1999/03/01  14:48:44  ricci
 * Added CPU conditionals.
 *
******************************************************************************/


#ifndef __H_HOST_ARCHITECTURE__
#define __H_HOST_ARCHITECTURE__


/*===========================================================================*/
/* CPU */

#if   defined(__alpha) 

#     define HOST_ARCH_ALPHA                    1

#elif defined(sgi) || defined(__sgi) || defined(mips) || defined(__mips64)

#     define HOST_ARCH_SGI                              1

#elif defined(__sparc) || defined(sun)

#     define HOST_ARCH_SUN                              1

#elif defined(NeXT)

#     define HOST_ARCH_NEXT                             1

#elif defined(__powerpc__) || defined(__ppc__)

#     define HOST_ARCH_PPC                              1

#elif defined(i386) || defined(__i386)

#     define HOST_ARCH_386                              1
#     define HOST_ARCH_X86                              1

#elif defined(i486) || defined(__i486)

#     define HOST_ARCH_486                              1
#     define HOST_ARCH_X86                              1

#elif defined(i586) || defined(__i586)

#     define HOST_ARCH_586                              1
#     define HOST_ARCH_X86                              1

#elif defined(i686) || defined(__i686)

#     define HOST_ARCH_686                              1
#     define HOST_ARCH_X86                              1

#elif defined(x86_64) || defined(__x86_64)

#     define HOST_ARCH_X86                              1
#     define HOST_ARCH_X86_64                           1

#elif defined(__arm__)

#     define HOST_ARCH_ARMv7				1

#endif


/*===========================================================================*/
/* UNIX (includes Apple Mac OSX and iOS)*/

#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__MACH__)

#define HOST_OS_UNIX  1

/* Sub-case Linux
*/
#if defined(__linux__)
#define HOST_OS_LINUX  1
#endif

/* Type definitions for integers with known size
*/
typedef unsigned char       UInt8;
typedef signed char         SInt8;
typedef unsigned short      UInt16;
typedef signed short        SInt16;
typedef unsigned long       UInt32;
typedef signed long         SInt32;
typedef unsigned char       Boolean;
typedef unsigned long       FourCharCode;
typedef char*               Ptr;
typedef unsigned char       Str255[256];
typedef unsigned char *     StringPtr;

#if HOST_ARCH_SGI || HOST_ARCH_NEXT || HOST_ARCH_PPC
#   define HOST_ENDIAN_BIG  1
#   define WORDS_BIGENDIAN  1
#elif HOST_ARCH_ALPHA || HOST_ARCH_X86 || HOST_ARCH_ARMv7
#   define HOST_ENDIAN_LITTLE  1
#   undef  WORDS_BIGENDIAN
#endif

#if HOST_ARCH_ALPHA
#define SIZEOF_LONG                                     8
#else
#define SIZEOF_LONG                                     4
#endif

#define HAVE_ERRNO_H                                    1

#ifdef __STDC__
#define STDC_HEADERS                                    1
#endif

#define HOST_DIRECTORY_DIVIDER                         '/'
#define HOST_DIRECTORY_DIVIDER_STR                     "/"

#define HOST_CALLBACK_API_C(_type, _name)       _type (*_name)


/*===========================================================================*/
/* MACINTOSH OS 9 */

#elif defined(macintosh)

#define HOST_OS_MAC                                      1
#include <ConditionalMacros.h>

#if TARGET_CPU_PPC
#define HOST_ARCH_PPC                                    1
#elif TARGET_CPU_68K
#define HOST_ARCH_68K                                    1
#endif

#if TARGET_RT_BIG_ENDIAN
#define HOST_ENDIAN_BIG                                  1
#define WORDS_BIGENDIAN                                  1
#elif TARGET_RT_LITTLE_ENDIAN
#define HOST_ENDIAN_LITTLE                               1
#undef  WORDS_BIGENDIAN
#endif

#define SIZEOF_LONG                                      4
#undef  HAVE_ERRNO_H

#ifdef __STDC__
#define STDC_HEADERS                                     1
#endif

#define HOST_DIRECTORY_DIVIDER                          ':'
#define HOST_DIRECTORY_DIVIDER_STR                      ":"

#define HOST_CALLBACK_API_C                             CALLBACK_API_C



/*===========================================================================*/
/* Android */

#elif defined(__ANDROID__)

#define HOST_OS_ANDROID					1


/*===========================================================================*/
/* WINDOWS */

#elif   (defined(__INTEL__) && __INTEL__) || defined(_M_IX86) \
  || defined(_WIN32) ||   defined(WIN32) || defined(OS2) || defined(__DOS__)

#define HOST_OS_WIN32                                   1
#define HOST_ARCH_X86                                   1
#define HOST_ENDIAN_LITTLE                              1
#undef  WORDS_BIGENDIAN

#define SIZEOF_LONG                                             4
#undef  HAVE_ERRNO_H

#ifdef __STDC__
#define STDC_HEADERS                                    1
#endif

#define HOST_DIRECTORY_DIVIDER                  '/'
#define HOST_DIRECTORY_DIVIDER_STR              "/"

#define HOST_CALLBACK_API_C(_type, _name)       _type (__cdecl * _name)


/*===========================================================================*/
/* Unknown */

#else

#error host_architecture.h: unknown target OS

#endif


/*===========================================================================*/
/* HOST_ENDIAN_LITTLE and HOST_ENDIAN_BIG */

#if !defined(HOST_ENDIAN_BIG) && !defined(HOST_ENDIAN_LITTLE)
#   error host_architecture.h: unknown endianess of target processor
#endif



/*===========================================================================*/

#endif /* __H_HOST_ARCHITECTURE__ */
