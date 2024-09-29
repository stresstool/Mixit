#if 0
/* ===========================================================================

	D I O . C


	Data I/O - Raw data.  There are three formats of DIO data.

	1)  An initial RUBOUT (FF) character begins the data, followed by 
	sequential bytes of data starting from address 0.  The data ends at EOF.

	2)  16-bit mode, where the format is:

	08,1C,2A,49,08,00,cccc,FF,data...,00,00,ssss

	where cccc is the 2-byte count of data,
	and   ssss is the 2-byte checksum.

	3)  32-bit mode, where the format is:

	08,1C,3E,6B,08,00,0c,0c,0c,0c,0c,0c,0c,0c,FF,data...,00,00,ss,ss

	where cccccccc is the 32 bit count of data split into the low 4 bit nibbles of 8 bytes, big endian (total of a 15 byte header)
	and       ss,ss is the 2-byte checksum, big endian


	On all systems the DIO files begin with a CR (0x0D) followed by the 15-byte header,
	etc.

	Copyright 1993 Atari Games.  All rights reserved.
	Author: Jim Petrick

---------------------------------------------------------------------------
	Revision history:

2-oct-1993	DMS	Would produce incorrect file if PutRec was called
			with recsize not a multiple of 512. Also it didn't
			pad the file to match the length specified in the
			header. Made it a little faster.

25-sep-2024 TG Fixed checksum handling on both input and output.

25-sep-2024	DMS Fixed the comments above.
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

extern uchar	*out_buf;
extern int	out_bufsize;

static uchar	buffer[512];
static ushort	bufIndex;
static ulong	checkSum;
static ulong	amtWritten, amtToWrite;
#if 0
static uchar	zeroes[512];
extern GPF	*gpf;
#endif

/*==========================================================================*/
int GetRec_dio(InRecord *rec)
{
	uchar *bp;
	static ushort	file_cksum = 0;
	static ushort	checksum = 0;
	static long		byteCount = 0;
	size_t			len, lclRecLen, dataSize;
	int				i;
	uchar			byte, arrow[5],
		a16[] = { 0x1C, 0x2A, 0x49, 0x08, 0x00 },
		a32[] = { 0x1C, 0x3E, 0x6B, 0x08, 0x00 },
		count[9];

	/* Skip header info until a rubout is reached */
	if ( !rec->recPrivate )
	{
		checksum  = 0;
		if ( fread(&byte, 1, 1, rec->recFile) == 0 )
			goto problem;

		/* Short form of DIO format */
		if ( byte == 0xFF )
			byteCount = 0x7FFFFFFFL;

		/* Check for CR, skip if found */
		else if ( byte == 0x0D )
			if ( fread(&byte, 1, 1, rec->recFile) == 0 )
				goto problem;

		/* Check for arrow start */
		if ( byte == 0x08 )
		{
			/* Got Arrow, but which type */
			if ( fread(&arrow, 1, 5, rec->recFile) == 0 )
				goto problem;

			if ( !strncmp((char *)arrow, (char *)a16, 5) )
				len = 4;
			else if ( !strncmp((char *)arrow, (char *)a32, 5) )
				len = 8;
			else
				goto problem;

			/* Read the count bytes */
			if ( fread(&count, 1, len + 1, rec->recFile) == 0 )
				goto problem;

			/* Save the count of bytes to read in */
			for ( dataSize = i = 0; i < 8 /*(int)len*/; ++i )
				dataSize = (dataSize << 4) + (count[i] & 0xF);

			/* Check for the RUBOUT byte */
			if ( count[i] != 0xFF )
				goto problem;

			byteCount = dataSize;
			rec->recPrivate = (void *)1;
		}
	}

	/* End of data reached, check out the checksum */
	if ( byteCount <= 0 )
	{
		if ( fread(count, 1, 4, rec->recFile) == 0 || count[0] || count[1] )
			goto problem;

		/* file_cksum now contains checksum */
		file_cksum = (count[2] << 8) | count[3];
		if ( file_cksum != checksum )
		{
			printf("Checksum Error - File Checksum = %X   Calculated Checksum = %X\n", file_cksum, checksum);
			rec->recLen  = 0;
			checksum = 0;
			file_cksum = 0;
			byteCount = 0;
			return (rec->recType = REC_ERR);
		}

		rec->recLen  = 0;
		return (rec->recType = REC_EOF);
	}

	rec->recData = rec->recBuf;
	lclRecLen = rec->recBufLen;
	if ( lclRecLen > byteCount )
		lclRecLen = byteCount;		/* Don't read more 'data' than is in the file (i.e. don't include the 4 checksum bytes here) */
	if ( (len = fread(rec->recBuf, 1, lclRecLen, rec->recFile)) > 0 )
	{
		rec->recSAddr += rec->recLen;	/* Advance address from last read size */
		rec->recLen  = len;				/* remember current read size for next time */
		rec->recEAddr = rec->recSAddr+rec->recLen-1; /* new last address */

		/* Compute the checksum */
		for ( bp = rec->recBuf, i = 0; i < len; ++i )
			checksum += *bp++;

		if ( byteCount != 0x7FFFFFFFL )
			byteCount -= len;

#if 0	/* Should never get here now */
		if ( byteCount < 0 )
		{
			for ( i = 0; i < 4; ++i )
				checksum -= *--bp;

			file_cksum = (*(bp+2) << 8) | *(bp+3);
			if ( file_cksum != checksum )
			{
				printf("Checksum Error - File Checksum = %X   Calculated Checksum = %X\n", file_cksum, checksum);
				rec->recLen  = 0;
				checksum = 0;
				file_cksum = 0;
				byteCount = 0;
				return (rec->recType = REC_ERR);
			}
		}
#endif

		return (rec->recType = REC_DATA);
	}

problem:
	byteCount = 0;
	checksum = 0;
	rec->recLen = 0;
	return (rec->recType = feof(rec->recFile) ? REC_EOF : REC_ERR);

} /* end GetRec_dio */


/*==========================================================================*
 * Outputs a single record in DIO format.
 *==========================================================================*/
int PutRec_dio(FILE *file, uchar *data, int recsize, ulong recstart)
{
	int	i;
	uchar *bp;

#if defined(VMS)
	int     amount;

	amtWritten += recsize;      /* record for posterity */
	for (; recsize > 0; recsize -= amount, data += amount )
	{
		if ( bufIndex > 0 )
		{
			amount = min(sizeof(buffer) - bufIndex, recsize);
			memcpy(buffer, data, amount);
			bufIndex += amount;
			if ( bufIndex == sizeof(buffer) )
			{
				/* Compute the checksum */
				for ( bp = buffer, i = 0; i < recsize; ++i )
					checkSum += *bp++;
				if ( fwrite(buffer, sizeof(buffer), 1, file) != 1 )
					return perr_return(0, "Error writing data");
				bufIndex = 0;
			}
			continue;
		}
		if ( recsize < sizeof(buffer) )
		{
			amount = recsize;
			memcpy(buffer, data, amount);
			memset(buffer + amount, 0, sizeof(buffer) - amount);
		}
		else
		{
			amount = recsize - (recsize % sizeof(buffer));

			/* Compute the checksum */
			for ( bp = data, i = 0; i < amount; ++i )
				checkSum += *bp++;

			if ( fwrite(data, sizeof(buffer), amount / sizeof(buffer), file) != amount / sizeof(buffer) )
				return perr_return(0, "Error writing data");
		}
	}
#else
	if ( amtWritten != recstart )
	{
		if ( fseek(file, recstart + 0x10, SEEK_SET) < 0 )
			return perr_return(0, "Error seeking to position in DIO file");
	}
	if ( amtWritten <= recstart )
	{
		amtWritten += recstart - amtWritten + recsize;
	}
	else
	{
		char emsg[132];
		sprintf(emsg, "ERROR: Not allowed to backpatch a .DIO file (%08lX-%08lX)\n",
				recstart, recstart + recsize - 1);
		return perr_return(0, emsg);
	}
	for ( bp = data, i = 0; i < recsize; ++i )
		checkSum += *bp++;
	if ( fwrite(data, recsize, 1, file) != 1 )
		return perr_return(0, "Error writing data");
#endif
	return 1;
} /* end PutRec_dio */


/*==========================================================================
 * Puts out any header info for the file.
 *==========================================================================*/
int PutHead_dio(FILE *file, ulong addr, ulong hi)
{

	amtToWrite = hi - addr + 1;
	amtWritten = 0;     /* say how much has been written */
	bufIndex = 0;       /* overflow buffer index */
	checkSum = 0;       /* Initialize the checksum */

	memset(buffer, 0, 16);
	if ( fwrite(buffer, 16, 1, file) != 1 )
		return perr_return(0, "Error writing header");

	return 1;

} /* end PutHead_dio */


/*==========================================================================
 * Puts out any footer info for the file.
 *==========================================================================*/
int PutFoot_dio(FILE *file)
{
	uchar csbuf[4], *bp;
	int sts;

#if defined(VMS)
	int amount;
	if ( bufIndex )
	{
		memset(buffer + bufIndex, 0, sizeof(buffer) - bufIndex);
		amount = min(amtToWrite - amtWritten, sizeof(buffer));
		if ( fwrite(buffer, amount, 1, file) != 1 )
			return perr_return(0, "Error flushing before writing footer");
		bufIndex = 0;
		amtWritten += amount;
	}
#endif
	bp = csbuf;
	*bp++ = 0;
	*bp++ = 0;
	*bp++ = (checkSum >> 8) & 0xFF;
	*bp++ = checkSum & 0xFF;
	if ( noisy )
		fprintf(stdout, "DIO Checksum = (%04X)%04X\n",
				(int)((checkSum >> 16) & 0xFFFF), (int)(checkSum & 0xFFFF));
	if ( fwrite(csbuf, sizeof(csbuf), 1, file) != 1 )
		return perr_return(0, "Error writing footer");
	fflush(file);
	if ( fseek(file, 0, SEEK_SET) < 0 )
		return perr_return(0, "Error seeking to 0 to re-read header");
	if ( (sts = fread(buffer, 16, 1, file)) != 1 )
	{
		char msg[132];
		sprintf(msg, "Error %d re-reading header", sts);
		return perr_return(0, msg);
	}
	bp = buffer;
	*bp++ = 0x0D;
	*bp++ = 0x08;
	*bp++ = 0x1C;
	*bp++ = 0x3E;
	*bp++ = 0x6B;
	*bp++ = 0x08;
	*bp++ = 0x00;
	*bp++ = (uchar)((amtWritten >> 28) & 15);
	*bp++ = (uchar)((amtWritten >> 24) & 15);
	*bp++ = (uchar)((amtWritten >> 20) & 15);
	*bp++ = (uchar)((amtWritten >> 16) & 15);
	*bp++ = (uchar)((amtWritten >> 12) & 15);
	*bp++ = (uchar)((amtWritten >>  8) & 15);
	*bp++ = (uchar)((amtWritten >>  4) & 15);
	*bp++ = (uchar)((amtWritten >>  0) & 15);
	*bp++ = 0xFF;
/* fprintf(errFile, "Wrote a %d (%08lX) byte dio file\n", amtWritten, amtWritten); */
	if ( fseek(file, 0, SEEK_SET) < 0 )
		return perr_return(0, "Error seeking to 0 to re-write header");
	if ( fwrite(buffer, 16, 1, file) != 1 )
		return perr_return(0, "Error re-writing dio header");
	return 1;

} /* end PutFoot_dio */
