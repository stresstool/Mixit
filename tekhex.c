#if 0
/*===========================================================================

T E K H E X . C


	Extended TEKHEX format records.

	The data is recorded as a series of ASCII records. The
	format of the records is as follows:

	%cctssvvdddd....dd

	where "%" is the record sentinel, "cc" is the count of the
	number of ascii bytes in the record (excluding the '%'
	sentinel), "t" is the record type (3 = symbol, 6=data, 8=termination),
	"ss" is the 8 bit checksum of all the binary nibbles in the
	record (except the "%" and the "ss" bytes, but including
	cc and t), "vv" is a variable length field containing the
	load address of the data byte to follow and "dd" is the data
	bytes. The variable length field format is: cd...d where
	"c" is the number of ascii chars that follow (0-F where 0 =
	16 decimal, 1 = 1, etc.) and "d" is the number to
	represent. I.e. "3421" expands to hex 0421. The minimum
	field width is 2 bytes and the maximum is 17 bytes
	(including the count byte).  All values are expressed in
	hex and there is 1 ASCII character for each 4 bit nibble. 

	Copyright 1989 Atari Games.  All rights reserved.
	Author: Lyle Rains

 ===========================================================================*/
#endif

#include "mixit.h"

#define BYTES_PER_REC   32

static char* to_tekhex(ulong number);

/*==========================================================================*/
int GetRec_tekhex(InRecord *rec)
{
	int 	cnt, chk, c;
	uchar    *token;
	uchar    *inbuf;
	uchar    *bufend;
	char    tmp[4];                     /* for count/type fields 			 */

	do
	{                               /* Purge everything until a '%'. 	 */
		if ( (c = getc(rec->recFile)) == EOF )
		{
			rec->recLen = 0;
			return (rec->recType = feof(rec->recFile) ? REC_EOF : REC_ERR);
		}
	} while ( c != '%' );
	inbuf = rec->recBuf;
	*inbuf++ = c;

	/* Get count and record type. 		 */
	if ( fread((char *)inbuf, 1, 3, rec->recFile) != 3 )
	{
		rec->recLen = 0;
		return (rec->recType = feof(rec->recFile) ? REC_EOF : REC_ERR);
	}
	if ( inbuf[2] == '3' )                /* symbol record 					 */
		memcpy(tmp, inbuf, 3);
	if ( strtohex(inbuf, 3) )
	{
		rec->recLen = 0;
		return (rec->recType = REC_ERR);
	}
	cnt = HEXTOBYTE(inbuf);
	bufend = inbuf + cnt;
	token = inbuf + 3;
	if ( (int)fread(token, 1, cnt - 3, rec->recFile) != cnt - 3 )
	{
		rec->recLen = 0;
		return (rec->recType = feof(rec->recFile) ? REC_EOF : REC_ERR);
	}
	if ( inbuf[2] == 6 )                  /* Data record 						 */
	{                               /* read remainder of record; get & verify checksum.*/
		if ( strtohex(token, cnt - 3) )
		{
			rec->recLen = 0;
			return (rec->recType = REC_ERR);
		}
		chk = HEXTOBYTE(token);
		token[0] = token[1] = 0;        /* clear checksum nybbles 			 */

		c = cnt;
		while ( --c >= 0 )
			chk -= inbuf[c];
		if ( chk & 0xFF )
		{
			moan("Checksum error");
			rec->recLen = 0;
			return (rec->recType = REC_ERR);
		}
		token += 2;

		/* Get address. 					 */
		if ( (c = *token++) == 0 )
			c = 16;
		rec->recSAddr = hextoaddr(token, c);
		token += c;

		/* Convert rest of the data & set return values in record.*/
		rec->recType = REC_DATA;
		rec->recLen = cnt = (bufend - token) / 2;
		rec->recEAddr = rec->recSAddr + rec->recLen - 1;
		hextobytes(token, cnt);
		rec->recData = token;
	}
	else if ( inbuf[2] == 3 )             /* Symbol record 					 */
	{
		/*  
		 *  Add CR/LF and pass thru transparently.
		 *  I'm not even bothering with checksum at this point.
		 */
		memcpy(inbuf, tmp, 3);          /* change type and count back to ascii*/
		memcpy(&inbuf[cnt], "\r\n", 3);
		rec->recLen = cnt + 3;          /* including '%' and "\r\n" 		 */
		rec->recType = REC_TRANSPARENT;
		rec->recData = rec->recBuf;
	}
	else                                /* ignore anything else 			 */
	{
		rec->recLen = 0;
		rec->recType = REC_UNKNOWN;
	}
	return (rec->recType);
} /* end GetRec_tekhex */


/*==========================================================================
 * Converts a binary number into variable length hex format.
 *==========================================================================*/
static char* to_tekhex(ulong number)
{
	static char str[18];
	static char hex_len[] = { '0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'A', 'B', 'C', 'D', 'E', '0' };
	int len;
	sprintf(&str[1], "%lX", number);
	len = strlen(str + 1);
	str[0] = hex_len[len];
	return str;
} /* end to_tekhex */


/*==========================================================================*
 * Outputs all footer information required by a TEKHEX format file.
 *==========================================================================*/
int PutFoot_tekhex(FILE *file)
{
	fputs("%0781010\n", file);
	return 1;
} /* end PutFoot_tekhex */


/*==========================================================================*
 * Outputs a symbol pointed to by data to 'file'.
 *==========================================================================*/
int PutSym_tekhex(FILE *file, uchar *data, int recsize)
{
	fputs((char *)data, file);
	return recsize != 0;
} /* end PutSym_tekhex */


/*==========================================================================*
 * Outputs a single record in TEKHEX format.
 *==========================================================================*/
int PutRec_tekhex(FILE *file, uchar *data, int recsize, ulong recstart)
{
	int 	len, j;
	char    *cp;
	char    outbuf[140], csum[3];
	uint    cksum;

#if 1
	sprintf(outbuf, "%%00600%s", to_tekhex(recstart));
#else
	sprintf(outbuf, "%%006004%s", to_tekhex(recstart&0xFFFF));
#endif 								/* Append data to the record 		 */
	len      = strlen(outbuf);
	for ( cp = outbuf + len, j = 0; j < recsize; ++j )
		cp = to_hex(*data++, cp);

	*cp++ = '\n';                       /* Add a line terminator 			 */
	*cp   = 0;
	/* Calculate the length 			 */
	to_hex((uchar)((cp - outbuf - 2) & 0xFF), csum);
	outbuf[1] = csum[0];
	outbuf[2] = csum[1];
	/* Calculate the checksum 			 */
	for ( cp -= 2, cksum = 0; cp > outbuf; --cp )
	{
		if ( in('0', *cp, '9') )
			cksum += *cp - '0';
		else if ( in('A', *cp, 'Z') )
			cksum += *cp - 'A' + 10;
		else if ( in('a', *cp, 'z') )
			cksum += *cp - 'a' + 40;
		else
			switch (*cp)
			{
			case '$':
				cksum += 36;
				break;
			case '%':
				cksum += 37;
				break;
			case '.':
				cksum += 38;
				break;
			case '_':
				cksum += 39;
				break;
			}
	}
	to_hex((uchar)(cksum & 0xFF), csum);      /* Insert the checksum 				 */
	outbuf[4] = csum[0];
	outbuf[5] = csum[1];
	fputs(outbuf, file);                /* Output the record to the file 	 */
	return 1;
} /* end PutRec_tekhex */
