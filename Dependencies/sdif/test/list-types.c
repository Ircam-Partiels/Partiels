/* $Id: list-types.c,v 1.4 2008-12-18 11:42:58 diemo Exp $

   example code list-types.c: given an SDIF file and a frame signature as
   arguments, get matrices and matrix column names

   compile as:	gcc -I../include -g  -o list-types list-types.c -lsdif

   run as:	./list-types <testfile.sdif> 1REB ; echo $?

   $Log: not supported by cvs2svn $
   Revision 1.3  2008/12/18 11:20:25  diemo
   separate frameinfo_from_signature function

   Revision 1.2  2008/12/18 11:07:25  diemo
   read headers

   Revision 1.1  2008/12/18 10:59:16  diemo
   simple example showing SdifTestFrameType and direct access to struct elements
*/


#include "sdif.h"


/* open file and read header */
SdifFileT *open_file (char *infile)
{
    SdifFileT *file;

    SdifGenInit(NULL);
    if (!(file = SdifFOpen (infile, eReadFile)))
    {
	fprintf (SdifStdErr, "Can't open input file '%s'.\n", infile);
        SdifGenKill();
        exit(1);
    }

    /* read header */
    if (SdifFReadGeneralHeader(file) == 0  ||
	SdifFReadAllASCIIChunks(file) == 0)
    {
	fprintf (SdifStdErr, "Can't read header of input file '%s'.\n", infile);
	SdifFClose(file);
        SdifGenKill();
        exit(1);
    }

    return file;
}

void matrixinfo_from_signature (SdifFileT *file, SdifSignature msig)
{
    SdifMatrixTypeT *mtype   = SdifTestMatrixType(file, msig);
    int		     mnumcol = SdifMatrixTypeGetNbColumns(mtype);
    int i;

    for (i = 1; i <= mnumcol; i++)
    {
	const char *colname = SdifMatrixTypeGetColumnName(mtype, i);
	
	printf("    matrix column %d: %s\n", i, colname);
   }
}

void frameinfo_from_signature (SdifFileT *file, SdifSignature fsig)
{
    SdifFrameTypeT *ftype = SdifTestFrameType(file, fsig);

    if (!ftype)	
    {
	printf("error: frame type %s not found!\n", SdifSignatureToString(fsig));
	exit(3);
    }

    int fnumcomp = SdifFrameTypeGetNbComponents(ftype);
    int i;

    printf("Frame type %s, %d components:\n", 
	   SdifSignatureToString(fsig), fnumcomp);

    for (i = 1; i <= fnumcomp; i++)
    {
	SdifComponentT  *fcomp = SdifFrameTypeGetNthComponent(ftype, i);
	if (!fcomp)	exit(4);

	SdifSignature    msig  = SdifFrameTypeGetComponentSignature(fcomp);

	printf("  component %d: matrix %s %s\n", 
	       i, SdifSignatureToString(msig), 
	       SdifFrameTypeGetComponentName(fcomp));

	matrixinfo_from_signature(file, msig);
    }

}

int main (int argc, char *argv[])
{
    char *filename = argc > 1 ? argv[1] : "-";
    char *fsigstr  = argc > 2 ? argv[2] : "1TYP";

    SdifFileT      *file   = open_file(filename);
    SdifSignature   fsig   = SdifStringToSignature(fsigstr);

    /* frame and matrix types tables in sdif file struct store only
       the file-defined types! */
    SdifHashTableT *ftable = SdifFGetFrameTypesTable(file);
    SdifHashTableT *mtable = SdifFGetMatrixTypesTable(file);

    if (!ftable || !mtable)	exit(2);
    printf("Found %d user-defined frame types, %d matrix types SDIF file %s.\n", 
	   SdifHashTableGetNbData(ftable), SdifHashTableGetNbData(mtable), 
	   filename);

    /* print information about the definition of the given frame and
       matrices therein */
    frameinfo_from_signature(file, fsig);

    SdifFClose(file);
}

