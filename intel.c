#if 0
/*===========================================================================

I N T E L . C

	INTEL format records.

	The data is recorded as a series of ASCII records. The format of the
	records is as follows:

	:ccaaaattdddd....ddddcs

	where ":" is the record sentinal, "cc" is the count of the number of
	data bytes in the record, "aaaa" is the 16 bit load address of byte
	0 in the record, "tt" is the record type (00 for data, 01 for
	termination), "dd" is the byte of data and "cs" is the twos compliment
	8-bit checksum.

	All binary bytes in the record are checksumed with 8-bit precision
	including the count and each byte of the 2 byte load address, and the type
	field. All values are expressed in hex and there is 1 ASCII character for
	each 4 bit nibble.

	Whitespace is ignored, as is text in a record after a '#'

	Copyright 1989 Atari Games.  All rights reserved.
	Author: Lyle Rains

---------------------------------------------------------------------------
	Revision history:

---------------------------------------------------------------------------
	Known bugs/features/limitations:

===========================================================================*/
#endif

#include "mixit.h"


/*==========================================================================*/
int GetRec_intel(InRecord *rec)
{
	int		cnt, datacnt, chk, c;
	uchar   *inbuf = rec->recBuf;
	uchar   * lookahead,*bufend,*data;

	if ( fgets((char *)inbuf, rec->recBufLen, rec->recFile) == NULL )
	{
		rec->recLen = 0;
		return (rec->recType = feof(rec->recFile) ? REC_EOF : REC_ERR);
	}

	/*
	 * Convert to 'pure' hexidecimal record:  purge everything until a ':',
	 * squeeze out junk, and quit when you find a '#' or end of record.
	 */

	lookahead = bufend = inbuf;
	c = 0;
	while ( *lookahead != '#' && *lookahead != 0 )
	{
		if ( c )
		{
			if ( isxdigit(*bufend = *lookahead) )
				++bufend;
		}
		else if ( *lookahead == ':' )
			c = 1;
		++lookahead;
	}
	if ( (cnt = bufend - inbuf) == 0 )
	{
		rec->recLen = 0;
		return (rec->recType = REC_UNKNOWN);
	}
	cnt /= 2;

	/* Convert to byte string. */
	strtobytes(inbuf, cnt);
	bufend = inbuf + cnt;

	/* Get count, addr, and record type. */
	datacnt = inbuf[0];
	rec->recSAddr = bytestoaddr(&inbuf[1], 2);

	/* Verify record length and checksum. */
	if ( datacnt > cnt - 5 /* 1-byte count, 2-byte addr, 1-byte type, 1-byte chksum */ )
	{
		moan("Record length error (sez %d, is %d)",
			 datacnt, cnt - 5);
		rec->recLen = 0;
		return (rec->recType = REC_ERR);
	}
	chk = 0;
	for ( data = inbuf; data < bufend; ++data )
		chk += *data;
	if ( chk & 0xFF )
	{
		moan("Checksum error");
		rec->recLen = 0;
		return (rec->recType = REC_ERR);
	}
	switch (inbuf[3]/* record type */)
	{
	case 0:             /* data */
		rec->recLen = datacnt;
		rec->recEAddr = rec->recSAddr+datacnt-1;
		rec->recData = inbuf + 4 /* skip header */;
		rec->recType = REC_DATA;
		break;

	case 2:             /* segment offset addr */
		rec->recSegBase = bytestoaddr(&inbuf[4], 2)/* paragraph */ * 16 /* size of paragraph */;
		rec->recLen = 0;
		rec->recType = REC_UNKNOWN;
		break;

	case 1:             /* termination */
	case 3:             /* transfer addr */
		rec->recLen = 0;
		rec->recType = REC_UNKNOWN;
		break;

	default:            /* illegal */
		rec->recLen = 0;
		rec->recType = REC_ERR;
		break;
	}
	return (rec->recType);
} /* end GetRec_intel */


/*==========================================================================*
 * Outputs all footer information required by a INTEL format file.
 *==========================================================================*/
int PutFoot_intel(FILE *file)
{
	fputs(":00000001FF\n", file); /* Add a record termination mark */
	return 1;

} /* end PutFoot_intel */


/*==========================================================================*
 * Outputs a single record in INTEL format.
 *==========================================================================*/
int PutRec_intel(FILE *file, uchar *data, int recsize, ulong recstart)
{
	int 	j;
	char    *cp;
	char    outbuf[512];
	uint    cksum;

	cp  = outbuf;
	sprintf(cp, ":%02X%04lX00", recsize, recstart & 0xFFFF);
	cp += strlen(cp);

	/* Append data to the record */
	cksum = recsize + ((recstart >> 8) & 0xFF) + (recstart & 0xFF);
	for ( j = 0; j < recsize; ++j )
	{
		cksum += *data;
		cp     = to_hex(*data++, cp);
	}
	/* Append the checksum */
	sprintf(cp, "%02X\n", (~cksum + 1) & 0xFF);
	fputs(outbuf, file);            /* Output the record to the file */
	return 1;

} /* end PutRec_intel */
