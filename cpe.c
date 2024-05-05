#if 0
/*===========================================================================

	C P E . C


	CPE - variable length binary stream data as used on the Sony
		playstation. Each record is identified by a leading byte in the
		range 0 to 8 indicating the record type. The types are defined
		as:

			0 = EOF (End of File)
			1 = n bytes of data to be loaded at m address.
				the address (m) is 4 bytes following immediately
				after the type byte, lsb first (little endian);
				the count (n) is 4 bytes following immediately
				after the address, lsb first (little endian);
				the data follows immediately after the count.
			2 = xfer address. 4 bytes, lsb first, follows immediately
				after the type byte.
			3 = set register n to long value v. Register number is 2 bytes,
				lsb first, following immediately after the type byte;
				value (v) is 4 bytes, lsb first, following immediately
				after the register number.
			4 = set register n to word value v. Register number is 2 bytes,
				lsb first, following immediately after the type byte;
				value (v) is 2 bytes, lsb first, following immediately
				after the register number.
			5 = set register n to byte value v. Register number is 2 bytes,
				lsb first, following immediately after the type byte;
				value (v) is 1 byte following immediately after the
				register number.
			6 = set register n to 3-byte value v. Register number is 2 bytes,
				lsb first, following immediately after the type byte;
				value (v) is 3 bytes, lsb first, following immediately
				after the register number.
			7 = workspace address is 4 bytes, lsb first, following immediately
				after the type byte.
			8 = unit number is 1 byte following immediately after the type
				byte.

	Copyright 1996 Atari Games.  All rights reserved.
	Author: David Shepperd

---------------------------------------------------------------------------
	Revision history:

---------------------------------------------------------------------------
	Known bugs/features/limitations:

===========================================================================*/
#endif

#include "mixit.h"

static int readl(unsigned long *ans, FILE *fp)
{
	unsigned char buf[4];
	int sts;

	sts = fread(buf, 1, sizeof(buf), fp);
	if ( sts != sizeof(buf) )
		return sts;
	if ( ans )
		*ans = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
	return 0;
}

static int readw(unsigned long *ans, FILE *fp)
{
	unsigned char buf[2];
	int sts;

	sts = fread(buf, 1, sizeof(buf), fp);
	if ( sts != sizeof(buf) )
		return sts;
	if ( ans )
		*ans = (buf[1] << 8) | buf[0];
	return 0;
}

static int read3(unsigned long *ans, FILE *fp)
{
	unsigned char buf[3];
	int sts;

	sts = fread(buf, 1, sizeof(buf), fp);
	if ( sts != sizeof(buf) )
		return sts;
	if ( ans )
		*ans = (buf[2] << 16) | (buf[1] << 8) | buf[0];
	return 0;
}

static int writel(unsigned long val, FILE *fp)
{
	unsigned char buf[4];
	int sts;

	buf[0] = val;
	buf[1] = val >> 8;
	buf[2] = val >> 16;
	buf[3] = val >> 24;
	sts = fwrite(buf, 1, sizeof(buf), fp);
	if ( sts != sizeof(buf) )
		return sts;
	return 0;
}

static int writew(unsigned long val, FILE *fp)
{
	unsigned char buf[2];
	int sts;

	buf[0] = val;
	buf[1] = val >> 8;
	sts = fwrite(buf, 1, sizeof(buf), fp);
	if ( sts != sizeof(buf) )
		return sts;
	return 0;
}

/*==========================================================================*/
static int GetHead_cpe(FILE *fp)
{
	uchar head[4];
	int sts;

	sts = fread(head, 1, sizeof(head), fp);
	if ( sts == sizeof(head) )
	{
		if ( head[0] == 'C' && head[1] == 'P' && head[2] == 'E' && head[3] == 1 )
			return 0;
	}
	return 1;
}

/*==========================================================================*/
int GetRec_cpe(InRecord *record)
{
	int cnt;

	record->recData = record->recBuf;

	if ( !record->recPrivate )
	{
		if ( GetHead_cpe(record->recFile) )
		{
			fprintf(errFile, "Not a CPE file format\n");
			return record->recType = REC_ERR;
		}
		record->recPrivate = (void *)1;
	}

	if ( (cnt = getc(record->recFile)) == EOF )
	{
	read_error:
		return record->recType = feof(record->recFile) ? REC_EOF : REC_ERR;
	}
	switch (cnt)
	{
	case 0:
		return record->recType = REC_EOF;
	case 1:
		{
			unsigned long addr;
			unsigned long len;
			int sts;
			if ( readl(&addr, record->recFile) )
				goto read_error;
			if ( readl(&len, record->recFile) )
				goto read_error;
			if ( (size_t)len > record->recBufLen )
			{
				record->recData = record->recBuf = realloc(record->recBuf, (size_t)len);
				if ( !record->recBuf )
				{
					fprintf(errFile, "Record too long: %ld > %d. Out of memory.\n", len, (int)record->recBufLen);
					record->recBufLen = 0;
					return record->recType = REC_ERR;
				}
				record->recBufLen = len;
			}
			sts = fread(record->recBuf, 1, len, record->recFile);
			if ( sts != len )
				goto read_error;
			record->recSAddr = addr;
			record->recEAddr = addr+len-1;
			record->recLen = len;
			return record->recType = REC_DATA;
		}
	case 2:
		if ( readl(&record->recSAddr, record->recFile) )
			goto read_error;
		record->recLen = 0;
		return record->recType = REC_XFER;
	case 3:
		{
			ulong reg=0;
			if ( readw(&reg, record->recFile) )
				goto read_error;
			if ( readl(&record->recSAddr, record->recFile) )
				goto read_error;
			if ( reg == 144 )
			{
				record->recLen = 0;
				return record->recType = REC_XFER;
			}
			break;
		}
	case 4:
		if ( readw(0, record->recFile) )
			goto read_error;
		if ( readw(0, record->recFile) )
			goto read_error;
		break;
	case 5:
		if ( readw(0, record->recFile) )
			goto read_error;
		if ( fgetc(record->recFile) < 0 )
			goto read_error;
		break;
	case 6:
		if ( readw(0, record->recFile) )
			goto read_error;
		if ( read3(0, record->recFile) )
			goto read_error;
		break;
	case 7:
		if ( readl(0, record->recFile) )
			goto read_error;
		break;
	case 8:
		if ( fgetc(record->recFile) < 0 )
			goto read_error;
		break;
	default:
		err_exit(__FILE__, __LINE__, "Unknown record type: %02X(%d)", cnt & 0xFF, cnt & 0xFF);
	}
	return record->recType = REC_UNKNOWN;
} /* end GetRec_cpe */

/*==========================================================================*
 * Outputs a single record in CPE format.
 *==========================================================================*/
int PutRec_cpe(FILE *file, uchar *data, int recsize, ulong recstart)
{
	uchar	maddr[4];

	maddr[0] = recstart;
	maddr[1] = recstart >> 8;
	maddr[2] = recstart >> 16;
	maddr[3] = recstart >> 24;

	fputc(1, file);             /* type 1 record */
	fwrite(maddr, 1, sizeof(maddr), file);  /* followed with address */
	maddr[0] = recsize;
	maddr[1] = recsize >> 8;
	maddr[2] = recsize >> 16;
	maddr[3] = recsize >> 24;
	fwrite(maddr, 1, sizeof(maddr), file);  /* followed with count */
	fwrite(data, 1, recsize, file);     /* followed by the data */
	/* Now the count field 				 */
	return 1;

} /* end PutRec_cpe */


/*==========================================================================*
 * Outputs all header information required by a CPE format file.
 *==========================================================================*/
int PutHead_cpe(FILE *file, ulong addr, ulong hi)
{
	fwrite("CPE\001", 1, 4, file);
	fwrite("\010\000", 1, 2, file); /* set unit number to 0 */
	return 0;
} /* end PutHead_cpe */

/*==========================================================================*
 * Outputs transfer address
 *==========================================================================*/
int PutXfer_cpe(FILE *file, ulong addr)
{
	fputc(3, file);         /* set register */
	writew(144, file);      /* 144 to */
	writel(addr, file);     /* xfer addr */
	return 0;

} /* end PutXfer_cpe */

/*==========================================================================*
 * Outputs all footer information required by a CPE format file.
 *==========================================================================*/
int PutFoot_cpe(FILE *file)
{
	fputc(0, file);
	return 0;

} /* end PutFoot_cpe */

