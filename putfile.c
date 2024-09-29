#if 0
===========================================================================

P U T F I L E . C

File reader for the mixit program.


Copyright 1991 Atari Games.  All rights reserved.
Author: Lyle Rains & Jim Petrick

===========================================================================
#endif

/*
 *	 Imported objects, declarations and definitions:
 *	       stdio.h:  NULL, size_t, fprintf(), fread(), fwrite()
 *	      string.h:  memset()
 *	      stdlib.h:  EXIT_FAILURE, EXIT_SUCCESS, exit(), malloc(), free()
 *	       image.h:  LogicalAddr, BlockGroup, MemBlock, Chunk, Page, Image,
 *	                 imageWrite(), imageFree()
 */

#include "mixit.h"

#define DEFAULT_BUFLEN 		0x1000

extern int debug;
#if !defined(fileno)
extern int fileno();        /* non-ANSI */
#endif

FILE	*fout = (FILE *)NULL;
ulong	last_address, base_address;
uchar	*out_buf;
int		out_bufsize;
char	current_fname[80];

#ifdef TEST
extern int	main(int argc, char **argv);
#endif

#ifdef VMS
	#define FOPEN_ARG_VLDA	"wb", "rfm=var", "ctx=rec", "mrs=512"
	#define FOPEN_ARG_IMG	"wb", "rfm=fix", "mrs=512"
	#define FOPEN_ARG_APPND	"wb"
	#define FOPEN_ARG_TXT	"w"
#else
	#ifdef sun
		#define FOPEN_ARG_VLDA	"w"
		#define FOPEN_ARG_IMG	"w"
		#define FOPEN_ARG_APPND	"w+"
		#define FOPEN_ARG_TXT	"w"
	#else
		#define FOPEN_ARG_VLDA	"wb"
		#define FOPEN_ARG_IMG	"wb"
		#define FOPEN_ARG_APPND	"wb+"
		#define FOPEN_ARG_TXT	"w"
	#endif	/* sun */
#endif	/* VMS */
/*
#ifdef VMS
if ( gpf->rec_type == GPF_K_VLDA )
	fout = fopen(fname, FOPEN_ARG_VLDA);
else if ( gpf->rec_type == GPF_K_LDA || gpf->rec_type == GPF_K_DUMP || gpf->rec_type == GPF_K_IMG || gpf->rec_type == GPF_K_DIO )
	fout = fopen(fname, FOPEN_ARG_IMG);
else
#else
if ( gpf->rec_type == GPF_K_VLDA || gpf->rec_type == GPF_K_LDA || gpf->rec_type == GPF_K_DUMP || gpf->rec_type == GPF_K_IMG || gpf->rec_type == GPF_K_DIO || gpf->rec_type == GPF_K_CPE )
	if ( gpf->rec_type == GPF_K_DIO )
#ifdef sun
		fout = fopen(fname, "w+");
#else
		fout = fopen(fname, "wb+");
#endif	// sun
else
	fout = fopen(fname, "wb");
#endif	// VMS
else
	fout = fopen(fname, "w");
if ( !fout )
	return perr_return(0, "Can't open '%s'",  fname);
strcpy(current_fname, fname);
rec_count = 0;
}
}
*/

static int openOutput(const char *fname, const GPF *gpf)
{
	switch (gpf->rec_type)
	{
	case GPF_K_VLDA:    /* Atari .VLDA or .VLD (32 bit addresses: I/O binary) */
		fout = fopen(fname, FOPEN_ARG_VLDA);
		break;
	case GPF_K_DIO:     /* Atari DataIO .DIO (I/O binary) */
		fout = fopen(fname, FOPEN_ARG_APPND);
		break;
	case GPF_K_DUMP:    /* mixit dump .DUMP: .DUM or .DMP (O only same as IMG) */
	case GPF_K_IMG:     /* Anything else is assumed image format (I/O binary) */
	case GPF_K_LDA:     /* RT11 .LDA (16 bit addresses: I/O binary) */
	case GPF_K_COFF:    /* Generic COFF .COFF (I/O binary) */
	case GPF_K_ELF:     /* Generic ELF .ELF (I/O binary) */
	case GPF_K_CPE:     /* Sony Playstation .CPE (I/O binary) */
		fout = fopen(fname, FOPEN_ARG_IMG);
		break;
	case GPF_K_ROM:     /* mixit .ROM (I/O ASCII) */
	case GPF_K_MAC:     /* macxx .MAC (O only ASCII)*/
	case GPF_K_HEX:     /* TekHex .HEX (I/O ASCII) */
	case GPF_K_DLD:     /* MOS Technology? .DLD (I/O ASCII) */
	case GPF_K_GNU:     /* GNU .ASM or .AS68K () (O only ASCII) */
	case GPF_K_INTEL:   /* Intel .INTEL (I/O ASCII) */
	case GPF_K_MOT:     /* Motorola .MOT (I/O ASCII) */
	default:
		fout = fopen(fname, FOPEN_ARG_TXT);
		break;
	}
	if ( !fout )
	{
		return perr_return(0, "Can't open '%s'",  fname);
	}
	return 1;
}

/*==========================================================================*
 *  group_data  - Handles the /GROUP option by copying a source array to a 
 *                destination array, but only copying the GROUP'ed bytes.
 *==========================================================================*/
static int group_data(uchar *dest, uchar *src, int count,
					   int bytes_per_word, int group_code)
{
	int i;
	uchar *dstSav = dest;
	
	src += group_code;                          /* For LSB -> MSB data */

	for ( i = 0; i < count; i += bytes_per_word )
	{
		*dest++ = *src;
		src += bytes_per_word;
	}
	return dest-dstSav;
} /* end group_data */


/*==========================================================================*
 *  swap_data   - Handles the /SWAP option by swapping bytes in the given 
 *                array governed by bytes_per_word.
 *==========================================================================*/
static int swap_data(uchar *dest, int count, int bytes_per_word)
{
	int i, j;
	uchar       temp, *dstSav=dest;

	if ( count % bytes_per_word )
	{
		moan("Can't swap %d byte words in a %d byte record", bytes_per_word,
			 count);
		return 0;
	}

	while ( count > 0 )
	{
		j = bytes_per_word - 1;
		for ( i = 0; i < bytes_per_word / 2; ++i, --j )
		{
			temp    = dest[i];
			dest[i] = dest[j];
			dest[j] = temp;
		}
		count -= bytes_per_word;
		dest  += bytes_per_word;
	}
	return dest-dstSav;
} /* end swap_data */

/*==========================================================================*
 *  evenodd_data  - Handles the /EVEN_WORDS and /ODD_WORDS options by copying
 *		  a source array to a destination array, but only copying the
 *		  half of the words.
 *==========================================================================*/
static int evenodd_data(uchar *dest, uchar *src, int count, short bytes_per_word, int odd)
{
	int ii, half;
	uchar *dstSav=dest;
	
	half = bytes_per_word / 2;
	if ( half == 0 )
	{
		moan("Can't take even/odd %d byte words from a %d byte word", half, bytes_per_word);
		return 0;
	}

	if ( count % bytes_per_word )
	{
		moan("Can't take (%d byte) words from a %d byte record", half, count);
		return 0;
	}

	if ( odd )
		src += half;
	for ( ii = 0; ii < count; ii += bytes_per_word )
	{
		memcpy(dest, src, half);
		dest += half;
		src += bytes_per_word;
	}
	return dest-dstSav;
} /* end evenodd_data */


static int mungBuffer(GPF *gpf, uchar *dstBuf, int bufLen, ulong inpAddr)
{
	/* Mung the entire input buffer according to the flags */
	if ( (gpf->flags & (GPF_M_GROUP|GPF_M_SWAP|GPF_M_EVENW|GPF_M_ODDW)) )
	{
		if ( (gpf->flags & GPF_M_GROUP) )
		{
			bufLen = group_data(dstBuf, dstBuf, bufLen, gpf->bytes_per_word, gpf->group_code);
		}
		else if ( (gpf->flags & GPF_M_SWAP) )
		{
			bufLen = swap_data(dstBuf, bufLen, gpf->bytes_per_word);
		}
		else if ( (gpf->flags & (GPF_M_EVENW | GPF_M_ODDW)) )
		{
			int jj;
			jj = inpAddr & 1;
			jj = jj ^ ((gpf->flags & GPF_M_ODDW) != 0);
			bufLen = evenodd_data(dstBuf, dstBuf, bufLen, 2, jj);
		}
	}
	return bufLen;
}

/*==========================================================================*
 | Dump the Image to a file.
 *==========================================================================*/
int putfile(char *fname, GPF *gpf)
{
	Page	*page;
	int		(*PutRec)(FILE *, uchar *, int, ulong) = NULL;
	int		(*PutFoot)(FILE *) = NULL;
	int		(*PutXfer)(FILE *, ulong) = NULL;
	int		(*PutSym)(FILE *, uchar *, int) = NULL;
	int		(*PutHead)(FILE *, ulong, ulong) = NULL;
	int 	status;
	uint	rBytes;	/* Bytes per output record desired */
	uint	rMax;
	uchar	*buffer;
	LogicalAddr	lo_range, hi_range, lo_ask, lo;
	static	ulong		rec_count;
	LogicalAddr inpOffset;
	LogicalAddr outAddr;
	uint	outLo, outHi, outCount;
	
	if ( !(gpf->flags & GPF_M_APPND) )
	{
		/* Determine type of the output file */
		if ( gpf->rec_type == 0 )
		{
			if ( (gpf->rec_type = getFormat(fname)) == GPF_K_UNKNOWN )
				return err_return(0, "File '%s' of unknown type", fname);
		}
		if ( !fout )
		{
			/* File is not yet open so open it */
			if ( !openOutput(fname, gpf) )
				return 0;
			strcpy(current_fname, fname);
			rec_count = 0;
		}
	}
	if ( (gpf->flags & GPF_M_DISPL) )
	{
#if 0
		if ( isatty(fileno(errFile)) )
			fputs("\n", errFile);
#endif
		printf("%s file '%s' . . .\n", (gpf->flags & (GPF_M_APPND | GPF_M_EOFONLY)) ? "Appending to" : "Creating", fname);
	}
	/*
	 *	Currently, only .VLDA and .HEX output symbols.
	 */
	switch (gpf->rec_type)      /* Determine output record function */
	{
	case GPF_K_CPE:
		PutRec	= PutRec_cpe;
		PutHead	= PutHead_cpe;
		PutFoot	= PutFoot_cpe;
		PutXfer	= PutXfer_cpe;
		rBytes	= 512 - 1 - 4 - 4;
		rMax	= 512 - 1 - 4 - 4;
		break;
	case GPF_K_VLDA:
		PutRec	= PutRec_vlda;
		PutSym	= PutSym_vlda;
		rBytes	= 512 - 7;
		rMax	= 512 - 7;
		break;
	case GPF_K_HEX:
		PutRec	= PutRec_tekhex;
		PutSym	= PutSym_tekhex;
		PutFoot	= PutFoot_tekhex;
		rBytes	= 32;
		rMax	= 121;
		break;
	case GPF_K_MOT:
		PutRec	= PutRec_mot;
		PutFoot	= PutFoot_mot;
		rBytes	= 32;
		rMax	= 246;
		break;
	case GPF_K_GNU:
		PutRec	= PutRec_asm;
		PutHead	= PutHead_asm;
		rBytes	= 16;
		rMax	= 80;
		break;
	case GPF_K_LDA:
		PutRec	= PutRec_lda;
		PutFoot	= PutFoot_lda;
		rBytes	= 249;
		rMax	= 512;
		break;
	case GPF_K_MAC:
		PutRec	= PutRec_mac;
		PutHead	= PutHead_mac;
		rBytes	= 16;
		rMax	= 80;
		break;
	case GPF_K_INTEL:
		PutRec	= PutRec_intel;
		PutFoot	= PutFoot_intel;
		rBytes	= 32;
		rMax	= 246;
		/* ??? */ break;
	case GPF_K_DLD:
		PutRec	= PutRec_dld;
		PutFoot	= PutFoot_dld;
		rBytes	= 32;
		rMax	= 121;
		break;
	case GPF_K_ROM:
		PutRec	= PutRec_rom;
		PutHead = PutHead_rom;
		rBytes	= 16;
		rMax	= 80;
		break;
	case GPF_K_DIO:
		PutHead = PutHead_dio;
		PutFoot = PutFoot_dio;
		PutRec  = PutRec_dio;
		rBytes	= 256;
		rMax	= 256;
		if ( !(gpf->flags & GPF_M_FILL) )
		{
			gpf->flags |= GPF_M_FILL;
			gpf->fill_char = 0;
		}
		break;
	case GPF_K_DUMP:
	case GPF_K_IMG:
		PutRec	= PutRec_img;
		rBytes	= 256;
		rMax	= 512;
		if ( !(gpf->flags & GPF_M_FILL) )
		{
			gpf->flags |= GPF_M_FILL;
			gpf->fill_char = 0;
		}
		break;
	default:
		moan("Output Type not implemented: '%s'", fname);
		return 0;
	}
	if ( gpf->rec_size )          /* Explicit record size wanted*/
		rBytes = gpf->rec_size;
	if ( rBytes > rMax )
		rBytes = rMax;
	/* Create a buffer to hold the output recs */
	if ( (uint)out_bufsize < rMax + 20 )
	{
		if ( out_buf )
			free(out_buf);
		out_bufsize = 0;
		out_buf = NULL;
	}
	if ( !out_buf )
	{
		if ( !(out_buf = (uchar *)malloc(out_bufsize = rMax + 20)) )
			return err_return(0, "Can't allocate %d bytes for output record", out_bufsize);
	}
	if ( (gpf->flags & GPF_M_EOFONLY) )     /* Output footer on EOF ONLY */
	{
		int rv = (PutFoot) ? PutFoot(fout) : 1;
		fclose(fout);
		fout = 0;
		if ( out_buf )
			free(out_buf);
		out_bufsize = 0;
		out_buf = NULL;
		return rv;
	}
	/* lo_range is relocated low limit */
	lo_range = gpf->low_limit;
	/* hi_range is relocated high limit */
	hi_range = gpf->high_limit;
	/* Put out any header needed */
	if ( !(gpf->flags & GPF_M_APPND) && PutHead )
		PutHead(fout, lo_range, hi_range);
	/* Output the transfer address */
	if ( PutXfer && gpf->xfer_add )
		PutXfer(fout, gpf->xfer_add);
	/* Output the symbol records */
	if ( (gpf->flags & GPF_M_SYMBOL) && PutSym )
	{
		for ( page = gpf->image.symbolList; page; page = page->next )
			PutSym(fout, (uchar *)page->data, page->end + 1);
	}
	/* Assume failure */
	status = 0;
	/* Create a buffer for boundary chunks */
	if ( !(buffer = malloc(rBytes)) )
		return err_return(0, "Can't allocate temp buffer for putfile");
	/* Now output the data records */
	if ( !init_reader(gpf) )
		return err_return(0, "No data found to output to: %s", fname);
	outCount = 0;
	outLo = gpf->out_add;
	outHi = outLo;
	for ( lo_ask = lo_range; lo_ask <= hi_range;)
	{
		int bytes, err;
		
		err = readImage(gpf, buffer, rBytes, lo_ask, hi_range, &lo, &bytes);
		if ( debug )
		{
			printf("putfile(): readImage(..,rBytes=%d, lo_ask=0x%lX, hi_range=0x%lX, lo=0x%lX, bytes=%d) returned %d\n",
				   rBytes, lo_ask, hi_range, lo, bytes,
				   err);
		}
		if ( err == -1 )
		{
			moan("Couldn't read %d bytes from image[%d:%d]", rBytes, lo_ask, hi_range);
			goto err_exit;
		}
		else if ( bytes )
		{
			/* FYI: The contents of buffer have already been relocated by the input function(s) */
			/* I.e. gpf->low_limit has been set to gpf->out_add before we get here */
			inpOffset = lo - gpf->low_limit;
			if ( lo + bytes > hi_range+1 )
				bytes = (hi_range+1)-lo;	/* clip to the end of data (padding, if desired, happens below) */
			/* FYI: The data stored in memory remains byte orientated. If word_size is not 8, the output address and data remain to be converted */
			outAddr = inpOffset / gpf->bytes_per_word + gpf->out_add;
			/* Do any required conversions of the input and return the number of bytes to output */
			outCount = mungBuffer(gpf,buffer,bytes,inpOffset);
			if ( debug )
				printf("putfile(): After Mung(). Calling PutRec(). lo_ask=0x%lX, hi_range=0x%lX, low_limit=0x%lX, bytes=%d, lo=0x%lX, inpOffset=0x%lX, outAddr=0x%lX, outCount=0x%X, err=%d\n",
					   lo_ask, hi_range, gpf->low_limit, bytes, lo, inpOffset, outAddr, outCount, err );
			
			if ( PutRec(fout, buffer, outCount, outAddr) )
			{
				++rec_count;
				if ( debug )
				{
					int ii;
					printf("Output %ld: 1 %d-byte record, asked 0x%lX-0x%lX, found 0x%lX-0x%lX, output to 0x%lX-0x%lX\n\t",
						   rec_count, outCount,
						   lo_ask, lo_ask + bytes - 1,
						   lo, lo+bytes-1,
						   outAddr, outAddr+outCount-1);
					for ( ii = 0; ii < (outCount > 16 ? 16 : outCount); ++ii )
					{
						printf(" %02X", (unsigned char)buffer[ii]);
					}
					if ( outCount > 16 )
						printf(" ...");
					printf("\n");
				}
			}
			else
			{
				moan("PutRec failure\n");
				goto err_exit;
			}
		}
		lo_ask = lo + bytes;
		if ( outAddr+outCount > outHi )
			outHi = outAddr + outCount;
		if ( err == 0 )
			break;
	}
/* Only image and DIO files require padding */
	if ( !(gpf->flags & GPF_M_NOPAD) && (gpf->rec_type == GPF_K_IMG || gpf->rec_type == GPF_K_DIO) && rBytes && hi_range < ~1l && lo_ask <= hi_range )
	{
		lo_ask = (lo_ask - lo_range) / gpf->bytes_per_word + gpf->out_add;
		hi_range = (hi_range - lo_range) / gpf->bytes_per_word + gpf->out_add;
		if ( debug )
			printf("putfile(): Padding file. lo_ask=0x%lX, hi_range=0x%lX (%ld bytes left), rBytes=%d, bytes_per_word=%d\n",
				   lo_ask, hi_range, hi_range-lo_ask+1, rBytes, gpf->bytes_per_word);
		memset(buffer, (gpf->flags & GPF_M_FILL) ? gpf->fill_char : 0, rBytes);
		while ( lo_ask <= hi_range )
		{
			int amount;
			amount = rBytes;
			if ( (unsigned int)amount > hi_range - lo_ask + 1 )
				amount = (int)(hi_range - lo_ask + 1);
			if ( amount )
			{
				if ( PutRec(fout, buffer, amount, lo_ask) )
				{
					++rec_count;
					if ( debug )
					{
						printf("Output %ld: %d %d-byte filler record, %08lX-%08lX\n", rec_count, 1, amount, lo_ask, lo_ask + amount - 1);
					}
				}
				else
				{
					moan("PutRec failure\n");
					goto err_exit;
				}
				lo_ask += amount;
			}
		}
	}
	status = 1;		/* Signal success */
err_exit:
	/* Clean out the old image */
	imageFree(&gpf->image);
	free(buffer);
	return status;
}
/* end putfile */
#ifdef TEST
/*==========================================================================*/
int main(int argc, char** argv)
{
	GPF	gpf;
	if ( argc != 3 )
	{
		fprintf(errFile, "Usage: mytest infile outfile\n");
		exit(EXIT_FAILURE);
	}
	if ( !getfile(argv[1], &gpf) )
		exit(1);
	if ( !putfile(argv[2], &gpf) )
		moan("Putfile failure");
	imageFree(&gpf.image);
	exit(EXIT_SUCCESS);
} /* end main */
#endif

