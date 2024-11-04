#if 0
/*===========================================================================

	L D A . C


	LDA - variable length binary stream data. Each record is
	identified by a leading short of 0001 or 0002 least
	significant byte first. The next 2 bytes contain the count
	lsb first. The next 2 bytes (or 4 bytes for type 0002
	record) contain the target memory load address. There are
	then n bytes of data followed by a 1 byte checksum. The
	count includes the sentinel, the count bytes, the address
	and all the data. It does not include the checksum byte.
	The checksum is the twos complement of the sum of all the
	bytes in the record (including the sentinel, count and
	address) except itself. (NOTE: There is no "system"
	identified record structure. On the VAX, records are read
	as fixed length 512 and broken down accordingly. On non-VMS
	systems, files are simply stream data). I.e.: 

	Bytes in file increasing this way --->

	01 00 c0 c1 a0 a1 dd ... dd cs
	|  |  |  |  |  |  |      |  |__ cksum = -(1+c0+c1+a0+a1+dd...)
	|  |  |  |  |  |  |______|_____ (count - 6) data bytes
	|  |  |  |  |__|_______________ target memory load address
	|  |  |__|_____________________ count of bytes in record
	|__|___________________________ sentinel (type 1 -> 16 bit address)

	02 00 c0 c1 a0 a1 a2 a3 dd ... dd cs
	|  |  |  |  |        |  |      |  |__ cksum = -(2+c0+c1+a0+a1+a2+a3+dd...)
	|  |  |  |  |        |  |______|_____ (count - 8) data bytes
	|  |  |  |  |________|_______________ target memory load address
	|  |  |__|___________________________ count of bytes in record
	|__|_________________________________ sentinel (type 2 -> 32 bit address)


	Copyright 1989 Atari Games.  All rights reserved.
	Author: Lyle Rains

---------------------------------------------------------------------------
	Revision history:

---------------------------------------------------------------------------
	Known bugs/features/limitations:

===========================================================================*/
#endif

#include "mixit.h"
#include <errno.h>

static int	readRec(InRecord *record);
static void	send_a_byte(int b, FILE *file, uint *cksum);

static int buffered_bytes;

/*==========================================================================*/
static int readRec(InRecord *record)
{
	int len;
	int cnt;
	int addrsize = 0;
	int chksum;
	int byte0, byte1, skip;
	GPF *gpf = record->gpfPtr;
	
#define HDR_SIZE 4  /* sentinel + count bytes */

	record->recType = REC_UNKNOWN;
	record->recData = record->recBuf;

	/* Loop until we get record sentinel. */
	skip = 0;
	for (;;)
	{
		/* The structure of lda files is the first two bytes of a record are either 1,0 or 2,0 */
		if ( !skip )
		{
			/* Get a byte if don't already have one */
			if ( (byte0 = getc(record->recFile)) == EOF && feof(record->recFile) )
			{
				record->recType = REC_EOF;
				return (0);
			}
		}
		skip = 0;
		/* check the byte is either a 1 or a 2 */
		if ( byte0 != 1 && byte0 != 2 )
		{
			/* Nope, keep looking */
			if ( byte0 && !gpf->reportedSkippedBytes )
			{
				/* Only squawk about non-zero bytes while searching for sentinel */
				warn("While looking for a 0x01 0x00 or 0x02 0x00 sentinel, found a 0x%02X, 0x??. Might be a corrupt input file first at offset %ld (0x%X).",
					 byte0, gpf->recordOffset, gpf->recordOffset);
				gpf->reportedSkippedBytes = 1;
			}
			++gpf->recordOffset;
			continue;
		}
		/* Found one. Now get second byte */
		byte1 = getc(record->recFile);
		if ( byte1 != 0 )
		{
			/* It is not zero, see if it's maybe an EOF */
			if ( feof(record->recFile) )
			{
				record->recType = REC_EOF;
				return (0);
			}
			/* Not EOF so ignore it (pretend it is byte 0 for the next loop)*/
			if ( !gpf->reportedSkippedBytes )
			{
				/* But squawk about it */
				warn("While looking for a 0x01 0x00 or 0x02 0x00 sentinel, found a 0x%02X, 0x%02X. Might be a corrupt input file first at offset %ld (0x%X).",
					 byte0, byte1, gpf->recordOffset, gpf->recordOffset );
				gpf->reportedSkippedBytes = 1;
			}
			skip = 1;
			byte0 = byte1;
			++gpf->recordOffset;
			continue;
		}
		/* Now we found the 1,0 or 2,0. */
		addrsize = 2*byte0;		/* addrsize becomes either a 2 or 4 */
		/* we're done looking */
		break;
	}
	chksum = byte0;

	/* Read 2 bytes of record length. */
	if ( (byte0 = getc(record->recFile)) == EOF ||
		 (byte1 = getc(record->recFile)) == EOF )
	{
		record->recType = feof(record->recFile) ? REC_EOF : REC_ERR;
		return (0);
	}
	chksum += byte0;
	chksum += byte1;
	record->recLen = (byte0 | (byte1 << 8));
	if ( record->recLen < HDR_SIZE+addrsize )
	{
		moan("Record too short (%d<%d). Probably corrupted input starting at offset: %ld (0x%lX)", record->recLen, HDR_SIZE+addrsize, gpf->recordOffset, gpf->recordOffset);
		record->recType = REC_ERR;
		return (0);
	}
	record->recLen -= HDR_SIZE;
	len = record->recLen + 1 /* include chksum byte in data read from disk */;
	if ( (size_t)len > record->recBufLen )
	{
		moan("Record too long (%d>%d). Probably corrupted input starting at offset: %ld (0x%lX)", len&0xFFFF, record->recBufLen, gpf->recordOffset, gpf->recordOffset);
		record->recType = REC_ERR;
		return (0);
	}
	cnt = 0;
	/*
	 *  Read the record data.  A loop is used because certain I/O methods
	 *  with some libraries can return partial length requests.  The new
	 *  ANSI C standard seems to say that this is unacceptable behavior for
	 *  conforming implementations, so this may be an unnecessary precaution?
	 */
	while ( (cnt += fread(&(record->recBuf[cnt]), 1, len - cnt, record->recFile)) != len )
	{
		/*
		 *  Error reading record?  An EOF before the end of the 
		 *  record indicates an error at this point, because the 
		 *  whole record was not read.
		 */
		if ( ferror(record->recFile) )
		{
			moan("Input file read error at record offset %ld (0x%lX): %s", gpf->recordOffset, gpf->recordOffset, strerror(errno));
			record->recType = REC_ERR;
			return (0);
		}
		if ( feof(record->recFile) )
		{
			moan("Unexpected EOF at input offset %ld (0x%lX). Probably corrupt input file.", gpf->recordOffset, gpf->recordOffset);
			record->recType = REC_ERR;
			return (0);
		}
	}
	/* Test checksum. */
	for ( cnt = 0; cnt < len; ++cnt )
		chksum += record->recBuf[cnt];
	if ( byte_of(chksum) )
	{
		moan("Bad checksum in record at offset %ld (0x%lX). Computed 0x%02X, expected 0x00. Probably corrupt input file.", gpf->recordOffset, gpf->recordOffset, byte_of(chksum));
		record->recType = REC_ERR;
		return (0);
	}
	gpf->recordOffset += HDR_SIZE + len;	/* Advance record offset to next one */
	return (addrsize);
} /* end ReadRec */


/*==========================================================================*/
int GetRec_lda(InRecord *record)
{
	LogicalAddr addr;
	int     addrsize;

	addrsize = readRec(record);
	if ( addrsize == 0 )
		record->recLen = 0;
	else
	{   /* DATA RECORD */
		record->recData = &(record->recBuf[addrsize]);
		record->recLen -= addrsize;
		addr = 0;
		while ( --addrsize >= 0 )
		{
			addr <<= 8;
			addr += record->recBuf[addrsize];
		}
		record->recSAddr = addr;
		record->recEAddr = addr+record->recLen-1;
		record->recType = REC_DATA;
	}
	return (record->recType);
} /* end GetRec_lda */


/*==========================================================================
 * Send a byte and keep track of a checksum value.
 *==========================================================================*/
static void send_a_byte(int b, FILE *file, uint *cksum)
{
	*cksum += (b &= 0xFF);
	*cksum &= 0xFF;
	fputc(b, file);

} /* end send_a_byte */


/*==========================================================================*
 * Outputs a single record in LDA format.
 *==========================================================================*/
int PutRec_lda(FILE *file, uchar *data, int recsize, ulong recstart)
{
	uint    cksum;
	int     j;
	uint    recbytes = recsize + 6;
	uchar   rtype = 1;

	cksum    = 0;

	if ( recstart > 0xFFFF )        /* 32-bit addressing */
	{
		rtype     = 2;
		recbytes += 2;
	}

	/* Output record type field 		 */
	send_a_byte(rtype, file, &cksum);
	send_a_byte(0, file, &cksum);
	/* Now the count field 				 */
	send_a_byte(recbytes,      file, &cksum);
	send_a_byte(recbytes >> 8, file, &cksum);
	/* Address field (32 bits for type 2)*/
	send_a_byte(recstart, file, &cksum);
	send_a_byte(recstart >>  8, file, &cksum);

	if ( rtype == 2 )                     /* 32-bit address 					 */
	{
		send_a_byte(recstart >> 16, file, &cksum);
		send_a_byte(recstart >> 24, file, &cksum);
	}

	for ( j = 0; j < recsize; ++j )         /* Add the data 					 */
		send_a_byte(*data++, file, &cksum);

	/* Finish with the checksum 		 */

	send_a_byte(-(j = cksum), file, (uint *)&j);
	/* Keep track of buffered data for flushing at end*/
	buffered_bytes += recbytes + 1;

	return 1;

} /* end PutRec_lda */


/*==========================================================================*
 * Outputs all footer information required by a LDA format file.
 *==========================================================================*/
int PutFoot_lda(FILE *file)
{
	int     rv;
	uchar   data[1];

	rv = PutRec_lda(file, data, 0, 0L);

#ifdef VMS
	buffered_bytes %= 512;
	if ( buffered_bytes )
	{
		/* Fill out to the nearest 512-byte record */
		while ( buffered_bytes < 512 )
		{
			fputc(0, file);
			++buffered_bytes;
		}
	}
#endif
	return rv;

} /* end PutFoot_lda */
