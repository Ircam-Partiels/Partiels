/* $Id: list-nvt.c,v 1.3 2009-04-09 09:34:04 diemo Exp $

   example code list-nvt.c: given an SDIF file, print name-value
   tables, or optionally the value of one name-key, found in any nvt

   run as:	./list-nvt <testfile.sdif> [<name to query>]

   $Log: not supported by cvs2svn $
   Revision 1.2  2009/01/07 16:13:00  diemo
   use new nvt access functions and hash table iterator

   Revision 1.1  2009/01/07 11:39:13  diemo
   nvt listing example for library version <= 3.10

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
        return 1;
    }

    /* read header */
    if (SdifFReadGeneralHeader(file) == 0  ||
	SdifFReadAllASCIIChunks(file) == 0)
    {
	fprintf (SdifStdErr, "Can't read header of input file '%s'.\n", infile);
	SdifFClose(file);
        SdifGenKill();
        return 1;
    }

    return file;
}


int main (int argc, char *argv[])
{
    char *filename   = argc > 1 ? argv[1] : NULL;
    if(filename == NULL)
    {
        printf("filename invalid\n");
        return 1;
    }
    char *nvtnamestr = argc > 2 ? argv[2] : NULL;

    SdifFileT        *file    = open_file(filename);
    if(file == NULL)
    {
        return 1;
    }

    /* get list of all nvts */
    SdifNameValuesLT *nvtlist = SdifFNameValueList(file);

    if (nvtnamestr)
    {   /* query one key */
	SdifNameValueT *nv = SdifNameValuesLGet(nvtlist, nvtnamestr);
	    
	if (nv)
	    printf("Name %s:\n%s\n", 
		   SdifNameValueGetName(nv), SdifNameValueGetValue(nv));
	else
	{
	    printf("Name %s not found!\n", nvtnamestr);
	    SdifFClose(file);
        return 1;
	}
    }
    else
    {   /* print nvts */
	SdifListT *nvtl   = SdifNameValueTableList(nvtlist);
	int	   numnvt = SdifListGetNbData(nvtl);
	SdifHashTableIteratorT *nvtiter = SdifCreateHashTableIterator(NULL);
	
	printf("Number of NVTs in file %s: %d\n", filename, numnvt);

	SdifListInitLoop(nvtl);
	while (SdifListIsNext(nvtl))
	{   /* print one NVT */
	    SdifNameValueTableT *currnvt = 
		(SdifNameValueTableT *) SdifListGetNext(nvtl);
	    SdifHashTableT      *nvht = SdifNameValueTableGetHashTable(currnvt);

	    printf("\nNameValueTable Number %d Stream %d Size %d:\n", 
		   SdifNameValueTableGetNumTable(currnvt), 
		   SdifNameValueTableGetStreamID(currnvt),
		   SdifHashTableGetNbData(nvht));

	    SdifHashTableIteratorInitLoop(nvtiter, nvht);
	    while (SdifHashTableIteratorIsNext(nvtiter))
	    {
		SdifNameValueT *nv = (SdifNameValueT *) 
		    SdifHashTableIteratorGetNext(nvtiter);
		printf("%s\t%s;\n", SdifNameValueGetName(nv), 
		                    SdifNameValueGetValue(nv));
	    }
	}

	SdifKillHashTableIterator(nvtiter);
    }
    
    SdifFClose(file);
}
