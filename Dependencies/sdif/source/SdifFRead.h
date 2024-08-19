/* $Id: SdifFRead.h,v 3.14 2005-04-07 15:56:47 schwarz Exp $
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

LIBRARY
 * SdifFRead.h
 *
 * F : SdifFileT* SdifF, Read : binary read (SdifF->Stream)
 *
 * author: Dominique Virolle 1997

LOG
 * $Log: not supported by cvs2svn $
 * Revision 3.13  2003/08/06 15:29:18  schwarz
 * oops;
 *
 * Revision 3.12  2003/08/06 15:16:41  schwarz
 * Why not declare and document new functions?
 *
 * Revision 3.11  2003/08/06 15:13:14  schwarz
 * New functions SdifFSkip, SdifFSkipOneRow.
 * Finally removed obsolete functions (like SdifSkip...).
 *
 * Revision 3.10  2001/05/02 09:34:42  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.9  2000/11/21 14:51:48  schwarz
 * - sdif.h is now included by all sdif/Sdif*.c files.
 * - Removed all public typedefs, enums, structs, and defines from the
 *   individual sdif/Sdif*.h files, because they were duplicated in sdif.h.
 * - Todo: Do the same for the function prototypes, decide which types and
 *   prototypes really need to be exported.
 * - Removed SdifFileStruct.h.
 * - Preliminary new version of SdiffGetPos, SdiffSetPos.  They used the
 *   type fpos_t, which is no longer a long on RedHat 7 Linux.
 *
 * Revision 3.8  2000/10/27 20:03:29  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.7  2000/08/22  13:38:23  schwarz
 * SdifSkip... renamed to SdifFSkip... to follow nomenclature.
 *
 * Revision 3.6.2.3  2000/08/21  21:35:10  tisseran
 * *** empty log message ***
 *
 * Revision 3.6.2.2  2000/08/21  18:34:10  tisseran
 * Add SdifSkipASCIIUntilfromSdifString function (same as SdifSkipASCIIUntil).
 * Add SdifFSkip for SdifSkip for (functions SdifSkip doesn't respect function nomemclature => obsolete).
 *
 * Revision 3.6.2.1  2000/08/21  14:04:11  tisseran
 * *** empty log message ***
 *
 * Revision 3.6  2000/07/18  15:08:33  tisseran
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
 * Revision 3.5  2000/05/15  16:23:07  schwarz
 * Avoided avoidable warnings.
 *
 * Revision 3.4  2000/03/01  11:19:58  schwarz
 * Assert Padding, added SdifFReadAndIgnore.
 * SdiffSetPos checks for pipe and then uses SdifFReadAndIgnore to seek forward.
 *
 * Revision 3.3  1999/09/28  13:08:54  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.2  1999/08/25  18:32:34  schwarz
 * Added cocoon-able comments with sentinel "DOC:" (on a single line).
 *
 * Revision 3.1  1999/03/14  10:56:44  virolle
 * SdifStdErr add
 *
 * Revision 2.3  1999/01/23  15:55:43  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:28  virolle
 * General Lists, and special chunk preparation to become frames
 *
 * Revision 2.1  1998/12/21  18:27:11  schwarz
 * Inserted copyright message.
 *
 * Revision 2.0  1998/11/29  11:41:34  virolle
 * - New management of interpretation errors.
 * - Alignement of frames with CNMAT (execpt specials Chunk 1NVT, 1TYP, 1IDS).
 * _ Sdif Header File has a Sdif format version.
 * - Matrices order in frames is not important now. (only one occurence of
 *   a Matrix Type in a Frame Type declaration )
 * - Hard coded predefined types more dynamic management.
 * - Standart streams (stdin, stdout, stderr) set as binary for Windows32 to
 *   have exactly the same result on each plateforme.
 *
 * Revision 1.2  1998/11/10  15:31:42  schwarz
 * Removed all 'extern' keywords for prototypes, since this is redundant
 * (function prototypes are automatically linked extern), and it
 * prohibits cocoon from generating an entry in the HTML documentation
 * for this function.
 */


#ifndef _SDIFFREAD_H 
#define _SDIFFREAD_H 1

#include <stdio.h>
#include <sdif.h>
#include "SdifGlobals.h"
#include "SdifMatrix.h"
#include "SdifFrame.h"
#include "SdifFGet.h"


/*DOC: 
  Lit l'entête du fichier, c'est à dire 'SDIF' puis 4 bytes.  affiche
  un message en cas de non reconnaissance du format.  */
size_t SdifFReadGeneralHeader    (SdifFileT *SdifF);

size_t SdifFReadChunkSize        (SdifFileT *SdifF);
size_t SdifFReadNameValueLCurrNVT(SdifFileT *SdifF);
size_t SdifFReadAllType          (SdifFileT *SdifF);
size_t SdifFReadAllStreamID      (SdifFileT *SdifF);

/*DOC: 
  Cette fonction permet de lire tous les Chunk ASCII qui se
  trouveraient en début de fichier juste après l'entête générale. Elle
  s'arrête lorsqu'elle ne reconnaît pas la signature de chunk comme un
  ASCII Chunk. Cette signature est donc normalement celle d'un
  frame. Elle est stockée dans SdifF->CurrSignature. <strong>Il n'est
  donc pas nécessaire de la relire</strong>.  */
size_t SdifFReadAllASCIIChunks   (SdifFileT *SdifF);

/*DOC: 
  Cette fonction lit une entête de matrice <strong>signature
  incluse</strong>.  Elle vérifie le type de matrice, le champ
  DataType. Toute les données se trouvent stockées dans
  SdifF->CurrMtrxH. La plupart de ses champs sont directement
  accessible par les fonctions indépendantes du mode d'ouverture du
  fichier.  <strong>Elle effectue une mise à jour de l'allocation
  mémoire de SdifF->CurrOneRow en fonction des paramètres de l'entête
  de matrice.</strong> Ainsi, on est normalement près pour lire chaque
  ligne de la matrice courrante.  */
size_t SdifFReadMatrixHeader     (SdifFileT *SdifF);

/*DOC: 
  Cette fonction permet de lire 1 ligne de matrice. Les données lues
  sont stockées dans SdifF->CurrOneRow (jusqu'à une prochaine lecture
  d'entête de matrice qui réinitialise ses paramètres).  */
size_t SdifFReadOneRow           (SdifFileT *SdifF);

/*DOC: 
  Cette fonction lit l'entête d'un frame à partir de la taille et
  jusqu'au temps. Donc <strong>elle ne lit pas la signature</strong>
  mais donne à SdifF->CurrFramH->Signature la valeur de
  SdifF->CurrSignature.  La lecture doit se faire avant, avec
  SdifFGetSignature.  */
size_t SdifFReadFrameHeader      (SdifFileT *SdifF);


/* sub-group: skipping */

/*DOC:
  skip one matrix row, when reading row by row with SdifFReadOneRow */
size_t SdifFSkipOneRow           (SdifFileT *SdifF);


/*DOC: 
  Cette fonction permet de passer une matrice toute entière entête
  incluse. Elle est utile lorsque qu'un frame contient plus de
  matrices que le programme lecteur n'en connaît. Il peut ainsi les
  passer pour retomber sur un autre frame.  */
size_t SdifFSkipMatrix            (SdifFileT *SdifF);


/*DOC: 
  Cette fonction permet de passer une matrice mais après la lecture de
  l'entête. On s'en sert lorsque le type de matrice est mauvais,
  inconnu, non interprétable par le programme lecteur.

  Note:  The matrix padding is skipped also. */
size_t SdifFSkipMatrixData        (SdifFileT *SdifF);


/*DOC: 
  Cette fonction à le même sens que SdifSkipMatrixData mais pour les
  frames. Il faut donc pour l'utiliser avoir au préalable lu la
  signature et l'entête.  */
size_t SdifFSkipFrameData         (SdifFileT *SdifF);


/*DOC: 
  skip given number of bytes, either by seeking or by reading and ignoring */
size_t SdifFSkip                  (SdifFileT *SdifF, size_t nbytes);

/*DOC:
  Read and throw away <i>num</i> bytes from the file. */
size_t SdifFReadAndIgnore (SdifFileT *SdifF, size_t bytes);



/* sub-group: padding */

/*DOC: 
  Cette fonction permet de lire le Padding en fin de matrice.
  l'utilisation classique de cette fonctin est:<br> 
  <code> SizeR =  SdifFReadPadding(SdifF, SdifFPaddingCalculate(SdifF->Stream, SizeR));</code><br> 
  où SizeR est la taille en bytes lue depuis le
  début de la matrice, c'est à dire NbRow*NbCol*DataWith. En réalité,
  pour que SdifFPaddingCalculate fonctionne, il est seulement
  nécessaire que SizeR soit le nombre de bytes qui s'épare la position
  actuelle dans le fichier et un byte, repère d'allignement sur 64
  bits.  */
size_t SdifFReadPadding          (SdifFileT *SdifF, size_t Padding);
size_t SdifFReadUndeterminatedPadding (SdifFileT *SdifF);

size_t SdifFReadOneMatrixType    (SdifFileT *SdifF);
size_t SdifFReadOneFrameType     (SdifFileT *SdifF);

/*DOC:
  Function to read text matrix.
  Read header.
  Read data.
  Read padding.
*/
size_t SdifFReadTextMatrix(SdifFileT *SdifF, SdifStringT *SdifString);

/*DOC:
  Function to read text matrix data.
  Make reallocation.
  Read data.
  Read padding.
*/
size_t SdifFReadTextMatrixData(SdifFileT *SdifF, SdifStringT *SdifString);

#endif /* _SdifFRead_ */
