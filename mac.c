#if 0
/*===========================================================================

M A C . C


	MAC 65 record format.

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
int macGetRec(InRecord *rec)
{
	return (rec->recType = REC_ERR);

} /* end macGetRec */

char *header[] = {
	".MACRO BYT .0,.1,.2,.3,.4,.5,.6,.7,.8,.9,.A,.B,.C,.D,.E,.F",
	"   .irp a,<.0,.1,.2,.3,.4,.5,.6,.7,.8,.9,.A,.B,.C,.D,.E,.F>",
	"	.iif b,<a>,.rexit",
	"	.iif dif,<a>,<XX>,.byte 0x'a",
	"	.iif idn,<a>,<XX>,.blkb 1",
	"   .endr",
	".endm",
	"",
	".MACRO ROM_ORG base address",
	". = MIXIT_BEGIN + 0x'base + 0x'address",
	".endm",
	"",
	"	.RADIX 16",
	"	.ASECT",
	"",
	"MIXIT_BEGIN:",
	0 };

/*==========================================================================
 * Puts out any header info for the file.
 *==========================================================================*/
int PutHead_mac( FILE *file, ulong addr, ulong hi )
{
	int i;

	fprintf( file, "\n\n; File name = %s\n\n", current_fname);
	for( i=0; header[i]; ++i )
		fprintf( file, "%s\n", header[i]);

	base_address = addr;
	last_address = (ulong)-1L;

	return 1;

} /* end PutHead_mac */


/*==========================================================================*
 * Outputs a single record in MAC format.
 *==========================================================================*/
int PutRec_mac( FILE *file, uchar *data, int recsize, ulong recstart )
{
	int j;
	char    *cp;
	char    outbuf[140];

	if (recstart != last_address)
		fprintf( file, "\n\tROM_ORG %05lX %05lX\n",
		base_address, recstart - base_address);
	last_address = recstart + recsize;

	cp  = outbuf;
   	sprintf(cp, "\tBYT %02X", *data++ );
	cp += strlen(cp);
					/* Append data to the record */
	for (j=1; j < recsize; ++j)
   		{
   		sprintf( cp, ",%02X", *data++ );
   		cp += strlen(cp);
   		}
	*cp++ = '\n';
	*cp++ = 0;
	fputs(outbuf, file);            /* Output the record to the file */

	return 1;

} /* end PutRec_mac */
