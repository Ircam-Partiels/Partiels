/* $Id: SdifTest.h,v 3.9 2007-12-10 10:45:54 roebel Exp $
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
LIBRARY
 * SdifTest.h
 *
 * Tests with message on errors apply on a SdifFileT*
 * exit(1) is exceptional
 *
 *
 * author: Dominique Virolle 1997
 *
LOG
 * $Log: not supported by cvs2svn $
 * Revision 3.8  2005/05/23 17:52:53  schwarz
 * Unified error handling:
 * - SdifErrorEnum (global errors) integrated into SdifErrorTagET (file errors)
 * - no more SdifError.[ch], everything done by SdifErrMess.[ch]
 *
 * Revision 3.7  2001/05/02 09:34:48  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.6  2000/11/21 14:51:51  schwarz
 * - sdif.h is now included by all sdif/Sdif*.c files.
 * - Removed all public typedefs, enums, structs, and defines from the
 *   individual sdif/Sdif*.h files, because they were duplicated in sdif.h.
 * - Todo: Do the same for the function prototypes, decide which types and
 *   prototypes really need to be exported.
 * - Removed SdifFileStruct.h.
 * - Preliminary new version of SdiffGetPos, SdiffSetPos.  They used the
 *   type fpos_t, which is no longer a long on RedHat 7 Linux.
 *
 * Revision 3.5  2000/10/27 20:03:45  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.4  2000/08/22  13:38:42  schwarz
 * SdifFTestMatrixWithFrameHeader warns about additional undeclared
 * matrix in frame but do not signal an error, because we allow any
 * additional matrices in frames.
 * Replaced short by int, because that's faster and less error-prone.
 *
 * Revision 3.3.2.1  2000/08/21  21:35:54  tisseran
 * *** empty log message ***
 *
 * Revision 3.3  1999/09/28  13:09:15  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.2  1999/08/25  18:32:37  schwarz
 * Added cocoon-able comments with sentinel "DOC:" (on a single line).
 *
 * Revision 3.1  1999/03/14  10:57:24  virolle
 * SdifStdErr add
 *
 * Revision 2.1  1998/12/21  18:27:42  schwarz
 * Inserted copyright message.
 *
 * Revision 2.0  1998/11/29  11:42:09  virolle
 * - New management of interpretation errors.
 * - Alignement of frames with CNMAT (execpt specials Chunk 1NVT, 1TYP, 1IDS).
 * _ Sdif Header File has a Sdif format version.
 * - Matrices order in frames is not important now. (only one occurence of
 *   a Matrix Type in a Frame Type declaration )
 * - Hard coded predefined types more dynamic management.
 * - Standart streams (stdin, stdout, stderr) set as binary for Windows32 to
 *   have exactly the same result on each plateforme.
 *
 * Revision 1.4  1998/11/10  15:31:56  schwarz
 * Removed all 'extern' keywords for prototypes, since this is redundant
 * (function prototypes are automatically linked extern), and it
 * prohibits cocoon from generating an entry in the HTML documentation
 * for this function.
 *
 */

#ifndef _SdifTest_
#define _SdifTest_

#include "SdifGlobals.h"
#include "SdifMatrixType.h"
#include "SdifFrameType.h"


#define _SdifFileMess(sdiff, error, mess) 

/*DOC: 
  Cette fonction vérifie si le type de matrice est répertorié
  dans SdifF.<br> S'il ne l'est pas, alors elle vérifie si c'est un
  type prédéfinis. S'il est prédéfini, elle crée le lien de SdifF vers
  le type prédéfini. Sinon, elle envoie un message sur l'erreur
  standart.  */
SdifMatrixTypeT* SdifTestMatrixType (SdifFileT *SdifF, SdifSignature Signature);
SdifFrameTypeT*  SdifTestFrameType  (SdifFileT *SdifF, SdifSignature Signature);



int SdifFTestMatrixWithFrameHeader (SdifFileT* SdifF);
int SdifFTestDataType              (SdifFileT* SdifF);
int SdifFTestNbColumns             (SdifFileT* SdifF);
int SdifFTestNotEmptyMatrix        (SdifFileT* SdifF);
int SdifFTestMatrixHeader          (SdifFileT* SdifF);

int SdifTestMatrixTypeModifMode  (SdifFileT *SdifF, SdifMatrixTypeT *MatrixType);
int SdifTestFrameTypeModifMode   (SdifFileT *SdifF, SdifFrameTypeT *FrameType);

#endif /* _ SdifTest_ */
