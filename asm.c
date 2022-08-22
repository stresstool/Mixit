#if 0
/*===========================================================================

A S M . C


	ASM68 record format.

	The data is recorded as a series of ASCII records. The format of the
	records is as follows:

	DC.B dd, dd, dd, ...

	Where each dd is a 2 character hex representations of the each data byte.

	Copyright 1991 Atari Games.  All rights reserved.
	Author: Jim Petrick

---------------------------------------------------------------------------
	Revision history:

---------------------------------------------------------------------------
	Known bugs/features/limitations:

===========================================================================*/
#endif

#include "mixit.h"

extern ulong    last_address, base_address;
extern char     current_fname[];

/*==========================================================================*/
int GetRec_asm(InRecord *rec)
{
	return (rec->recType = REC_ERR);
} /* end GetRec_asm */


/*==========================================================================
 * Puts out any header info for the file.
 *==========================================================================*/
int PutHead_asm( FILE *file, ulong addr, ulong hi )
{
	fprintf( file, "* File name = %s\n\n", current_fname);
	fprintf( file, "MIXIT_BEGIN:\n");

	base_address = addr;
	last_address = (ulong)-1L;
	return 1;

} /* end PutHead_asm */


/*==========================================================================*
 * Outputs a single record in ASM format.
 *==========================================================================*/
int PutRec_asm( FILE *file, uchar *data, int recsize, ulong recstart )
{
	int		j;
	char    *cp;
	char    outbuf[140];

	if (recstart != last_address)
		fprintf( file, "\n\t* EQU MIXIT_BEGIN + $%04lX + $%04lX\n",
		base_address, recstart - base_address);
	last_address = recstart + recsize;

        sprintf(outbuf, " DC.B $%02X", *data++ );
	cp = outbuf + strlen(outbuf);
 						/* Append data to the record 		 */
	for (j=1; j < recsize; ++j) {
		sprintf( cp, ",$%02X", *data++ );                 
		cp += strlen(cp);
        }
	strcpy( cp, "\n" );
	fputs(outbuf, file); 			/* Output the record to the file 	 */
	return 1;

} /* end PutRec_asm */
