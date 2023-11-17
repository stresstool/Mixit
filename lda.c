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
#define HDR_SIZE 4  /* sentinel + count bytes */

	record->recType = REC_UNKNOWN;
	record->recData = record->recBuf;

	/* Loop until we get record sentinel. */
	for (;;)
	{
		if ( (cnt = getc(record->recFile)) == EOF ||
			 (len = getc(record->recFile)) == EOF )
		{
			record->recType = feof(record->recFile) ? REC_EOF : REC_ERR;
			return (0);
		}
		if ( len == 0 )
			switch (cnt)
			{
			default:
				continue;

			case 1:
				addrsize = 2;
				break;
			case 2:
				addrsize = 4;
				break;
			}
		break;
	}
	chksum = cnt;

	/* Read 2 bytes of record length. */
	if ( (cnt = getc(record->recFile)) == EOF ||
		 (len = getc(record->recFile)) == EOF )
	{
		record->recType = feof(record->recFile) ? REC_EOF : REC_ERR;
		return (0);
	}
	chksum += cnt;
	chksum += len;
	record->recLen = (cnt | (len << 8)) - HDR_SIZE;
	len = record->recLen + 1 /* chksum byte */;
	cnt = 0;
	if ( (size_t)len > record->recBufLen )
		err_exit(__FILE__, __LINE__, "too long (%d>%d)", len, record->recBufLen);
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
			moan("Read error");
			perror(__FILE__);
		}
		else if ( feof(record->recFile) )
			moan("Unexpected EOF");
		else
			continue;                        /* loop until record finished or error */

		/* Stuff common to both ferror() and feof() errors. */
		record->recType = REC_ERR;
		return (0);
	}

	/* Test checksum. */
	for ( cnt = 0; cnt < len; ++cnt )
		chksum += record->recBuf[cnt];
	if ( byte_of(chksum) )
	{
		moan("Bad checksum");
		record->recType = REC_ERR;
		return (0);
	}
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
	if ( (buffered_bytes += recbytes + 1) >= 512 )
		buffered_bytes %= 512;

	return 1;

} /* end PutRec_lda */


/*==========================================================================*
 * Outputs all footer information required by a LDA format file.
 *==========================================================================*/
int PutFoot_lda(FILE *file)
{
	uchar   data[512];
	int     rv;

	buffered_bytes = 0;

	rv = PutRec_lda(file, data, 0, 0L);

	/* Fill out to the nearest 512-byte record */
	while ( buffered_bytes++ < 512 )
		fputc(0, file);

	return rv;

} /* end PutFoot_lda */
