/* $Id: SdifFWrite.h,v 3.13 2005-05-24 09:37:29 roebel Exp $
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
 * SdifFWrite.h
 *
 * F : SdifFileT* SdifF, Write : sdif file write (SdifF->Stream)
 *
 *
 * author: Dominique Virolle 1997
 *
EXAMPLE

L'exemple suivant essaye de montrer l'ordonnancement des appels
de fonction. Biensûr ce code devrait être plus modulaire. En effet,
il devrait y avoir une fonction par niveau structurel d'écriture:
une(plus) fonction(s) d'écriture de matrice, une(plus fonction(s)
d'écriture de frame...

<pre>
#include "sdif.h"

void main(void)
{
  SdifFileT   *SdifF;
  SdifUInt4    NbMatrix = 3;
  SdifUInt4    NumID = 0;
  SdifFloat8   Time = 0.0;
  SdifFloat4   TabValue[] = {1,2,3,4,5,6,7};
  SdifFloat4  *pTabValue;
  size_t       SizeFrameW;
  size_t       SizeMatrixW;

  pTabValue = TabValue; // pour permettre le cast par pointeur 

  SdifGenInit("SdifTypes.STYP");

  SdifF = SdifOpenFile("NewFile.sdif",      eWriteFile);


  // remplir les tables NameValues, MatrixTypesTable,
  // FrameTypesTable et StreamIDsTable.
   
  [ ..... ]

  // écriture de l'entête 
  SdifFWriteGeneralHeader (SdifF);
  // écriture des chunks ASCII 
  SdifFWriteAllASCIIChunks (SdifF)


  // ***FRAME HEADER****
  // Mise à jour le l'entête de frame à écrire 
  SdifSetCurrFrameHeader (SdifF, '1FOB', _SdifUnknownSize, NbMatrix, NumID, Time);
  // écriture de l'entête de frame 
  SizeFrameW = SdifFWriteFrameHeader (SdifF);


  // ***FIRST MATRIX**
  // Mise à jour le l'entête de matrice à écrire : 1 ligne, 1 colonne
  SdifSetCurrMatrixHeader (SdifF, '1FQ0', eFloat4, 1, 1);
  // écriture de l'entête de frame 
  SizeMatrixW = SdifFWriteMatrixHeader (SdifF);

  // Mise à jour de la ligne-buffer de SdifF
  // La largeur des données est conservée par le eFloat4 de l'entête de matrice
  SdifSetCurrOneRow (SdifF, (void*) pTabValue);
  // écriture de la ligne 
  SizeMatrixW += SdifFWriteOneRow (SdifF);
  // Si on a d'autres lignes à écrire alors
  // on répette SdifSetCurrOneRow et SizeMatrixW += SdifFWriteOneRow...

  // écriture du Padding en fin de matrice et ajout de la taille de la matrice écrite
  // à la taile du frame.
   
  SizeMatrixW += SdifFWritePadding(SdifF,
                       SdifFPaddingCalculate(SdifF->Stream, SizeMatrixW))  
  SizeFrameW += SizeMatrixW;


  // * MATRIX 2 & 3
  [. 2 matrices à écrire
   .
   .]

   
  // pas de padding en fin de frame car on est déjà aligné 

  // la taille écrite ne doit pas compter la signature et la taille===> -8 
  SizeFrameW -= 8;
  SdifFUpdateChunkSize(SdifF, SizeFrameW);

  SdifCloseFile(SdifF);

  SdifGenKill();
}
</pre>

LOG
 * $Log: not supported by cvs2svn $
 * Revision 3.12  2005/04/07 15:56:47  schwarz
 * removed some now empty local include files,
 * added include of <sdif.h> and "SdifGlobals.h"
 *
 * Revision 3.11  2003/08/06 15:11:45  schwarz
 * Finally removed obsolete functions (like SdifSkip...).
 *
 * Revision 3.10  2001/05/02 09:34:42  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.9  2000/11/21 14:51:49  schwarz
 * - sdif.h is now included by all sdif/Sdif*.c files.
 * - Removed all public typedefs, enums, structs, and defines from the
 *   individual sdif/Sdif*.h files, because they were duplicated in sdif.h.
 * - Todo: Do the same for the function prototypes, decide which types and
 *   prototypes really need to be exported.
 * - Removed SdifFileStruct.h.
 * - Preliminary new version of SdiffGetPos, SdiffSetPos.  They used the
 *   type fpos_t, which is no longer a long on RedHat 7 Linux.
 *
 * Revision 3.8  2000/10/27 20:03:31  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.7.2.2  2000/08/21  21:35:14  tisseran
 * *** empty log message ***
 *
 * Revision 3.7.2.1  2000/08/21  14:04:13  tisseran
 * *** empty log message ***
 *
 * Revision 3.7  2000/07/18  15:08:35  tisseran
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
 * Revision 3.6  2000/05/10  15:32:13  schwarz
 * Added functions to calculate the Size argument for SdifFSetCurrFrameHeader:
 * SdifSizeOfFrameHeader and SdifSizeOfMatrix
 *
 * Revision 3.5  2000/04/11  14:31:57  schwarz
 * SdifFWriteTextMatrix
 *
 * Revision 3.4  2000/03/01  11:19:46  schwarz
 * Added functions for matrix-wise writing:  SdifUpdateFrameHeader,
 * SdifFWriteMatrixData, SdifFWriteMatrix, SdifFWriteFrameAndOneMatrix
 *
 * Revision 3.3  1999/09/28  13:08:56  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.2  1999/08/25  18:32:35  schwarz
 * Added cocoon-able comments with sentinel "DOC:" (on a single line).
 *
 * Revision 3.1  1999/03/14  10:56:48  virolle
 * SdifStdErr add
 *
 * Revision 2.3  1999/01/23  15:55:47  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:31  virolle
 * General Lists, and special chunk preparation to become frames
 *
 * Revision 2.1  1998/12/21  18:27:14  schwarz
 * Inserted copyright message.
 *
 * Revision 2.0  1998/11/29  11:41:39  virolle
 * - New management of interpretation errors.
 * - Alignement of frames with CNMAT (execpt specials Chunk 1NVT, 1TYP, 1IDS).
 * _ Sdif Header File has a Sdif format version.
 * - Matrices order in frames is not important now. (only one occurence of
 *   a Matrix Type in a Frame Type declaration )
 * - Hard coded predefined types more dynamic management.
 * - Standart streams (stdin, stdout, stderr) set as binary for Windows32 to
 *   have exactly the same result on each plateforme.
 *
 * Revision 1.2  1998/11/10  15:31:44  schwarz
 * Removed all 'extern' keywords for prototypes, since this is redundant
 * (function prototypes are automatically linked extern), and it
 * prohibits cocoon from generating an entry in the HTML documentation
 * for this function.
 *
 */


#ifndef _SDIFFWRITE_H 
#define _SDIFFWRITE_H 1

#include "SdifGlobals.h"
#include <stdio.h>
#include "SdifMatrixType.h"
#include "SdifFrameType.h"
#include "SdifMatrix.h"
#include "SdifFrame.h"


/*
//FUNCTION GROUP:	Writing Header and Init-Frames
*/

/*DOC: 
  écrit sur le fichier 'SDIF' puis 4 bytes chunk size.  */
size_t  SdifFWriteGeneralHeader   (SdifFileT *SdifF);

size_t  SdifFWriteChunkHeader     (SdifFileT *SdifF, SdifSignature ChunkSignature, size_t ChunkSize);
size_t  SdifFWriteNameValueLCurrNVT (SdifFileT *SdifF);
size_t  SdifFWriteAllNameValueNVT   (SdifFileT *SdifF);

/*
no longer supported
size_t  SdifFWriteOneNameValue    (SdifFileT *SdifF, SdifNameValueT  *NameValue);
size_t  SdifFWriteOneMatrixType   (SdifFileT *SdifF, SdifMatrixTypeT *MatrixType);
size_t  SdifFWriteOneComponent    (SdifFileT *SdifF, SdifComponentT  *Component);
size_t  SdifFWriteOneFrameType    (SdifFileT *SdifF, SdifFrameTypeT  *FrameType);
size_t  SdifFWriteOneStreamID     (SdifFileT *SdifF, SdifStreamIDT   *StreamID);
*/


size_t  SdifFWriteAllMatrixType   (SdifFileT* SdifF);
size_t  SdifFWriteAllFrameType    (SdifFileT *SdifF);
size_t  SdifFWriteAllType         (SdifFileT *SdifF);

/*DOC:
  Remark:
         This function implements the new SDIF Specification (June 1999):
	 Name Value Table, Matrix and Frame Type declaration, Stream ID declaration are
	 defined in text matrix:
	 1NVT 1NVT
	 1TYP 1TYP
	 1IDS 1IDS
  Removed test for _SdifFormatVersion
  Now we write type in 1IDS frame which contains a 1IDS matrix
*/
size_t  SdifFWriteAllStreamID     (SdifFileT *SdifF);

/*DOC: 
  écrit tous les chunks ASCII. C'est à dire: les tables de names
  values, les types créés ou complétés, et les Stream ID. Il faut donc
  au préalable avoir rempli complétement les tables avant de la
  lancer. Cette fonction de peut donc pas être executer une 2nd fois
  durant une écriture.  */
size_t  SdifFWriteAllASCIIChunks  (SdifFileT *SdifF);




/*
//FUNCTION GROUP:	Writing Matrices
*/

/*DOC: 
  Après avoir donner une valeur à chaque champ de SdifF->CurrMtrxH
  gràce à la fonction SdifFSetCurrMatrixHeader, SdifFWriteMatrixHeader
  écrit toute l'entête de la matrice.  Cette fonction réalise aussi
  une mise à jour de SdifF->CurrOneRow, tant au niveau de l'allocation
  mémoire que du type de données.  */
size_t  SdifFWriteMatrixHeader    (SdifFileT *SdifF);

/*DOC: 
  Après avoir donner les valeurs à chaque case de SdifF->CurrOneRow à
  l'aide de SdifFSetCurrOneRow ou de SdifFSetCurrOneRowCol (suivant
  que l'on possède déjà un tableau flottant ou respectivement une
  méthode pour retrouver une valeur de colonne), SdifFWriteOneRow
  écrit 1 ligne de matrice suivant les paramètres de SdifF->CurrMtrxH.  */
size_t  SdifFWriteOneRow          (SdifFileT *SdifF);

/*DOC: 
  Write whole matrix data, (after having set the matrix header with 
  SdifFSetCurrMatrixHeader (file, matrixsig, datatype, nrow, ncol).
  Data points to nbrow * nbcol * SdifSizeofDataType (datatype) bytes in 
  row-major order.  Padding still has to be written.  */
size_t SdifFWriteMatrixData (SdifFileT *SdifF, void *Data);

/*DOC:
  Write whole matrix: header, data, and padding.
  Data points to NbRow * NbCol * SdifSizeofDataType (DataType) bytes in
  row-major order. */
size_t SdifFWriteMatrix (SdifFileT     *SdifF,
			 SdifSignature  Signature,
			 SdifDataTypeET DataType,
			 SdifUInt4      NbRow,
			 SdifUInt4      NbCol,
			 void	       *Data);

/*DOC:
  Write a matrix with datatype text (header, data, and padding).
  Data points to Length bytes(!) of UTF-8 encoded text.  Length
  includes the terminating '\0' character!!!  That is, to write a
  C-String, use SdifFWriteTextMatrix (f, sig, strlen (str) + 1, str);
  to include it. */
size_t SdifFWriteTextMatrix (SdifFileT     *SdifF,
			     SdifSignature  Signature,
			     SdifUInt4      Length,
			     char	   *Data);

/*DOC: 
  TBI: Convert ASCII C-String to UTF-8 encoded string, returning
  length (including terminating null character). */
size_t SdifAsciiToUTF8 (char *ascii_in, char *utf8_out);

/*DOC: 
  Cette fonction permet en fin d'écriture de matrice d'ajouter le
  Padding nécessaire. Il faut cependant avoir la taille de ce
  Padding. On utilise SdifFPaddingCalculate(SdifF->Stream,
  SizeSinceAlignement) où SizeSinceAllignement est un
  <code>size_t</code> désignant le nombre de bytes qui sépare la
  position actuelle d'écriture avec une position connue où le fichier
  est aligné sur 64 bits (en général, c'est la taille de la matrice en
  cours d'écriture: NbRow*NbCol*DatWitdh).  */
size_t  SdifFWritePadding         (SdifFileT *SdifF, size_t Padding);




/*
//FUNCTION GROUP:	Writing Frames
*/

/*DOC: 
  Après avoir donner une valueur à chaque champ de SdifF->CurrFramH
  gràce à la fonction SdifFSetCurrFrameHeader, SdifFWriteFrameHeader
  écrit toute l'entête de frame.  Lorsque la taille est inconnue au
  moment de l'écriture, donner la valeur _SdifUnknownSize. Ensuite,
  compter le nombre de bytes écrit dans le frame et réaliser un
  SdifUpdateChunkSize avec la taille calculée.  */
size_t  SdifFWriteFrameHeader     (SdifFileT *SdifF);

/*DOC: 
  Execute un retour fichier de ChunkSize bytes et l'écrit, donc on
  écrase la taille du chunk ou du frame.  Dans le cas où le fichier
  est stderr ou stdout, l'action n'est pas réalisée.  */
void    SdifUpdateChunkSize       (SdifFileT *SdifF, size_t ChunkSize);

/*DOC: 
  Rewrite given frame size and number of matrices in frame header.
  Return -1 on error or if file is not seekable (stdout or stderr). */
int     SdifUpdateFrameHeader	  (SdifFileT *SdifF, size_t ChunkSize, 
				   SdifInt4 NumMatrix);

/*DOC:
  Write a whole frame containing one matrix: 
  frame header, matrix header, matrix data, and padding.
  Data points to NbRow * NbCol * SdifSizeofDataType (DataType) bytes in
  row-major order. 

  This function has the big advantage that the frame size is known in
  advance, so there's no need to rewind and update after the matrix
  has been written.  */
size_t  SdifFWriteFrameAndOneMatrix (SdifFileT	    *SdifF,
				     SdifSignature  FrameSignature,
				     SdifUInt4      NumID,
				     SdifFloat8     Time,
				     SdifSignature  MatrixSignature,
				     SdifDataTypeET DataType,
				     SdifUInt4      NbRow,
				     SdifUInt4      NbCol,
				     void	    *Data);


/*DOC:
  Return (constant) size of frame header after signature and size field. 
  Use this to calculate the Size argument for SdifFSetCurrFrameHeader. */
size_t SdifSizeOfFrameHeader (void);

/*DOC:
  Return size of matrix (header, data, padding).
  Use this to calculate the Size argument for SdifFSetCurrFrameHeader. */
size_t SdifSizeOfMatrix (SdifDataTypeET DataType,
			 SdifUInt4      NbRow,
			 SdifUInt4      NbCol);

/*DOC:
  Write a text matrix using a string.
  Return number of bytes written.
*/
size_t SdifFWriteTextFrame(SdifFileT     *SdifF,
			   SdifSignature FrameSignature,
			   SdifUInt4     NumID,
			   SdifFloat8    Time,
			   SdifSignature MatrixSignature,
			   char          *str,
			   size_t        length);

/*DOC:
  Write a text matrix using a SdifString.
  Return number of bytes written.
*/
size_t SdifFWriteTextFrameSdifString(SdifFileT     *SdifF,
				     SdifSignature FrameSignature,
				     SdifUInt4     NumID,
				     SdifFloat8    Time,
				     SdifSignature MatrixSignature,
				     SdifStringT   *SdifString);

#endif /* _SdifFWrite_ */

