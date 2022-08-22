#if 0
/*===========================================================================

G E T F I L E . C

	File reader for the mixit program.


	Copyright 1991 Atari Games.  All rights reserved.
	Author: Lyle Rains & Jim Petrick

===========================================================================*/
#endif

/*
 *   Imported objects, declarations and definitions:
 *         stdio.h:  NULL, size_t, fprintf(), fread(), fwrite()
 *        string.h:  memset()
 *        stdlib.h:  EXIT_FAILURE, EXIT_SUCCESS, exit(), malloc(), free()
 *         image.h:  LogicalAddr, BlockGroup, MemBlock, Chunk, Page, Image,
 *                   imageWrite(), imageFree()
 */

#include "mixit.h"

#if !defined(fileno)
extern int fileno();        /* non-ANSI */
#endif

#define DEFAULT_BUFLEN      0x1000

/*==========================================================================*
 *  group_data  - Handles the /GROUP option by copying a source array to a 
 *                destination array, but only copying the GROUP'ed bytes.
 *==========================================================================*/
static void group_data(uchar *dest, uchar *src, int count,
					   int bytes_per_word, int group_code)
{
	register int i;

#if 0
	src += bytes_per_word - group_code - 1;     /* For MSB -> LSB data machines */
#else
	src += group_code;                          /* For LSB -> MSB data */
#endif

	for ( i = 0; i < count; i += bytes_per_word )
	{
		*dest++ = *src;
		src += bytes_per_word;
	}

} /* end group_data */


/*==========================================================================*
 *  swap_data   - Handles the /SWAP option by swapping bytes in the given 
 *                array governed by bytes_per_word.
 *==========================================================================*/
static void swap_data(uchar *dest, int count, int bytes_per_word)
{
	register int i, j;
	uchar       temp;

	if ( count % bytes_per_word )
	{
		moan("Can't swap %d byte words in a %d byte record", bytes_per_word,
			 count);
		return;
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

} /* end swap_data */

/*==========================================================================*
 *  evenodd_data  - Handles the /EVEN_WORDS and /ODD_WORDS options by copying
 *		  a source array to a destination array, but only copying the
 *		  half of the words.
 *==========================================================================*/
static void evenodd_data(uchar *dest, uchar *src, int count, short bytes_per_word, int odd)
{
	register int ii, half;

	half = bytes_per_word / 2;
	if ( half == 0 )
	{
		moan("Can't take even/odd %d byte words from a %d byte word", half, bytes_per_word);
		return;
	}

	if ( count % bytes_per_word )
	{
		moan("Can't take (%d byte) words from a %d byte record", half, count);
		return;
	}

	if ( odd )
		src += half;
	for ( ii = 0; ii < count; ii += bytes_per_word )
	{
		memcpy(dest, src, half);
		dest += half;
		src += bytes_per_word;
	}

} /* end evenodd_data */


/*==========================================================================*
 *  save_data   - Deposits a record into an image after munging it based on
 *                various flag settings.
 *==========================================================================*/
void save_data(InRecord *rec, LogicalAddr userLo, LogicalAddr userHi, long relocation, GPF *gpf)
{
	size_t dataLen;
	LogicalAddr startOffset;
	
	/* Record the address limits found in the input file */
	if ( rec->recSAddr < gpf->low_add )
		gpf->low_add = rec->recSAddr;
	if ( rec->recEAddr > gpf->high_add )
		gpf->high_add = rec->recEAddr;

	/* If what we want to save isn't in the range, then just skip the whole thing */
	if ( userLo > rec->recEAddr || userHi < rec->recSAddr )
	{
		if ( debug )
		{
			printf("save_data(): Skipped %4d byte input record. lo=0x%lX, hi=0x%lX, recordAdd=0x%lX-0x%lX (relocated=0x%lX-0x%lX)\n",
				   rec->recLen, userLo, userHi, rec->recSAddr, rec->recEAddr, rec->recSAddr+relocation, rec->recEAddr+relocation );
		}
		return;
	}
	if ( !rec->beenConverted )
	{
		rec->recConvertedLen = rec->recLen;
		/* Mung the entire input buffer according to the flags */
		/* NOTE: If we know we might revisit this record, then convert it into a separate buffer */
		if ( (gpf->flags & GPF_M_GROUP) != 0 )
		{
			group_data(rec->recData, rec->recData, rec->recLen,
					   gpf->bytes_per_word, gpf->group_code);
			rec->recConvertedLen /= gpf->bytes_per_word;
		}
		else if ( (gpf->flags & GPF_M_SWAP) != 0 )
			swap_data(rec->recData, rec->recLen, gpf->bytes_per_word);
		else if ( (gpf->flags & (GPF_M_EVENW | GPF_M_ODDW)) != 0 )
		{
			int jj = (rec->recSAddr / gpf->bytes_per_word) & 1;
			jj = jj ^ ((gpf->flags & GPF_M_ODDW) != 0);
			evenodd_data(rec->recData, rec->recData, rec->recLen, gpf->bytes_per_word, jj);
			rec->recConvertedLen /= 2;
		}
		rec->recEAddr = rec->recSAddr + rec->recConvertedLen - 1;
		rec->beenConverted = 1;
	}

	/* Clip anything off the low end */
	if ( userLo < rec->recSAddr )
		userLo = rec->recSAddr;
	/* Clip anything off the high end */
	if ( userHi > rec->recEAddr )
		userHi = rec->recEAddr;
	/* Compute offset in source buffer where data begins */
	startOffset = userLo-rec->recSAddr;
	/* Compute an actual byte count to save */
	dataLen = userHi - userLo + 1;
	/* Make sure it can't be more than what we have */
	if ( dataLen > rec->recLen )
		dataLen = rec->recLen;
	if ( debug )
	{
		printf("save_data(): Saving %4d byte relocated input record. RecordAdd=0x%lX-0x%lX, startOffset=%ld, relocated=0x%lX-0x%lX\n",
			   dataLen, rec->recSAddr+startOffset, rec->recSAddr+dataLen-1, startOffset,
			   rec->recSAddr+relocation+startOffset, rec->recSAddr+relocation+dataLen-1  );
	}
	imageWrite(&gpf->image, rec->recSAddr+relocation+startOffset, dataLen, rec->recData + startOffset);
} /* end save_data */


/*==========================================================================*
 | getfile -    Reads files in various formats into the common Image struct.
 |      The gpf struct controls the start and end addresses of
 |      interest, and whether the data is moved to a new address range.
 |      Input filling is NOT done; a fill is only done on output.
 |  Params:
 |  fname   - Name of the file to read.
 |  gpf - Control structure and holder of image to build.
 |  returns - 0 for failure, 1 for success.
 *==========================================================================*/
int getfile(char *fname, GPF *gpf)
{
	InRecord    rec;            /* Buffer for input records */
	FileFormat  itype;          /* Input file type */
	int(* GetRec)(InRecord *) = 0;    /* Routine to call to input 1 record */
	int         status;         /* Return status value */
	int         reccnt;         /* Count of input records */
	LogicalAddr lo, hi;			/* Low and high address range to get */
	long		relocation;		/* offset to apply to addresses */
	long	    flag;
	char        open_opt[] = "rb";
	
	memset(&rec,0,sizeof(rec));
	flag            = gpf->flags;
	lo              = (flag& GPF_M_START) ? gpf->low_limit : 0;
	hi              = (flag& GPF_M_END) ? gpf->high_limit  : -1;
	relocation		= 0;
	if ( (flag & GPF_M_MOVE) )
	{
		relocation = ingpf.beg_add;
		if ( (flag&GPF_M_START) )
			relocation -= lo;
	}
	gpf->high_add	= 0;
	gpf->low_add	= -1;
	rec.recBufLen = DEFAULT_BUFLEN;
	if ( (rec.recBuf = (uchar *)malloc(rec.recBufLen)) == NULL )
		return err_return(0, "Not enough memory for input buffer");
/*	rec.recLen = 0; */
	itype = gpf->rec_type;
	if ( itype == GPF_K_UNKNOWN )
	{
		if ( (itype = getFormat(fname)) == GPF_K_UNKNOWN )
			return err_return(0, "File '%s' of unknown type", fname);
	} /* Read file into image */
	switch (itype)
	{
	case GPF_K_VLDA:
		GetRec = GetRec_vlda;
		break;
	case GPF_K_LDA:
		GetRec = GetRec_lda;
		break;
	case GPF_K_CPE:
		GetRec = GetRec_cpe;
		break;
	case GPF_K_DIO:
		GetRec = GetRec_dio;
		break;
	case GPF_K_IMG:
		GetRec = GetRec_img;
		break;
	case GPF_K_HEX:
		GetRec = GetRec_tekhex;
		open_opt[1] = 0;
		break;
	case GPF_K_INTEL:
		GetRec = GetRec_intel;
		break;
	case GPF_K_MOT:
		GetRec = GetRec_mot;
		break;
	case GPF_K_DLD:
		GetRec = GetRec_dld;
		break;
	case GPF_K_ROM:
		GetRec = GetRec_rom;
		open_opt[1] = 0;
		break;
	case GPF_K_COFF:
		break;
#if INCLUDE_ELF
	case GPF_K_ELF:
		break;
#endif
	default:
		return err_return(0, "Type not implemented: '%s'", fname);
	}
	if ( (rec.recFile = fopen(fname, open_opt)) == (FILE *)NULL )
		return perr_return(0, "Can't open '%s'", fname);
	if ( (flag & GPF_M_DISPL) != 0 )
		printf("Reading file '%s' . . .\n", fname);
/*	rec.recPrivate = 0; */
/*	rec.recSAddr = 0; */
/*	rec.recSegBase = 0; */
	for ( status = reccnt = 1; status == 1; ++reccnt )
	{
		if ( itype == GPF_K_COFF )
		{
			status = GetRec_coff(gpf, &rec, lo, hi, relocation);
			break;
		}
#if INCLUDE_ELF
		else if ( itype == GPF_K_ELF )
		{
			status = GetRec_elf(gpf, &rec, lo, hi, relocation);
			break;
		}
#endif
		switch ((*GetRec)(&rec))
		{
		case REC_DATA:
			/* This record contains some data that we're looking for */
#if 1
/*			save_data(&rec, addrS - lo + base, gpf); */
			save_data(&rec,lo,hi,relocation,gpf);
#else
			{
				if ((flag & GPF_M_GROUP) != 0)
				{
					group_data( rec.recData, rec.recData, rec.recLen, gpf->bytes_per_word, gpf->group_code );
					rec.recLen /= gpf->bytes_per_word;
				}
				else if ((flag & GPF_M_SWAP) != 0)
				{
					swap_data( rec.recData, rec.recLen, gpf->bytes_per_word );
				}
				imageWrite( &gpf->image, addr - lo + base, rec.recLen, rec.recData);
			}
#endif
			continue;
		case REC_XFER:
			gpf->xfer_add = rec.recSAddr;
			continue;
		case REC_TRANSPARENT:
			if ( flag & GPF_M_SYMBOL )
				symbolWrite(&gpf->image, rec.recData, rec.recLen);
			continue;
		case REC_UNKNOWN:
			continue;
		case REC_ERR:
			moan("Error on record %d", reccnt);
			status = 0;
			break;
		case REC_EOF:
		default:
			status = -1;
			break;
		}
	}
	fclose(rec.recFile);
	free(rec.recBuf);
	return (status == -1);
} /* end getfile */
