/* $Id: SdifMatrix.c,v 3.19 2009-10-29 23:30:20 roebel Exp $
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
 * Matrix Header, Data, Rows structure management.
 *
 * Memory allocation of SdifOneRowT* depend on size of one granule.
 * if nb rows or data type (float4 or float8),
 * then SdifReInitOneRow makes a realloc. Then think to use it before write.
 *
 * author: Dominique Virolle 1997
 * $Log: not supported by cvs2svn $
 * Revision 3.18  2008/04/10 14:12:25  roebel
 * Don't create new vrariables on the fly
 *
 * Revision 3.17  2007/11/27 12:12:22  roebel
 * Fixed warning due to missing return value.
 *
 * Revision 3.16  2007/11/26 18:51:21  roebel
 * Changed data types to have less casts and
 * less compilation warnings with MSVC.
 * renamed some functions that are deprecated in MSVC.
 *
 * Revision 3.15  2005/10/21 14:32:29  schwarz
 * protect all static buffers from overflow by using snprintf instead of sprintf
 * move big errorMess buffers into error branch to avoid too large stack allocation
 *
 * Revision 3.14  2005/05/24 09:36:30  roebel
 *
 * Fixed last checkin comment which turned out to be the start of
 * a c-comment.
 *
 * Revision 3.13  2005/05/23 19:17:53  schwarz
 * - Sdiffread/Sdiffwrite functions with SdifFileT instead of FILE *
 *   -> eof error reporting makes more sense
 * - more cleanup of sdif.h, above functions are private in SdifRWLowLevel.h
 * - eEof becomes error 4 to be distinguishable from ascii chars
 * - SdifFScanNameValueLCurrNVT reimplemented for ascii only
 *
 * Revision 3.12  2005/05/23 17:52:53  schwarz
 * Unified error handling:
 * - SdifErrorEnum (global errors) integrated into SdifErrorTagET (file errors)
 * - no more SdifError.[ch], everything done by SdifErrMess.[ch]
 *
 * Revision 3.11  2004/09/10 13:27:40  schwarz
 * missing return
 *
 * Revision 3.10  2004/09/09 17:46:44  schwarz
 * Changed SdifMatrixDataT to something sensible that allows to read and
 * store a whole matrix's data as one block, with automatic reallocation.
 * Copy matrix data to float array, to use in jMax/FTM.
 * Moved SdifSizeOfMatrix* functions here.
 *
 * Revision 3.9  2004/07/22 14:47:56  bogaards
 * removed many global variables, moved some into the thread-safe SdifGlobals structure, added HAVE_PTHREAD define, reorganized the code for selection, made some arguments const, new version 3.8.6
 *
 * Revision 3.8  2003/11/07 21:47:18  roebel
 * removed XpGuiCalls.h and replaced preinclude.h  by local files
 *
 * Revision 3.7  2002/05/24 19:37:07  ftissera
 * Add include "sdif.h" to be compatible with C++
 *
 * Revision 3.6  2001/05/02 09:34:45  tisseran
 * Change License from GNU Public License to GNU Lesser Public License.
 *
 * Revision 3.5  2000/11/15 14:53:32  lefevre
 * no message
 *
 * Revision 3.4  2000/10/27  20:03:37  roebel
 * autoconf merged back to main trunk
 *
 * Revision 3.3.2.1  2000/08/21  21:35:33  tisseran
 * *** empty log message ***
 *
 * Revision 3.3  1999/10/13  16:05:51  schwarz
 * Changed data type codes (SdifDataTypeET) to SDIF format version 3, as
 * decided with Matt Wright June 1999, added integer data types.
 * Added writing of 1NVT with real frame header (but data is still not in
 * matrices).
 * The data type handling makes heavy use of code-generating macros,
 * called for all data types with the sdif_foralltypes macro, thus
 * adding new data types is easy.
 *
 * Revision 3.2  1999/09/28  13:09:03  schwarz
 * Included #include <preincluded.h> for cross-platform uniformisation,
 * which in turn includes host_architecture.h and SDIF's project_preinclude.h.
 *
 * Revision 3.1  1999/03/14  10:57:06  virolle
 * SdifStdErr add
 *
 * Revision 2.4  1999/02/28  12:16:49  virolle
 * memory report
 *
 * Revision 2.3  1999/01/23  15:55:54  virolle
 * add querysdif.dsp, delete '\r' chars from previous commit
 *
 * Revision 2.2  1999/01/23  13:57:39  virolle
 * General Lists, and special chunk preparation to become frames
 *
 *
 */


#include "sdif_portability.h"

#include "SdifMatrix.h"
#include <stdlib.h>
#include <assert.h>
#include "sdif.h"



/* size in bytes of matrix data, according to header */
size_t
SdifSizeOfMatrixData (SdifMatrixHeaderT *mtx)
{
    return (mtx->NbRow * mtx->NbCol * SdifSizeofDataType (mtx->DataType));
}


/* file size in bytes of matrix including header */
size_t
SdifSizeOfMatrix (SdifDataTypeET DataType,
		  SdifUInt4      NbRow,
		  SdifUInt4      NbCol)
{
    SdifUInt4 msz = 
           /* matrix header  */ sizeof(SdifSignature) + 3 * sizeof(SdifUInt4) +
           /* matrix data    */ NbRow * NbCol * SdifSizeofDataType (DataType);
    msz += /* matrix padding */ SdifPaddingCalculate (msz);
    return (msz);
}



SdifMatrixHeaderT*
SdifCreateMatrixHeader(SdifSignature  Signature,
		       SdifDataTypeET DataType,
		       SdifUInt4 NbRow,
		       SdifUInt4 NbCol)
{
  SdifMatrixHeaderT *NewMatrixHeader = NULL;
  
  NewMatrixHeader = SdifMalloc(SdifMatrixHeaderT);
  if (NewMatrixHeader)
    {
      NewMatrixHeader->Signature = Signature;
      NewMatrixHeader->DataType  = DataType;
      NewMatrixHeader->NbRow = NbRow;
      NewMatrixHeader->NbCol = NbCol;
      
      return NewMatrixHeader;
    }
  else
    {
      _SdifError(eAllocFail, "MatrixHeader allocation");
      return NULL;
    }
}


SdifMatrixHeaderT*
SdifCreateMatrixHeaderEmpty(void)
{
  return SdifCreateMatrixHeader(eEmptySignature, eFloat4, 0, 0);
}


void
SdifKillMatrixHeader(SdifMatrixHeaderT *MatrixHeader)
{
  if (MatrixHeader)
    SdifFree(MatrixHeader);
  else
    _SdifError(eFreeNull, "MatrixHeader free");
}





/******************************************************************************
 *
 * OneRow class
 *
 */

SdifOneRowT*
SdifCreateOneRow(SdifDataTypeET DataType, SdifUInt4  NbGranuleAlloc)
{
  SdifOneRowT * NewOneRow = NULL;

  if (NbGranuleAlloc <= 0)
    {
      _SdifError(eAllocFail, "NbGranuleAlloc of OneRow <= 0. You must protect it at upper level.");
      return NULL;
    }

  NewOneRow = SdifMalloc(SdifOneRowT);
  if (!NewOneRow)
  {
      _SdifError(eAllocFail, "OneRow allocation");
      return NULL;
  }

  NewOneRow->DataType = DataType;
  NewOneRow->NbGranuleAlloc = NbGranuleAlloc;
  NewOneRow->NbData = 0;

#if (_SdifFormatVersion >= 3)

  if (!SdifDataTypeKnown (DataType))
  {
      char errorMess[_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess), 
	       "Data of a OneRow : 0x%x", NewOneRow->DataType); 
      _SdifError(eNotInDataTypeUnion, errorMess);
      SdifFree(NewOneRow);
      return NULL;
  }

  NewOneRow->Data.Void = 
      (void *) SdifCalloc (char, NewOneRow->NbGranuleAlloc * _SdifGranule);
  if (!NewOneRow->Data.Void)
  {
      _SdifError(eAllocFail, "OneRow->Data allocation");
      SdifFree(NewOneRow);
      return NULL;
  }
  else
      return NewOneRow;
      
#else
      switch (NewOneRow->DataType)
      {
	case eFloat4 :
	  NewOneRow->Data.Float4 = (SdifFloat4*) SdifCalloc(char, NewOneRow->NbGranuleAlloc * _SdifGranule);
	  if (! NewOneRow->Data.Float4)
	    {
	      _SdifError(eAllocFail, "OneRow->Data.Float4 allocation");
	      SdifFree(NewOneRow);
	      return NULL;
	    }
	  else
	    return NewOneRow;

	case eFloat8 :
	  NewOneRow->Data.Float8 = (SdifFloat8*) SdifCalloc(char, NewOneRow->NbGranuleAlloc * _SdifGranule);
	  if (! NewOneRow->Data.Float8)
	    {
	      _SdifError(eAllocFail, "OneRow->Data.Float8 allocation");
	      SdifFree(NewOneRow);
	      return NULL;
	    }
	  else
	    return NewOneRow;
	default :
	{
	  char errorMess [_SdifStringLen];

	  snprintf(errorMess, sizeof(errorMess), 
		   "Data of a OneRow : 0x%x", NewOneRow->DataType);
	  _SdifError(eNotInDataTypeUnion, errorMess);
	  SdifFree(NewOneRow);
	  return NULL;
	}
      }
#endif
}




SdifOneRowT*
SdifReInitOneRow (SdifOneRowT *OneRow, SdifDataTypeET DataType, SdifUInt4 NbData)
{
  SdifUInt4 NewNbGranule;

  OneRow->NbData   = NbData;
  OneRow->DataType = DataType;


#if (_SdifFormatVersion >= 3)

  if (OneRow->NbGranuleAlloc * _SdifGranule < NbData * SdifSizeofDataType (DataType))
  {
      NewNbGranule = (NbData * SdifSizeofDataType (DataType)) / _SdifGranule;
   /* NewNbGranule = NewNbGranule ? NewNbGranule : 1; This case cannot appear*/
      OneRow->Data.Void = (void *) SdifRealloc (OneRow->Data.Void, char, 
						NewNbGranule * _SdifGranule);
      if (!OneRow->Data.Void)
      {
	  _SdifError(eAllocFail, "OneRow->Data RE-allocation");
	  return NULL;
      }
      else
      {
	  OneRow->NbGranuleAlloc = NewNbGranule;
	  return OneRow;
      }
  }
  else
      return OneRow;

#else

  switch (OneRow->DataType)
  {
    case eFloat4 :
      if ( (OneRow->NbGranuleAlloc * _SdifGranule) < (OneRow->NbData * sizeof(SdifFloat4)) )
	{
	  NewNbGranule = (SdifUInt4)(OneRow->NbData * sizeof(SdifFloat4)) / _SdifGranule;
	  /* NewNbGranule = (NewNbGranule) ? NewNbGranule : 1; This case cannot appear */
	  OneRow->Data.Float4 = (SdifFloat4*) SdifRealloc(OneRow->Data.Float4, char, NewNbGranule * _SdifGranule);
	  if (! OneRow->Data.Float4)
	    {
	      _SdifError(eAllocFail, "OneRow->Data.Float4 RE-allocation");
	      return NULL;
	    }
	  else
	    {
	      OneRow->NbGranuleAlloc = NewNbGranule;
	      return OneRow;
	    }
	}
      else
	return OneRow;
      
      
    case eFloat8 :
      if ( (OneRow->NbGranuleAlloc * _SdifGranule) < (OneRow->NbData * sizeof(SdifFloat8)) )
	{
	  NewNbGranule = (SdifUInt4)(OneRow->NbData * sizeof(SdifFloat8)) / _SdifGranule;
	  /* NewNbGranule = (NewNbGranule) ? NewNbGranule : 1; This case cannot appear */
	  OneRow->Data.Float8 = (SdifFloat8*) SdifRealloc(OneRow->Data.Float8, char, NewNbGranule * _SdifGranule);
	  if (! OneRow->Data.Float8)
	    {
	      _SdifError(eAllocFail, "OneRow->Data.Float8 RE-allocation");
	      return NULL;
	    }
	  else
	    {
	      OneRow->NbGranuleAlloc = NewNbGranule;
	      return OneRow;
	    }
	}
      else
	return OneRow;
    
    default :
    {
      char errorMess [_SdifStringLen];
  
      snprintf(errorMess, sizeof(errorMess), 
	       "Data of a OneRow : 0x%x", OneRow->DataType);
      _SdifError(eNotInDataTypeUnion, errorMess);
      return NULL;
    }
  }

#endif
}


void
SdifKillOneRow(SdifOneRowT *OneRow)
{
  if (OneRow)
    {
#if (_SdifFormatVersion >= 3)

#ifndef NDEBUG	
	if (!SdifDataTypeKnown (OneRow->DataType))
	{
	    char errorMess [_SdifStringLen];
    
	    snprintf(errorMess, sizeof(errorMess), 
		     "Data of a OneRow : 0x%x", OneRow->DataType);
	    _SdifError (eNotInDataTypeUnion, errorMess);
	}
#endif

	if (OneRow->Data.Void)
	{
	    SdifFree (OneRow->Data.Void);
	}
	else
	    _SdifError(eFreeNull, "OneRow->Data free");

#else

      switch (OneRow->DataType)
      {
	case eFloat4 :
	  if (OneRow->Data.Float4)
	    {
	      SdifFree(OneRow->Data.Float4);
	    }
	  else
	    _SdifError(eFreeNull, "OneRow->Data.Float4 free");
	  break;
	case eFloat8 :
	  if (OneRow->Data.Float8)
	    {
	      SdifFree(OneRow->Data.Float8);
	    }
	  else
	    _SdifError(eFreeNull, "OneRow->Data.Float8 free");
	  break;
	default :
	{
	  char errorMess [_SdifStringLen];

	  snprintf(errorMess, sizeof(errorMess), 
		   "Data of a OneRow : 0x%x", OneRow->DataType);
	  _SdifError(eNotInDataTypeUnion, errorMess);
	}
	break;
      }
#endif
     SdifFree(OneRow);

    }
  else
    _SdifError(eFreeNull, "OneRow free");
}


/* row element access */

SdifOneRowT*
SdifOneRowPutValue(SdifOneRowT *OneRow, SdifUInt4 numCol, SdifFloat8 Value)
{
    /* case template for type from SdifDataTypeET */
#   define setrowcase(type) 						   \
    case e##type:   OneRow->Data.type [numCol-1] = (Sdif##type) Value;  break;
   
  /* numCol is in [1, NbData] */
  if ((numCol <= OneRow->NbData) && (numCol > 0))
    switch (OneRow->DataType)
      {
	  /* generate cases for all types */
	  sdif_foralltypes (setrowcase);
  
      default :
	OneRow->Data.Float4[numCol-1] = (SdifFloat4) Value;      
	break;
      }
  else
    {
      char errorMess [_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess), 
	       "OneRow Put Value Col : %d ", numCol);
      _SdifError(eArrayPosition, errorMess);
    }
  return OneRow;
}


SdifFloat8
SdifOneRowGetValue(SdifOneRowT *OneRow, SdifUInt4 numCol)
{
    /* case template for type from SdifDataTypeET */
#   define getrowcase(type) 						   \
    case e##type:   return (SdifFloat8) OneRow->Data.type [numCol-1];
   
  /* numCol is in [1, NbData] */
  if ((numCol <= OneRow->NbData) && (numCol > 0))
    switch (OneRow->DataType)
      {
	  /* generate cases for all types */
	  sdif_foralltypes (getrowcase);
  
      default :
	return (SdifFloat8) OneRow->Data.Float4[numCol-1];
      }
  
  {
    char errorMess [_SdifStringLen];
    
    snprintf(errorMess, sizeof(errorMess), 
             "OneRow Get Value Col : %d ", numCol);
    _SdifError(eArrayPosition, errorMess);
  }
  return _SdifFloat8Error;
}


SdifFloat8
SdifOneRowGetValueColName(SdifOneRowT *OneRow,
			  SdifMatrixTypeT *MatrixType,
			  char *NameCD)
{
  return SdifOneRowGetValue(OneRow,
			    SdifMatrixTypeGetNumColumnDef(MatrixType, NameCD));
}




/******************************************************************************
 *
 * SdifMatrixDataT class 
 *
 */

SdifMatrixDataT* 
SdifCreateMatrixData(SdifSignature Signature,
		     SdifDataTypeET DataType,
		     SdifUInt4 NbRow,
		     SdifUInt4 NbCol)
{
    SdifMatrixDataT *NewMatrixData = NULL;
  
    NewMatrixData = SdifCalloc(SdifMatrixDataT, 1);	/* all fields zero */

    if (NewMatrixData)
    {
	NewMatrixData->Header = SdifCreateMatrixHeader(Signature,
						       DataType,
						       NbRow,
						       NbCol);
	NewMatrixData->ForeignHeader = 0;
	NewMatrixData->Size      = SdifSizeOfMatrix(DataType, NbRow, NbCol);
	NewMatrixData->AllocSize = SdifSizeOfMatrixData(NewMatrixData->Header);
	NewMatrixData->Data.Char = SdifCalloc(char, NewMatrixData->AllocSize);

	if (!NewMatrixData->Data.Char)
	{
	    SdifFree(NewMatrixData);
	    NewMatrixData = NULL;
	    _SdifError(eAllocFail, "MatrixData->Data allocation");
	}
    }
    else
    {
	_SdifError(eAllocFail, "MatrixData allocation");
    }
    
    return NewMatrixData;
}


void 
SdifKillMatrixData(SdifMatrixDataT *MatrixData)
{
    if (MatrixData)
    {
	if (MatrixData->Data.Void)
	    SdifFree(MatrixData->Data.Void);
      
	if (MatrixData->Header  &&  !MatrixData->ForeignHeader)
	    SdifKillMatrixHeader(MatrixData->Header);

	SdifFree(MatrixData);
    }
}


/* set header pointer, reallocate to its size */
int SdifMatrixDataUpdateHeader (SdifMatrixDataT *Data, SdifMatrixHeaderT *NewH)
{
    size_t datasize = SdifSizeOfMatrixData(NewH); /* size needed */
    
    Data->Header	= NewH;
    Data->ForeignHeader = 1;

    return SdifMatrixDataRealloc(Data, datasize);
}


/* see if there's enough space for data, if not, grow buffer */
int SdifMatrixDataRealloc (SdifMatrixDataT *data, int newsize)
{
    char *newdata;

    if (data->AllocSize < (SdifUInt4)newsize )
    {   /* grow buffer to multiple of _SdifGranule (1024 byte block) */
	newsize = (newsize / _SdifGranule + 1) * _SdifGranule;
	newdata = SdifRealloc(data->Data.Char, char, newsize);

	if (newdata)
	{
	    data->Data.Char = newdata;
	    data->AllocSize = newsize;
	    return 1;
	}
	else
	{
	    char errorMess [_SdifStringLen];

	    snprintf(errorMess, sizeof(errorMess), 
		    "MatrixData reallocation from %d to %d returned %p",
		    data->AllocSize, newsize, newdata);

	    SdifFree(data->Data.Char);
	    data->Data.Char = NULL;
	    data->AllocSize = 0;

	    _SdifError(eAllocFail, errorMess);
	    return 0;
	}
    }
    else
	return 1;    
}


/* matrix data element access by index */

SdifMatrixDataT *
SdifMatrixDataPutValue(SdifMatrixDataT *data,
		       SdifUInt4  numRow,
		       SdifUInt4  numCol,
		       SdifFloat8 Value)
{
    /* case template for type from SdifDataTypeET */
#   define setdatacase(type) 						   \
    case e##type:   data->Data.type [index] = (Sdif##type) Value;  break;
   
    /* numRow, numCol count from 1! */
    if (0 < numRow  &&  numRow <= data->Header->NbRow  &&  
	0 < numCol  &&  numCol <= data->Header->NbCol)
    {
	int index = (numRow-1) * data->Header->NbCol + (numCol-1);

	switch (data->Header->DataType)
	{
	    /* generate cases for all types */
	    sdif_foralltypes (setdatacase);
  
	    default:
		data->Data.Float4[index] = (SdifFloat4) Value;
	    break;
	}
    }
    else
    {
	char errorMess [_SdifStringLen];

	snprintf(errorMess, sizeof(errorMess), 
		 "SdifMatrixDataPutValue: row %d, col %d ", numRow, numCol);
	_SdifError(eArrayPosition, errorMess);
    }
    
    return data;
}


SdifFloat8
SdifMatrixDataGetValue(SdifMatrixDataT *data,
		       SdifUInt4  numRow,
		       SdifUInt4  numCol)
{
    /* numRow, numCol count from 1! */
    if (0 < numRow  &&  numRow <= data->Header->NbRow  &&  
	0 < numCol  &&  numCol <= data->Header->NbCol)
    {
	int index = (numRow-1) * data->Header->NbCol + (numCol-1);

	/* case template for type from SdifDataTypeET */
#	define getdatacase(type)					   \
	case e##type:   return (SdifFloat8) data->Data.type [index];
   
	switch (data->Header->DataType)
	{
	    /* generate cases for all types */
	    sdif_foralltypes (getdatacase);
  
	    default:
		return (SdifFloat8) data->Data.Float4[index];
	}
    }

    { 
      char errorMess [_SdifStringLen];
    
      snprintf(errorMess, sizeof(errorMess), 
               "SdifMatrixDataGetValue:  row %d, col %d", numRow, numCol);
      _SdifError(eArrayPosition, errorMess);
    }

    return _SdifFloat8Error;
}


/* matrix data element access by column name */

SdifMatrixDataT *
SdifMatrixDataColNamePutValue(SdifHashTableT *MatrixTypesTable,
			      SdifMatrixDataT *MatrixData,
			      SdifUInt4  numRow,
			      char *ColName,
			      SdifFloat8 Value)
{
  SdifMatrixTypeT* MtrxT;
  SdifUInt4 numCol;
  
  MtrxT = SdifGetMatrixType(MatrixTypesTable, MatrixData->Header->Signature);

  if (MtrxT)
    {
      numCol = (SdifUInt4) SdifMatrixTypeGetNumColumnDef(MtrxT, ColName);
      if (numCol)
	SdifMatrixDataPutValue(MatrixData, numRow, numCol, Value);
      else
	{
	  char errorMess [_SdifStringLen];

	  snprintf(errorMess, sizeof(errorMess), "'%s' of '%s' matrix type",
		  ColName, SdifSignatureToString(MatrixData->Header->Signature));
	  _SdifError(eNotFound, errorMess);
	}
    }
  else
    {
      char errorMess [_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess), "'%s' Matrix type", 
	       SdifSignatureToString(MatrixData->Header->Signature));
      _SdifError(eNotFound, errorMess);
    }
  return MatrixData;
}


SdifFloat8
SdifMatrixDataColNameGetValue(SdifHashTableT *MatrixTypesTable,
			      SdifMatrixDataT *MatrixData,
			      SdifUInt4  numRow,
			      char *ColName)
{
  SdifMatrixTypeT* MtrxT;
  SdifUInt4 numCol;
  
  MtrxT = SdifGetMatrixType(MatrixTypesTable, MatrixData->Header->Signature);

  if (MtrxT)
    {
      numCol = (SdifUInt4) SdifMatrixTypeGetNumColumnDef(MtrxT, ColName);
      if (numCol)
	return SdifMatrixDataGetValue(MatrixData, numRow, numCol);
      else
	{
	  char errorMess [_SdifStringLen];

	  snprintf(errorMess, sizeof(errorMess),
		  "'%s' of '%s' matrix type",
		  ColName, SdifSignatureToString(MatrixData->Header->Signature));
	  _SdifError(eNotFound, errorMess);
	  return _SdifFloat8Error;
	}
    }
  else
    {
      char errorMess [_SdifStringLen];

      snprintf(errorMess, sizeof(errorMess), "'%s' Matrix type", 
	       SdifSignatureToString(MatrixData->Header->Signature));
      _SdifError(eNotFound, errorMess);
      return _SdifFloat8Error;
    }
}


void SdifCopyMatrixDataToFloat4 (SdifMatrixDataT *data, SdifFloat4 *dest)
{
    int i, n = data->Header->NbRow * data->Header->NbCol; 
    
    switch (data->Header->DataType)
    {
	/* case template for type from SdifDataTypeET */
#	define copydata(type)					   \
	    case e##type:					   \
	        for (i = 0; i < n; i++)				   \
		    dest [i] = (SdifFloat4) data->Data.type [i];   \
	    break;

	/* generate cases for all types */
	sdif_foralltypes (copydata);

        default:
	    _SdifError(eNotInDataTypeUnion, 
		       "SdifCopyMatrixDataToFloat4 source");
	break;
    }
}

/* todo:
void SdifCopySelectedMatrixDataToFloat8 (SdifMatrixDataT *data, SdifFloat4 *dest)
*/
