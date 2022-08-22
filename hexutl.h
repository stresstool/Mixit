#if 0
===========================================================================

H E X U T L . H


	Copyright 1989 Atari Games.  All rights reserved.
	Author: Lyle Rains

	---------------------------------------------------------------------------
	Revision history:

	---------------------------------------------------------------------------
	Known bugs/features/limitations:

	===========================================================================
#endif

#ifndef HEXUTL_H
#define HEXUTL_H

#include "mixit.h"

#define HEXTOBYTE(hex)   ((hex)[1] + ((hex)[0] << 4))

#define XX 0x7F

extern uchar chartohex[256];
extern uchar hex_of[16];

int			strtobytes(uchar *str, int nbytes);
int			strtohex(uchar *str, int nchars);
void		hextobytes(uchar *hexstr, int nbytes);
LogicalAddr bytestoaddr(uchar *bytestr, int nbytes);
LogicalAddr hextoaddr(uchar *hexstr, int nnybbles);

#endif /* HEXUTL_H */
