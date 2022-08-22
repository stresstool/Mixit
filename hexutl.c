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
int strtobytes(uchar *str, int nbytes)
{
	uchar *bytestr = str;
	int i;

	for (i = 0; i < nbytes; ++i) 
		{
		if ( ((str[0] = chartohex[str[0]]) == XX) ||
			((str[1] = chartohex[str[1]]) == XX) )
			{
			moan( "Not hex digit" );
			return (1);
			}
		*bytestr++ = HEXTOBYTE(str);
		str += 2;
		}
	return (0);
} /* end strtobytes */


/*=========================================================================
 * Convert ASCII hex string to binary hex nybbles (only 4 bits worth per byte).
 * Make sure all the chars are hexidecimal.
 *========================================================================*/
int strtohex(uchar *str, int nchars)
{
	int i;

	for (i = 0; i < nchars; ++i) 
		if ((str[i] = chartohex[str[i]]) == XX) 
			{
			moan("Not hex digit");
			return (1);
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
