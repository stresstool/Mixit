#if 0
/*===========================================================================

	I M G . C


	IMAGE - Raw data. The first byte in the file is assumed to
	occupy target system memory address 0. Each additional byte
	occupies the next sequential target memory address. On the
	VAX, the minimum file size is 512 (since image files are
	stored as fixed length records), however, this is too
	restrictive. I want the system to be able to read image
	files of any aribitray size.

	Copyright 1989 Atari Games.  All rights reserved.
	Author: Lyle Rains

---------------------------------------------------------------------------
	Revision history:

---------------------------------------------------------------------------
	Known bugs/features/limitations:

===========================================================================*/
#endif

#include "mixit.h"

#ifndef SEEK_SET
#define	SEEK_SET	0	/* Set file pointer to "offset" */
#define	SEEK_CUR	1	/* Set file pointer to current plus "offset" */
#define	SEEK_END	2	/* Set file pointer to EOF plus "offset" */
#endif

/*==========================================================================*/
int GetRec_img(InRecord *rec)
{
	size_t len;

	rec->recData = rec->recBuf;
	if ((len = fread(rec->recBuf, 1, rec->recBufLen, rec->recFile)) > 0) 
		{
		rec->recSAddr += rec->recLen;
		rec->recLen = len;
		rec->recEAddr = rec->recSAddr+len-1;
		rec->recType = REC_DATA;
		}
	else 
		{
		rec->recType = feof(rec->recFile) ? REC_EOF : REC_ERR;
		rec->recLen = 0;
		}
	return (rec->recType);

} /* end GetRec_img */


/*==========================================================================*
 * Outputs a single record in IMG format.
 *==========================================================================*/
int PutRec_img( FILE *file, uchar *data, int recsize, ulong recstart )
{
#if defined(VMS)
	uchar   buffer[512];
	int     amount;
	while ( recsize > 0 )
		{
		amount = min( recsize, 512 );
		memcpy( buffer, data, amount );

		if (amount < 512)
			memset( &buffer[amount], 0, 512-amount );

		if ( fwrite( buffer, 512, 1, file ) != 1)
			return perr_return( 0, "Error writing data" );

		recsize -= amount;
		data += amount;
		}
#else
	if (fseek(file, recstart, SEEK_SET) < 0) 
	    return perr_return( 0, "Error seeking to position in IMAGE file");
	if (fwrite( data, recsize, 1, file) != 1) 
	    return perr_return( 0, "Error writing data");
#endif

	return 1;
} /* end PutRec_img */
