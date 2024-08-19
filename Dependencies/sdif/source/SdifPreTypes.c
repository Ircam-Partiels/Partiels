/* $Id: SdifPreTypes.c,v 3.9 2005-04-07 15:56:47 schwarz Exp $
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
 * Compiled Predefined Types. If the predefined types file isn't find or understood,
 * then this types are used. But in this case, some types can miss.
 *
 * author: Dominique Virolle 1997
 *
 *
 *
 * $Log: not supported by cvs2svn $
 * Revision 3.8  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.7  2001/05/02 09:34:46  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.6  2000/11/21 16:34:50  roebel
 * New SdifSignatureConst builds integer signature according to
 * endianess of machine. Multicharacter constants are no longer
 * supported by the library. Cleaned up sdif.h/SdifGlobals.h a bit.
 * Test for Multicharacter conversion is removed from configure.in.
 *
 * Revision 3.5  2000/11/21 14:51:50  schwarz
 * - sdif.h is now included by all sdif/Sdif*.c files.
 * - Removed all public typedefs, enums, structs, and defines from the
 *   individual sdif/Sdif*.h files, because they were duplicated in sdif.h.
 * - Todo: Do the same for the function prototypes, decide which types and
 *   prototypes really need to be exported.
 * - Removed SdifFileStruct.h.
 * - Preliminary new version of SdiffGetPos, SdiffSetPos.  They used the
 *   type fpos_t, which is no longer a long on RedHat 7 Linux.
 *
 * Revision 3.4  2000/11/15 14:53:33  lefevre
 * no message
 *
 * Revision 3.3  2000/10/27  20:03:40  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.2.2.1  2000/08/21  21:35:40  tisseran
 * *** empty log message ***
 *
 * Revision 3.2  1999/09/28  13:09:08  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.1  1999/03/14  10:57:14  virolle
 * SdifStdErr add
 *
 * Revision 2.3  1999/01/23  15:55:58  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:43  virolle
 * General Lists, and special chunk preparation to become frames
 *
 *
 *
 */


#include "sdif_portability.h"

#include <sdif.h>
#include "SdifGlobals.h"



/*****************************************
******************************************
*          PREDEFINED MATRIX             *
******************************************
******************************************/


/* 1FQ0 matrix
 *  Mtrx  1FQ0    {Frequency, Mode, Hit}
 */
SdifMatrixTypeT*
CreateM_1FQ0(void)
{
  SdifMatrixTypeT*  M_1FQ0;


  M_1FQ0 = SdifCreateMatrixType(SdifSignatureConst('1','F','Q','0'), NULL);
  SdifMatrixTypeInsertTailColumnDef(M_1FQ0, M_1FQ0_Frequency);
  SdifMatrixTypeInsertTailColumnDef(M_1FQ0, M_1FQ0_Mode);
  SdifMatrixTypeInsertTailColumnDef(M_1FQ0, M_1FQ0_Hit);
  M_1FQ0->ModifMode = eNoModif;
  return M_1FQ0;
}






/* 1FOF matrix
 *  Mtrx  1FOF  {Frequency, Amplitude, BandWidth, Tex, DebAtt, Atten, Phase}
 */
SdifMatrixTypeT*
CreateM_1FOF(void)
{
  SdifMatrixTypeT*  M_1FOF;


  M_1FOF = SdifCreateMatrixType(SdifSignatureConst('1','F','O','F'), NULL);
  SdifMatrixTypeInsertTailColumnDef(M_1FOF, M_1FOF_Frequency);
  SdifMatrixTypeInsertTailColumnDef(M_1FOF, M_1FOF_Amplitude);
  SdifMatrixTypeInsertTailColumnDef(M_1FOF, M_1FOF_BandWidth);
  SdifMatrixTypeInsertTailColumnDef(M_1FOF, M_1FOF_Tex);
  SdifMatrixTypeInsertTailColumnDef(M_1FOF, M_1FOF_DebAtt);
  SdifMatrixTypeInsertTailColumnDef(M_1FOF, M_1FOF_Atten);
  SdifMatrixTypeInsertTailColumnDef(M_1FOF, M_1FOF_Phase);
  M_1FOF->ModifMode = eNoModif;
  return M_1FOF;
}





/* 1CHA matrix
 *  Mtrx  1CHA	{Channel1, Channel2, Channel3, Channel4}
 */
SdifMatrixTypeT*
CreateM_1CHA(void)
{
  SdifMatrixTypeT*  M_1CHA;


  M_1CHA = SdifCreateMatrixType(SdifSignatureConst('1','C','H','A'), NULL);
  SdifMatrixTypeInsertTailColumnDef(M_1CHA, M_1CHA_Channel1);
  SdifMatrixTypeInsertTailColumnDef(M_1CHA, M_1CHA_Channel2);
  SdifMatrixTypeInsertTailColumnDef(M_1CHA, M_1CHA_Channel3);
  SdifMatrixTypeInsertTailColumnDef(M_1CHA, M_1CHA_Channel4);
  M_1CHA->ModifMode = eNoModif;
  return M_1CHA;
}



/* 1RES matrix
 *  Mtrx  1RES	{Frequency, Amplitude, BandWidth, Saliance, Correction}
 */
SdifMatrixTypeT*
CreateM_1RES(void)
{
  SdifMatrixTypeT*  M_1RES;


  M_1RES = SdifCreateMatrixType(SdifSignatureConst('1','R','E','S'), NULL);
  SdifMatrixTypeInsertTailColumnDef(M_1RES, M_1RES_Frequency);
  SdifMatrixTypeInsertTailColumnDef(M_1RES, M_1RES_Amplitude);
  SdifMatrixTypeInsertTailColumnDef(M_1RES, M_1RES_BandWidth);
  SdifMatrixTypeInsertTailColumnDef(M_1RES, M_1RES_Saliance);
  SdifMatrixTypeInsertTailColumnDef(M_1RES, M_1RES_Correction);
  M_1RES->ModifMode = eNoModif;
  return M_1RES;
}



/* 1DIS matrix
 *  Mtrx  1DIS	{Distribution, Amplitude}
 */
SdifMatrixTypeT*
CreateM_1DIS(void)
{
  SdifMatrixTypeT*  M_1DIS;


  M_1DIS = SdifCreateMatrixType(SdifSignatureConst('1','D','I','S'), NULL);
  SdifMatrixTypeInsertTailColumnDef(M_1DIS, M_1DIS_Distribution);
  SdifMatrixTypeInsertTailColumnDef(M_1DIS, M_1DIS_Amplitude);
  M_1DIS->ModifMode = eNoModif;
  return M_1DIS;
}




/*****************************************
******************************************
*          PREDEFINED FRAMES             *
******************************************
******************************************/


/* 1FOB frame
 *  Fram	1FOB
 *	{
 *	  1FQ0  PitchModeHit;
 *	  1FOF	Formants;
 *	  1CHA  FormantsChannels;
 *	}
 */

SdifFrameTypeT*
CreateF_1FOB(void)
{
  SdifFrameTypeT*  F_1FOB;

  F_1FOB = SdifCreateFrameType(SdifSignatureConst('1','F','O','B'), NULL);
  SdifFrameTypePutComponent(F_1FOB, SdifSignatureConst('1','F','Q','0'), "PitchModeHit");
  SdifFrameTypePutComponent(F_1FOB, SdifSignatureConst('1','F','O','F'), "Formants");
  SdifFrameTypePutComponent(F_1FOB, SdifSignatureConst('1','C','H','A'), "FormantsChannels");
  F_1FOB->ModifMode = eNoModif;
  return F_1FOB;
}


/* 1REB frame
 *  Fram	1REB
 *	{
 *	  1RES  Filters;
 *	  1CHA  FiltersChannels;
 *	}
 */

SdifFrameTypeT*
CreateF_1REB(void)
{
  SdifFrameTypeT*   F_1REB;

  F_1REB = SdifCreateFrameType(SdifSignatureConst('1','R','E','B'), NULL);
  SdifFrameTypePutComponent(F_1REB, SdifSignatureConst('1','R','E','S'), "Filters");
  SdifFrameTypePutComponent(F_1REB, SdifSignatureConst('1','C','H','A'), "FiltersChannels");
  F_1REB->ModifMode = eNoModif;
  return F_1REB;
}



/* 1NOI frame
 *  Fram  1NOI
 *	{
 *	  1DIS  NoiseInfo;
 *	}
 */

SdifFrameTypeT*
CreateF_1NOI(void)
{
  SdifFrameTypeT*   F_1NOI;

  F_1NOI = SdifCreateFrameType(SdifSignatureConst('1','N','O','I'), NULL);
  SdifFrameTypePutComponent(F_1NOI, SdifSignatureConst('1','D','I','S'), "NoiseInfo");
  F_1NOI->ModifMode = eNoModif;
  return F_1NOI;
}


void
SdifCreatePredefinedTypes(SdifHashTableT *MatrixTypesHT, SdifHashTableT *FrameTypesHT)
{
  SdifPutMatrixType(MatrixTypesHT,   CreateM_1FQ0());
  SdifPutMatrixType(MatrixTypesHT,   CreateM_1FOF());
  SdifPutMatrixType(MatrixTypesHT,   CreateM_1CHA());
  SdifPutMatrixType(MatrixTypesHT,   CreateM_1RES());
  SdifPutMatrixType(MatrixTypesHT,   CreateM_1DIS());

  SdifPutFrameType(FrameTypesHT,   CreateF_1FOB() );
  SdifPutFrameType(FrameTypesHT,   CreateF_1REB() );
  SdifPutFrameType(FrameTypesHT,   CreateF_1NOI() );
}

