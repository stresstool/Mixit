#if 0
/*===========================================================================

H E X U T L . C

	Copyright 1989 Atari Games.  All rights reserved.
	Author: Lyle Rains

---------------------------------------------------------------------------
	Revision history:

---------------------------------------------------------------------------
	Known bugs/features/limitations:

===========================================================================*/
#endif

#include "mixit.h"

/*
 * The following is for ASCII only.
 */
uchar chartohex[] = {
	XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,
	XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,
	XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,
	0,  1,  2,  3,    4,  5,  6,  7,    8,  9, XX, XX,   XX, XX, XX, XX,

	XX, 10, 11, 12,   13, 14, 15, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,
	XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,
	XX, 10, 11, 12,   13, 14, 15, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,
	XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,

	XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,
	XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,
	XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,
	XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,

	XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,
	XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,
	XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,
	XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX,   XX, XX, XX, XX
};


uchar   hex_of[] = {	'0', '1', '2', '3', '4', '5', '6', '7',
						'8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

/*==========================================================================
 * Convert ASCII hex string to binary bytes.
 * Make sure all the chars are hexidecimal.
 *==========================================================================*/
#define ERRMSG_SIZE (256)
int strtobytes(uchar *inpStr, int nbytes)
{
	uchar *bytestr = inpStr, *str=inpStr;
	int i;
	char errMsg[ERRMSG_SIZE];
	int errCnt = 0;

	for ( i = 0; i < nbytes; ++i )
	{
		char msb,lsb;
		msb = chartohex[str[0]];
		lsb = chartohex[str[1]];
		if ( msb == XX || lsb == XX )
		{
			int ii;
			errCnt = snprintf(errMsg, ERRMSG_SIZE, "strtobytes(): Not hex digit at str[%d] of len %d '", i, nbytes*2);
			for ( ii = 0; ii < i && errCnt < ERRMSG_SIZE - 18-4-4; ++ii )
			{
				errMsg[errCnt++] = hex_of[(inpStr[ii]>>4)&0xF];
				errMsg[errCnt++] = hex_of[inpStr[ii]&0xF];
			}
			if ( ii >= ERRMSG_SIZE-18-4-4 )
				errCnt += snprintf(errMsg+errCnt,ERRMSG_SIZE-errCnt,"...'");
			errCnt += snprintf(errMsg + errCnt, ERRMSG_SIZE - errCnt, "' 0x%02X(%c) 0x%02X(%c)",
							   str[0], isprint(str[0])?str[0]:'.',
							   str[1], isprint(str[1])?str[1]:'.'
							   );
			++i;
			if ( errCnt < ERRMSG_SIZE - 4 && nbytes-i > 1)
			{
				str += 2;
				errMsg[errCnt++] = ' ';
				errMsg[errCnt++] = '\'';
				for (ii=i*2; ii < nbytes*2 && errCnt < ERRMSG_SIZE-2; ii += 2, str += 2)
				{
					errMsg[errCnt++] = isprint(str[0]) ? str[0]:'.';
					errMsg[errCnt++] = isprint(str[1]) ? str[1]:'.';
				}
				errMsg[errCnt++] = '\'';
				errMsg[errCnt] = 0;
			}
			moan(errMsg);
			return 1;
		}
		*bytestr++ = (msb<<4)|lsb;
		str += 2;
	}
	return (0);
} /* end strtobytes */


/*=========================================================================
 * Convert ASCII hex string to binary hex nybbles (only 4 bits worth per byte).
 * Make sure all the chars are hexidecimal.
 *========================================================================*/
int strtohex(uchar *inpStr, int nchars)
{
	uchar *str=inpStr;
	int i;
	char errMsg[ERRMSG_SIZE];
	int errCnt = 0;

	for ( i = 0; i < nchars; ++i )
	{
		char bits;
		bits = chartohex[str[i]];
		if ( bits == XX )
		{
			int ii;
			errCnt = snprintf(errMsg, ERRMSG_SIZE, "strtohex(): Not hex digit at str[%d] of len %d '", i, nchars);
			for ( ii = 0; ii < i && errCnt < ERRMSG_SIZE - 18-4-4; ++ii )
			{
				errMsg[errCnt++] = hex_of[inpStr[ii]&0xF];
			}
			if ( ii >= ERRMSG_SIZE-18-4-4 )
				errCnt += snprintf(errMsg+errCnt,ERRMSG_SIZE-errCnt,"...'");
			errCnt += snprintf(errMsg + errCnt, ERRMSG_SIZE - errCnt, "' 0x%02X(%c)",
							   str[i], isprint(str[i])?str[i]:'.');
			++i;
			if ( errCnt < ERRMSG_SIZE - 4 )
			{
				errMsg[errCnt++] = ' ';
				errMsg[errCnt++] = '\'';
				for (ii=i; ii < nchars && errCnt < ERRMSG_SIZE-2; ++ii)
				{
					errMsg[errCnt++] = isprint(str[ii]) ? str[ii]:'.';
				}
				errMsg[errCnt++] = '\'';
				errMsg[errCnt] = 0;
			}
			moan(errMsg);
			return 1;
		}
		str[i] = bits;
	}
	return (0);
} /* end strtohex */



/*==========================================================================
 * Convert binary hex nybble string to bytes.
 *==========================================================================*/
void hextobytes(uchar *hexstr, int nbytes)
{
	uchar *bytestr = hexstr;
	int i;

	for (i = 0; i < nbytes; ++i) 
		{
		*bytestr++ = HEXTOBYTE(hexstr);
		hexstr += 2;
		}
} /* end hextobytes */


/*==========================================================================
 * Assemble big-endian byte stream into multi-byte logical addr.
 *==========================================================================*/
LogicalAddr bytestoaddr(uchar *bytestr, int nbytes)
{
	LogicalAddr val = 0L;
	int i;

	if (nbytes > 0) 
		{
		if (nbytes > 4) 
			{
			bytestr += nbytes - 4;
			nbytes = 4;
			}
		for (i = 0; i < nbytes; ++i) 
			val = (val << 8) + bytestr[i];
		}
	return (val);
} /* end bytestoaddr */


/*==========================================================================
 * Assemble big-endian hex nybble stream into multi-byte logical addr.
 *==========================================================================*/
LogicalAddr hextoaddr(uchar *hexstr, int nnybs)
{
	LogicalAddr val = 0L;
	int i;

	if (nnybs > 0) 
		{
		if (nnybs > 8) 
			{
			hexstr += nnybs - 8;
			nnybs = 8;
			}
		for (i = 0; i < nnybs; ++i) 
			val = (val * 16) + hexstr[i];
		}
	return (val);
} /* end hextoaddr */
