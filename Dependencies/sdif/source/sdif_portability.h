/******************************************************************************
 *
 * $Id: sdif_portability.h,v 3.6 2013-07-16 10:26:13 diemo Exp $
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
 * Desctiption of header
 *
 * sdif_portability ensures proper  compilation on systems that do not
 * provide the autoconf mechanism.
 *
 * LOG
 * $Log: not supported by cvs2svn $
 * Revision 3.5  2012/01/02 23:49:08  roebel
 * Base selection of WIN32 specific implementation on definition of macros  WIN32 OR _WIN32. The latter being standard in
 * Visual C++ it is most important to have it.
 *
 * Revision 3.4  2006/12/14 17:21:57  roebel
 * Support universal binaries configuration for mac os x.
 * Use endian.h for endianness determination.
 *
 * Revision 3.3  2006/01/16 18:33:29  muller
 * updated visual studio 7.1 projects in order to build the latest sdif version. (only tested with FTM)
 *
 * it only builds a static library for now.
 * for a DLL we either need a module definition file with the list of exported symbols
 * or something like:
 *
 * #if defined( _WIN32) || defined(WIN32)
 * #if defined(SDIFDLL_EXPORTS)
 * #define SDIF_API __declspec(dllexport)
 * #else
 * #define SDIF_API __declspec(dllimport)
 * #endif
 * #else
 * #define SDIF_API extern
 * #endif
 *
 * and SDIF_API in front of all exported symbols
 *
 * Revision 3.2  2004/07/22 14:47:56  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.1  2003/11/07 22:00:48  roebel
 * added preincluded.h replacement to CVS
 *
 *
 *****************************************************************************/

#ifndef __H_SDIF_PORTABILITY__
#define __H_SDIF_PORTABILITY__

#include "sdif_version.h"

#if defined( _WIN32) || defined(WIN32)
#   include <stdio.h>
#   define snprintf _snprintf
#endif

#ifndef HAVE_CONFIG_H
/* Host Architecture Mechanism
   ---------------------------
*/
#include <host_architecture.h>

#if __MACH__
#define HAVE_PTHREAD 1
#endif

/* Macro HOST_FOUR_CHAR_CODE()
   ---------------------------
*/
#if defined(WORDS_BIGENDIAN)
#   define HOST_FOUR_CHAR_CODE(x)		(x)
#   define FOUR_CHAR_CODE(x)		(x)
#else
#   define HOST_FOUR_CHAR_CODE(x)	(((UInt32) ((x) & 0x000000FF)) << 24) \
				      | (((UInt32) ((x) & 0x0000FF00)) << 8)  \
				      | (((UInt32) ((x) & 0x00FF0000)) >> 8)  \
				      | (((UInt32) ((x) & 0xFF000000)) >> 24)
#   define FOUR_CHAR_CODE(x)		(x)
#endif /* defined(WORDS_BIGENDIAN) */



#else /* HAVE_CONFIG_H */

/* Auto Configure Mechanism
   ------------------------
*/
/* Yet defined in host_architecture.h, then undef those common macros...
*/
#undef STDC_HEADERS
#undef WORDS_BIGENDIAN
#undef SIZEOF_LONG
#undef HAVE_ERRNO_H

/* ... and redefined them in "config.h"
*/
#include "config.h"

/* 
 * for mac os x we use machine/endian.h to be able to create universal binaries in
 * cross compilation mode.
 */
#ifdef HAVE_MACHINE_ENDIAN_H
#include <machine/endian.h>
#if  BYTE_ORDER == BIG_ENDIAN
#define WORDS_BIGENDIAN 1
#endif
#endif

/* HOST_ENDIAN_LITTLE/BIG defined from WORDS_BIGENDIAN
*/
#undef  HOST_ENDIAN_BIG
#undef  HOST_ENDIAN_LITTLE

#if defined(WORDS_BIGENDIAN)
#define HOST_ENDIAN_BIG		1
#else
#define HOST_ENDIAN_LITTLE	1
#endif /* defined(WORDS_BIGENDIAN) */

#define HOST_DIRECTORY_DIVIDER '/'
#define HOST_DIRECTORY_DIVIDER_STR "/"

#endif /* HAVE_CONFIG_H */


/* project_preinclude.h must be present in YOUR project
   ----------------------------------------------------
*/

#endif /* __H_SDIF_PORTABILITY__ */
