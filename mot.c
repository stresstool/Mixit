#if 0
/*===========================================================================

M O T . C

	Motorola S record format.

	The data is recorded as a series of ASCII records. The format of the
	records is as follows:

	Stccaa...dd...cs

	where "S" is the record sentinal, "t" is the record type, "cc" is the
	count of the number of binary bytes in the record (including address,
	data and checksum), "aa..." is the multibyte load address of byte 0 in
	the record, "dd..." is the data and "cs" is the twos compliment 8-bit
	checksum (adding bytes of all fields after "t" should result in a
	sum of 0xFF).  All values are expressed in hex and there is 1 ASCII
	character for each 4 bit nibble.

		Non-hex characters (whitespace, etc.) is ignored.

	Valid record types (ignore any but data):
		0   - block header (ignore)
		1   - data (16-bit addr)
		2   - data (24-bit addr)
		3   - data (32-bit addr)
		5   - record count for block (ignore)
		7   - 32-bit block termination (ignore)
		8   - 24-bit block termination (ignore)
		9   - 16-bit block termination (ignore)

	Copyright 1989 Atari Games.  All rights reserved.
	Author: Lyle Rains

---------------------------------------------------------------------------
	Revision history:

---------------------------------------------------------------------------
	Known bugs/features/limitations:

===========================================================================*/
#endif

#include "mixit.h"

#define BYTES_PER_REC   32

/*==========================================================================*/
int GetRec_mot(InRecord *rec)
{
	int 	cnt=0, datacnt, chk, c, addrlen;
	uchar   *inbuf = rec->recBuf;
	uchar   *lookahead, *bufend, *data;

	do {
	    if (fgets((char*)inbuf, rec->recBufLen, rec->recFile) == NULL) 
		    {
		    rec->recLen = 0;
		    return (rec->recType = feof(rec->recFile) ? REC_EOF : REC_ERR);
		    }
	    /*
	     *  Convert to 'pure' hexidecimal record:  purge everything until a 
	     *  ':', squeeze out junk, and quit when you find a '#' or end of 
	     *  record.
	     */
	    lookahead = bufend = inbuf;
	    c = 0;
	    while (*lookahead != 0) 
		    {
		    if (c > 0) 
			    {
			    if (isxdigit(*bufend = *lookahead)) 
				    ++bufend;
			    }
		    else if (*lookahead == 'S')
			    c = 1;
		    else if (*lookahead == '#')
			    {
			    c = -1;
			    break;
			    }
		    ++lookahead;
		    }
	} while (cnt < 0);
	if ((cnt = bufend - inbuf) == 0) 
		{
		rec->recLen = 0;
		return (rec->recType = REC_UNKNOWN);
		}

	/* Exclude record type byte. */
	cnt = (cnt - 1) / 2;
	++inbuf;

	/* Convert to byte string. */
	SHOW( *bufend = 0; fputs((char *)inbuf, errFile); putc('\n', errFile); )
	strtobytes(inbuf, cnt);
	bufend = inbuf + cnt;
	SHOW(
		for (data = inbuf; data < bufend; ++data)
			fprintf(errFile, "%.2X", *data);
	putc('\n', errFile);
	)

	/* Get record type. */
	addrlen = 2;
	switch (inbuf[-1])          /* record type */
		{
		case '3': ++addrlen; 		/* data: 32-bit addr 					 */
		case '2': ++addrlen; 		/* data: 24-bit addr 					 */
		case '1': break; 			/* data: 16-bit addr 					 */
		case '0': 					/* header 								 */
		case '5': 					/* record count 						 */
		case '7': 					/* termination 							 */
		case '8': 					/* termination 							 */
		case '9': 					/* termination 							 */
			rec->recLen = 0;
			return (rec->recType = REC_UNKNOWN);
		default:            /* illegal */
			rec->recLen = 0;
			return (rec->recType = REC_ERR);
		}

	/* Data record:  Verify record length and checksum. */
	datacnt = inbuf[0];
	if (datacnt > cnt - 1)          /* less 1-byte chksum */
		{
		moan("Record length error (sez %d, is %d)",
			datacnt, cnt - 5);
		rec->recLen = 0;
		return (rec->recType = REC_ERR);
		}

	chk = 0;
	for (data = inbuf; data < bufend; ++data)
		chk += *data;
	SHOW( fprintf(errFile, "chk = 0x%.2X\n", chk); )
	if ((chk & 0xFF) != 0xFF) 
		{
		moan("Checksum error");
		rec->recLen = 0;
		return (rec->recType = REC_ERR);
		}

	/* Finish up addr and data. */
	rec->recSAddr = bytestoaddr(&inbuf[1], addrlen);
	SHOW( fprintf(errFile, "addr = %.4X\n", rec->recSAddr); )

	rec->recLen = datacnt - 1 - addrlen;
	rec->recEAddr = rec->recSAddr + rec->recLen -1;
	rec->recData = inbuf + 1 + addrlen;
	return (rec->recType = REC_DATA);
} /* end GetRec_mot */

/*==========================================================================*/
char *to_hex( uchar value, char *str )
{
	*str++ = hex_of[(value >> 4) & 0x0F];
	*str++ = hex_of[ value & 0x0F ];
	return str;
} /* end to_hex */


/*==========================================================================*
 * Outputs all footer information required by a MOT format file.
 *==========================================================================*/
int PutFoot_mot(FILE *file)
{
	fputs( "S9030000FC\n", file );  /* Add a S1 record termination mark */
	return 1;
} /* end PutFoot_mot */


/*==========================================================================*
 * Outputs a single record in MOT format.
 *==========================================================================*/
int PutRec_mot( FILE *file, uchar *data, int recsize, ulong recstart )
{
	int j;
	char    *cp;
	char    outbuf[512];
	uint    cksum;
	int recbytes, type;

        if ((recstart&0xFFFF0000) == 0) type = 1;
        else if ((recstart&0xFF000000) == 0) type = 2;
        else type = 3;
	recbytes = recsize + 2 + type;
	cp  = outbuf;
	sprintf(cp, "S%d%02X%0*lX", type, recbytes, (type-1)*2+4, recstart);
   	cp += strlen(cp);

	/* Append data to the record */
	cksum = recbytes + 
    		((recstart >> 24)&0xFF) +
    		((recstart >> 16)&0xFF) +
    		((recstart >>  8)&0xFF) +
    		(recstart&0xFF); 
	for (j=0; j < recsize; ++j)
		{
		cksum += *data;
		cp     = to_hex( *data++, cp );
		}       /* end each record */
	/* Append the checksum */
	sprintf( cp, "%02X\n", (~cksum) & 0xFF );
	fputs(outbuf, file);    /* Output the record to the file */
	return 1;

} /* end PutRec_mot */
