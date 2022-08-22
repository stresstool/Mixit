#if 0
/*===========================================================================

V L D A . C

	VLDA - variable length binary records. 1st byte after count
	identifes the record type. A value of 0 means the data is
	binary load data. A value of 13 (decimal) means data is
	transparent. That is, the text of the record (bytes 1-n)
	are unspecified and are to be passed through unchanged.
	This is typically used for symbol data records. All other
	record types are ignored (these are object file format
	records which should be irrelavent. For type 0 record,
	bytes 1-4 are the target memory address and bytes 5-n
	are the data. I.e.:

		Bytes in file increasing this way --->
		(c0 c1)00 a0 a1 a2 a3 dd ... dd
		|    |  |  |  |  |  |  |______|___ (count - 5) bytes of data
		|    |  |  |__|__|__|_____________ Address, least significant byte first
		|    |  |_________________________ Record type 0
		|____|____________________________ count of bytes in record (not on VAX)


	Copyright 1989 Atari Games.  All rights reserved.
	Author: Lyle Rains

---------------------------------------------------------------------------
	Revision history:

---------------------------------------------------------------------------
	Known bugs/features/limitations:

===========================================================================*/
#endif

#include "mixit.h"

extern uchar *out_buf;
extern int   out_bufsize;

static int	readVarLen(InRecord *record);

#ifdef VMS
/*==========================================================================
 * System dependent routine (VAX RMS varlen records) 
 *==========================================================================*/
static int readVarLen(InRecord *record)
{
	int len;
	int cnt;
	int ispadded;

	record->recType = REC_UNKNOWN;
	record->recData = record->recBuf;

	/* Read 2 bytes of record length. */
	if ( (cnt = getc(record->recFile)) == EOF
		 || (len = getc(record->recFile)) == EOF )
	{
		record->recType = feof(record->recFile) ? REC_EOF : REC_ERR;
		return (0);
	}
	len = record->recLen = cnt | (len << 8);
	cnt = 0;
	ispadded = len & 1;
	if ( !len )
		return 0;
	if ( len > record->recBufLen )
		err_exit("Record too big (%d > %d)", len, record->recBufLen);
	/*
	 *  Read the record data.  A loop is used because certain I/O methods
	 *  with some libraries can return partial length requests.  The new
	 *  ANSI C standard seems to say that this is unacceptable behavior for
	 *  conforming implementations, so this may be an unnecessary 
	 *  precaution?
	 */
	while ( (cnt += fread(&(record->recBuf[cnt]), 1, len - cnt, record->recFile)) != len )
	{
		/*
		 *  Error reading record?  An EOF before the end of the record 
		 *  indicates at error at this point, because the whole 
		 *  record was not read.
		 */
		if ( ferror(record->recFile) )
		{
			moan("Read error");
			perror(__FILE__);
		}
		else if ( feof(record->recFile) )
			moan("Unexpected EOF");

		else
			continue;      /* loop until record finished or error */

		/* Stuff common to both ferror() and feof() errors. */
		record->recType = REC_ERR;
		return (0);
	}
	if ( ispadded )
		getc(record->recFile);    /* read garbage padding char */
	return (1);
} /* end readVarLen */

#else /* !VMS */

/*==========================================================================
 * System dependent routine (VAX RMS varlen records) 
 *==========================================================================*/
static int readVarLen(InRecord *record)
{
	int len;
	int cnt;
	int ispadded;

	record->recType = REC_UNKNOWN;
	record->recData = record->recBuf;

	/* Read 2 bytes of record length. */
	if ( (cnt = getc(record->recFile)) == EOF
		 || (len = getc(record->recFile)) == EOF )
	{
		record->recType = feof(record->recFile) ? REC_EOF : REC_ERR;
		return (0);
	}
	len = record->recLen = cnt | (len << 8);
	cnt = 0;
	ispadded = len & 1;
	if ( ((int)len) > (int)record->recBufLen )
		err_exit("Record too big (%d > %d)", len, record->recBufLen);
	/*
	 *  Read the record data.  A loop is used because certain I/O methods
	 *  with some libraries can return partial length requests.  The new
	 *  ANSI C standard seems to say that this is unacceptable behavior for
	 *  conforming implementations, so this may be an unnecessary 
	 *  precaution?
	 */
	while ( (cnt += fread(&(record->recBuf[cnt]), 1, len - cnt, record->recFile)) != len )
	{
		/*
		 *  Error reading record?  An EOF before the end of the record 
		 *  indicates at error at this point, because the whole 
		 *  record was not read.
		 */
		if ( ferror(record->recFile) )
		{
			moan("Read error");
			perror(__FILE__);
		}
		else if ( feof(record->recFile) )
			moan("Unexpected EOF");

		else
			continue;      /* loop until record finished or error */

		/* Stuff common to both ferror() and feof() errors. */
		record->recType = REC_ERR;
		return (0);
	}

	if ( ispadded )
		(void)getc(record->recFile);    /* read garbage padding char */
	return (1);
} /* end readVarLen */
#endif /* !VMS */

/*===========================================================================*/
int GetRec_vlda(InRecord *record)
{
	LogicalAddr addr;
	int addroffs;

	if ( !readVarLen(record) )
		record->recLen = 0;
	else
		switch (record->recBuf[0])
		{
		case 0:     /* DATA RECORD  */
			addroffs = 5 /* size + record type byte */;
			record->recData = &(record->recBuf[addroffs]);
			record->recLen -= addroffs;
			addr = 0;
			while ( --addroffs > 0 )
			{
				addr <<= 8;
				addr += record->recBuf[addroffs];
			}
			record->recSAddr = addr;
			record->recEAddr = addr + record->recLen - 1;
			record->recType = REC_DATA;
			break;
		case 13:    /* TRANSPARENT RECORD. */
			++record->recData;
			record->recData[--(record->recLen)] = '\0';
			record->recType = REC_TRANSPARENT;
			break;
		default:    /* Unknown record type -- ignored. */
			record->recType = REC_UNKNOWN;
			break;
		}
	return (record->recType);
} /* end GetRec_vlda */

/*==========================================================================*/
int PutSym_vlda(FILE *file, uchar *data, int recsize)
{
	register uchar *bp = out_buf;

	/*
	 *  Make sure the output buffer is big enough.
	 */

	if ( out_bufsize < recsize + 3 )
	{
		free(out_buf);
		if ( !(bp = out_buf = (uchar *)malloc(out_bufsize = recsize + 3)) )
			return err_return(0, "Can't allocate %d bytes for symbol record",
							  out_bufsize);
	}

	PUT_BUF(bp, byte_of(recsize));
	PUT_BUF(bp, byte_of(recsize >> 8));
	PUT_BUF(bp, '\r');

	memcpy(bp, data, recsize);
	bp += recsize;

	if ( fwrite(out_buf, bp - out_buf, 1, file) != 1 )
		return perr_return(0, "Error writing symbol record");

	return 1;

} /* end PutSym_vlda */


/*==========================================================================*
 * Outputs a single record in VLDA format.
 *==========================================================================*/
int PutRec_vlda(FILE *file, uchar *data, int recsize, ulong recstart)
{
	register uchar  *bp = out_buf;
	uint            recbytes = recsize;

#ifndef VMS
	recbytes += 5;
#endif

	/*
	 *  Make sure the output buffer is big enough.
	 */

	if ( out_bufsize < (int)recbytes )
	{
		free(out_buf);
		if ( !(bp = out_buf = (uchar *)malloc(out_bufsize = recbytes)) )
			return err_return(0, "Can't allocate %d bytes for data record",
							  out_bufsize);
	}

#ifndef VMS
	PUT_BUF(bp, byte_of(recbytes));
	PUT_BUF(bp, byte_of(recbytes >> 8));
#endif

	PUT_BUF(bp, 0);           /* Record type 0 */
	PUT_BUF(bp, byte_of(recstart));   /* Address bytes */
	PUT_BUF(bp, byte_of(recstart >>  8));
	PUT_BUF(bp, byte_of(recstart >> 16));
	PUT_BUF(bp, byte_of(recstart >> 24));

	memcpy(bp, data, recsize);
	bp += recsize;
	if ( recbytes & 1 )
		PUT_BUF(bp, 0);

	/* Write the data */
	if ( fwrite(out_buf, bp - out_buf, 1, file) != 1 )
		return perr_return(0, "Error writing data record");

	return 1;

} /* end PutRec_vlda */
