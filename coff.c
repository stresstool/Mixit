#if 0
/*===========================================================================

	C O F F . C


	COFF - Coff format. The Coff file format is described in the
	"Understanding and Using COFF", O'Reilly&Assoc, Nutshell handbook.

	Copyright 1994 Atari Games.  All rights reserved.
	Author: Dave Shepperd

---------------------------------------------------------------------------
	Revision history:
	940603_DMS - image creation.
	
---------------------------------------------------------------------------
	Known bugs/features/limitations:

===========================================================================*/
#endif

#include "mixit.h"
#include "coff-m68k.h"
#include "coff-internal.h"
#define ONLY_MAGICS
#include "ecoff.h"

#ifndef SEEK_SET
	#define	SEEK_SET	0	/* Set file pointer to "offset" */
	#define	SEEK_CUR	1	/* Set file pointer to current plus "offset" */
	#define	SEEK_END	2	/* Set file pointer to EOF plus "offset" */
#endif

static int endian;
static int ecoff;

#undef UNPACK2
#undef UNPACK4

#define UNPACK2(arg) (endian ? (((arg[1]&0xFF)<<8) | (arg[0]&0xFF)) : (((arg[0]&0xFF)<<8) | (arg[1]&0xFF)))
#define UNPACK4(arg) (endian ? (((arg[3]&0xFF)<<24) | ((arg[2]&0xFF)<<16) | ((arg[1]&0xFF)<<8) | (arg[0]&0xFF)) : \
                               (((arg[0]&0xFF)<<24) | ((arg[1]&0xFF)<<16) | ((arg[2]&0xFF)<<8) | (arg[3]&0xFF)))

typedef struct {
	unsigned short magic;
	char *name;
	int flag;
} MagicStr;

static MagicStr magics[] = {
	{ MC68MAGIC, "680x0" },
	{ MC68KWRMAGIC, "680x0, writeable text segments" },
	{ MC68KROMAGIC, "680x0, read only, shareable text segments" },
	{ MC68KPGMAGIC, "680x0, demand paged text segments" },
	{ M68MAGIC, "680x0, plain text segments" },
	{ M68TVMAGIC,  "680x0, readonly shareable segments" },
	{ MC68KBCSMAGIC, "680x0, names have underscores (Bull DPX/2)" },
	{ LYNXCOFFMAGIC, "680x0, LYNX coff file" },
	{ MTIMAGIC, "TI's CH31 coff file" },
	{ 0, 0 }
};

static MagicStr mips_magics[] = {
	{ MIPS_MAGIC_1, 	"MIPS R3k, ECOFF, otherwise unknown", 1 },
	{ MIPS_MAGIC_LITTLE,	"MIPS R3k, ECOFF ISA 1, little endian header/data", 1 },
	{ MIPS_MAGIC_BIG,	"MIPS R3k, ECOFF ISA 1, big endian header/data", 1 },
	{ MIPS_MAGIC_LITTLE2, "MIPS R3k, ECOFF ISA 2, little endian header/data", 1 },
	{ MIPS_MAGIC_BIG2,	"MIPS R3k, ECOFF ISA 2, big endian header/data", 1 },
	{ MIPS_MAGIC_LITTLE3, "MIPS R3k, ECOFF ISA 3, little endian header/data", 1 },
	{ MIPS_MAGIC_BIG3,	"MIPS R3k, ECOFF ISA 3, big endian header/data", 1 },
	{ 0, 0, 0 }
};

static MagicStr *magicsp[] = { magics, mips_magics, 0 };

static void flip_fh(FILHDR *fh, FileHdr *nfh)
{
	nfh->f_magic = UNPACK2(fh->f_magic);
	nfh->f_nscns = UNPACK2(fh->f_nscns);
	nfh->f_timdat = UNPACK4(fh->f_timdat);
	nfh->f_symptr = UNPACK4(fh->f_symptr);
	nfh->f_nsyms = UNPACK4(fh->f_nsyms);
	nfh->f_opthdr = UNPACK2(fh->f_opthdr);
	nfh->f_flags = UNPACK2(fh->f_flags);
	return;
}

static void flip_sec(SCNHDR *sec, SecHdr *nsec)
{
	memcpy(nsec->s_name, sec->s_name, 8);
	nsec->s_paddr = UNPACK4(sec->s_paddr);
	nsec->s_vaddr = UNPACK4(sec->s_vaddr);
	nsec->s_size = UNPACK4(sec->s_size);
	nsec->s_scnptr = UNPACK4(sec->s_scnptr);
	nsec->s_relptr = UNPACK4(sec->s_relptr);
	nsec->s_lnnoptr = UNPACK4(sec->s_lnnoptr);
	nsec->s_nreloc = UNPACK2(sec->s_nreloc);
	nsec->s_nlnno = UNPACK2(sec->s_nlnno);
	nsec->s_flags = UNPACK4(sec->s_flags);
	return;
}

int GetRec_coff(GPF *gpf, InRecord *rec, LogicalAddr lo, LogicalAddr hi, long relocation)
{
	FILHDR *ch;          /* ptr to external section struct */
	FileHdr *lch;        /* ptr to internal section struct */
	SCNHDR *esec;        /* ptr to array of external section structs */
	SecHdr  *lsec;       /* ptr to internal section struct */
	int ii, jj, ll, sts;
	typedef struct {
		FILHDR *fh;
		FileHdr lfh;
		SCNHDR *esec;     /* ptr to array of external section structs */
		SecHdr lsec;      /* local section struct */
		ulong remaining;
		int secindx;
	} Coff;
	Coff *coff;
	unsigned short mag;
	MagicStr *mp;

	sts = 0;         /* assume error */
	coff = (Coff *)calloc(sizeof(Coff) + sizeof(FILHDR), 1);
	if ( coff == 0 )
	{
		fprintf(errFile, "Out of memory allocating %d bytes in GetRec_coff\n",
				sizeof(Coff) + sizeof(FILHDR));
		return 0;
	}
	coff->fh = (FILHDR *)(coff + 1);
	ch = coff->fh;
	lch = &coff->lfh;
	if ( fread((char *)ch, sizeof(FILHDR), 1, rec->recFile) != 1 )   /* read the file header into memory */
	{
		perror("Error reading input file");
		goto clean_up;
	}
	mag = UNPACK2(ch->f_magic);  /* pickup the magic bytes */

	jj = 0;
	for ( ll = 0;; ++ll )
	{
		mp = magicsp[ll];
		if ( mp == 0 )
		{
			if ( endian )
				break;
			endian = 1;
			mag = (mag << 8) | ((mag >> 8) & 0xFF);  /* flip 'em */
			ll = -1;
			continue;
		}
		while ( mp->magic )
		{
			if ( mag == mp->magic )
			{
				jj = 1;
				break;
			}
			++mp;
		}
		if ( jj )
			break;
	}

	if ( jj == 0 )       /* no match on magic */
	{
		mag = (mag << 8) | ((mag >> 8) & 0xFF); /* flip 'em back to original */
		fprintf(errFile, "Input file is not COFF. Magic = %04X\n", mag);
		goto clean_up;
	}

	ecoff = mp->flag;

	flip_fh(ch, lch);
	coff->esec = esec = (SCNHDR *)malloc(sizeof(SCNHDR) * lch->f_nscns);
	lsec = &coff->lsec;
	if ( fseek(rec->recFile, (ii = sizeof(FILHDR) + lch->f_opthdr), SEEK_SET) < 0 )
	{
		perror("Error seeking to section table");
		goto clean_up;
	}
	if ( (ii = fread((char *)esec, sizeof(SCNHDR), lch->f_nscns, rec->recFile)) != lch->f_nscns )
	{
		perror("Error reading section table");
		goto clean_up;
	}

	if ( noisy )
		printf("%sCOFF file: Magic = %04X, Byte swap %srequired, sections=%d\n",
			   ecoff ? "E" : "", mag, endian ? "" : "not ", lch->f_nscns);

	for ( ii = 0; (unsigned short)ii < lch->f_nscns; ++ii )
	{
		LogicalAddr addr;
		long amt;                 /* amount to read */
		long startp;              /* address in file to reading next */

		flip_sec(esec + ii, lsec);
#if 0
		printf("Section %3d before test: size=%5d, addr=%08lX, scnptr=%08lX\n",
			   ii, lsec->s_size, lsec->s_vaddr, lsec->s_scnptr);
#endif
		if ( lsec->s_size == 0 )
			continue;      /* ignore empty sections */
		if ( lsec->s_scnptr == 0 )
			continue;    /* ignore sections with no raw data */
		if ( (gpf->flags & GPF_M_WORD) != 0 )   /* addressing is word mode */
		{
			lsec->s_vaddr *= gpf->bytes_per_word;  /* adjust the address and size */
			lsec->s_size *= gpf->bytes_per_word;
		}
		if ( lsec->s_vaddr > hi )
			continue;     /* ignore sections with data out of range */
		if ( lsec->s_vaddr + lsec->s_size <= lo )
			continue;
		amt = lsec->s_size;           /* assume we can read the whole thing */
		startp = lsec->s_scnptr;          /* starting here */
		addr = lsec->s_vaddr;
#if 0
		printf("Section %3d before: amt=%5d, addr=%08lX, startp=%08lX\n",
			   ii, amt, lsec->s_vaddr, startp);
#endif
		if ( addr < lo )              /* we have to lop off some amount */
		{
			long toomuch;
			toomuch = lo - addr;
			amt -= toomuch;            /* trim some off the front */
			startp += toomuch;         /* advance the file pointer */
			addr += toomuch;           /* and advance the address */
		}
		if ( addr + amt > hi )          /* too much */
		{
			amt -= addr + amt - (hi + 1);       /* lop some off the end */
		}
#if 0
		printf("             after: amt=%5d, addr=%08lX, startp=%08lX\n",
			   amt, addr, startp);
#endif
		if ( amt <= 0 )
			continue;           /* all eaten, ignore it */
		if ( fseek(rec->recFile, startp, SEEK_SET) < 0 )
		{
			perror("Error seeking to section data");
			goto clean_up;
		}
		rec->recData = rec->recBuf;
		rec->recSAddr = addr;
		rec->recEAddr = addr+amt-1;
		rec->recType = REC_DATA;
		while ( amt > 0 )
		{
			size_t len;
			len = rec->recBufLen;
			if ( len > amt )
				len = amt;
			if ( fread(rec->recBuf, 1, len, rec->recFile) != len )
			{
				char tmp[132];
				sprintf(tmp, "Error reading section data. Wanted %d, got %d\n", lch->f_nscns, ii);
				perror(tmp);
				goto clean_up;
			}
			rec->recLen = len;
/*			save_data(rec, addr - lo + base,  gpf); */
			save_data(rec, lo, hi, relocation, gpf);
			amt -= len;
			addr += len;
			rec->recSAddr = addr;
			rec->recEAddr = addr + len -1;
		}
	}
	sts = -1;        /* this means ok */
clean_up:
	if ( coff )
	{
		if ( coff->esec )
			free(coff->esec);
		free(coff);
	}
	return sts;
}

