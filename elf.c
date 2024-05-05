#if 0
/*===========================================================================

	E L F . C

		ELF32 file format as described in the IDT programmers reference man.

	Copyright 1995 Time Warner Interactive, Inc. All rights reserved.
	Author: Dave Shepperd

---------------------------------------------------------------------------
	Revision history:
	951125_DMS - image creation.
	
---------------------------------------------------------------------------
	Known bugs/features/limitations:

===========================================================================*/
#endif

#if INCLUDE_ELF
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "mixit.h"
#include "elf.h"

#ifndef SEEK_SET
	#define	SEEK_SET	0	/* Set file pointer to "offset" */
	#define	SEEK_CUR	1	/* Set file pointer to current plus "offset" */
	#define	SEEK_END	2	/* Set file pointer to EOF plus "offset" */
#endif

#if defined(sun)
extern void fprintf(), perror(), printf(), fclose();
extern int fread();
extern char* ctime();
#endif

static int endian;

#define FLIP2(x) flip2((uchar *)&(x))
#define FLIP4(x) flip4((uchar *)&(x))

static void flip2(uchar *src)
{
	int t;
	t = src[1];
	src[1] = src[0];
	src[0] = t;
	return;
}

static void flip4(uchar *src)
{
	int t0, t1, t2, t3;
	t0 = src[0];
	t1 = src[1];
	t2 = src[2];
	t3 = src[3];
	src[0] = t3;
	src[1] = t2;
	src[2] = t1;
	src[3] = t0;
	return;
}

#define ENDIAN (endian != 0)

/* flip_eh - do big/little endian flops for the ELF header struct
 * At entry:
 *      neh - points to flipped internal ELF header struct (output)
 *       eh - points to packed ELF header read from disk (input)
 * At exit:
 *      members of neh updated to contain adjusted copies of input
 *
 * Note that if neh is 0, the input is transformed in place
 */

static void flip_eh(Elf32_Ehdr *neh, Elf32_Ehdr *eh)
{
	if ( eh != neh && neh != 0 )
	{
		memcpy((char *)neh, (char *)eh, sizeof(Elf32_Ehdr));
	}
	else
	{
		neh = eh;
	}
	if ( ENDIAN )
	{
		FLIP2(neh->e_type);
		FLIP2(neh->e_machine);
		FLIP4(neh->e_version);
		FLIP4(neh->e_entry);
		FLIP4(neh->e_phoff);
		FLIP4(neh->e_shoff);
		FLIP4(neh->e_flags);
		FLIP2(neh->e_ehsize);
		FLIP2(neh->e_phentsize);
		FLIP2(neh->e_phnum);
		FLIP2(neh->e_shentsize);
		FLIP2(neh->e_shnum);
		FLIP2(neh->e_shstrndx);
	}
	return;
}

/* flip_ph - do big/little endian flops for the program header struct
 * At entry:
 *      nph - points to flipped internal program header struct (output)
 *       ph - points to packed program header read from disk (input)
 *     size - size of the program header
 * At exit:
 *      members of nph updated to contain adjusted copies of input
 *
 * Note that if nph is 0, the input is transformed in place
 */

static void flip_ph(Elf32_Phdr *nph, Elf32_Phdr *ph, int size)
{
	if ( ph != nph && nph != 0 )
	{
		memcpy((char *)nph->p_type, (char *)ph->p_type, size);
	}
	else
	{
		nph = ph;
	}
	if ( ENDIAN )
	{
		FLIP4(nph->p_type);
		FLIP4(nph->p_offset);
		FLIP4(nph->p_vaddr);
		FLIP4(nph->p_paddr);
		FLIP4(nph->p_filesz);
		FLIP4(nph->p_memsz);
		FLIP4(nph->p_flags);
		FLIP4(nph->p_align);
	}
	return;
}

int GetRec_elf(GPF *gpf, InRecord *rec, LogicalAddr lo, LogicalAddr hi, long relocation)
{
	Elf32_Ehdr lleh, *leh = &lleh;
	Elf32_Phdr *llph = 0, *lph;
	int ii, sts;
/*   char name[9]; */
	union
	{
		unsigned short s;
		unsigned char c[2];
	} endian_test;

/*   name[8] = 0; */

	sts = 0;
	sts = fread((char *)leh, sizeof(Elf32_Ehdr), 1, rec->recFile);
	if ( sts != 1 )
	{
		perror("Error reading Elf file header");
		return 0;
	}

	if ( leh->e_ident[0] != 0x7F ||
		 leh->e_ident[1] != 'E' ||
		 leh->e_ident[2] != 'L' ||
		 leh->e_ident[3] != 'F' )
	{
		fprintf(errFile, "Not an ELF format file\n");
		return 0;
	}

	endian_test.s = 1;
	if ( endian_test.c[0] == 0 )
	{
		endian = 1;           /* we're a big endian machine */
	}
	if ( endian )            /* if big endian computer */
	{
		if ( leh->e_ident[5] == 2 )   /* if big endian file */
		{
			endian = 0;            /* no flip required */
		}
	}
	else             /* little endian computer */
	{
		if ( leh->e_ident[5] == 2 )   /* if big endian file */
		{
			endian = 1;            /* flip required */
		}
	}
#if 0
	printf("Struct member flip's %srequired\n", endian ? "" : "not ");
#endif
	flip_eh(0, leh);         /* do any necessary flipping */
#if 0
	printf("Type=0x%X, Machine=%d, Version=%d, Entry=%08lX\n",
		   leh->e_type, leh->e_machine, leh->e_version, leh->e_entry);
	printf("Phoff=%d, Shoff=%d, flags=%08lX\n", leh->e_phoff, leh->e_shoff, leh->e_flags);
	printf("Ehsize=%d, Phentsize=%d, Phnum=%d, Shentsize=%d\n",
		   leh->e_ehsize, leh->e_phentsize, leh->e_phnum, leh->e_shentsize);
	printf("Shnum=%d, Shstrndx=%d\n", leh->e_shnum, leh->e_shstrndx);
#endif
	sts = fseek(rec->recFile, leh->e_phoff, SEEK_SET);
	if ( sts < 0 )
	{
		perror("Error seeking to ELF32 Phdr area");
		return 0;
	}
	llph = (Elf32_Phdr *)malloc(leh->e_phentsize * leh->e_phnum);
	if ( llph == 0 )
	{
		perror("Ran out of memory allocating space for program headers");
		return 0;
	}
	lph = llph;
	sts = fread((char *)lph, leh->e_phentsize, leh->e_phnum, rec->recFile);
	if ( sts != leh->e_phnum )
	{
		char emsg[132];
		sprintf(emsg, "Error reading progam headers, expected %d got %d", leh->e_phnum, sts);
		perror(emsg);
		goto clean_up;
	}

#if 0
	for (ii=0;
		 ii < (int)leh->e_phnum;
		 ++ii, ++lph)
	{
		flip_ph(0, lph, leh->e_phentsize);
		printf("Program section %d:\n", ii);
		printf("\ttype=0x%X, offset=%d, vaddr=%08lX, paddr=%08lX\n",
			   lph->p_type, lph->p_offset, lph->p_vaddr, lph->p_paddr);
		printf("\tfilesize=%d, memsize=%d, flags=%08lX, align=%d\n",
			   lph->p_filesz, lph->p_memsz, lph->p_flags, lph->p_align);
	}
#endif

	if ( noisy )
		printf("ELF32 file: Byte swap %srequired, sections=%d\n",
			   endian ? "" : "not ", leh->e_phnum);

	for ( ii = 0; ii < (int)leh->e_phnum; ++ii, ++lph )
	{
		LogicalAddr addr;
		long amt;                 /* amount to read */
		long startp;              /* address in file to reading next */

		flip_ph(0, lph, leh->e_phentsize);
		if ( lph->p_filesz == 0 )
			continue;     /* there's nothing in the file */
		if ( lph->p_memsz == 0 )
			continue;      /* there's nothing in memory */
		if ( (gpf->flags & GPF_M_WORD) != 0 )   /* addressing is word mode */
		{
			lph->p_vaddr *= gpf->bytes_per_word;   /* adjust the address and size */
			lph->p_memsz *= gpf->bytes_per_word;
		}
		if ( lph->p_vaddr > hi )
			continue;      /* ignore sections with data out of range */
		if ( lph->p_vaddr + lph->p_memsz <= lo )
			continue; /* ditto */
		amt = lph->p_filesz;          /* assume we can read the whole thing */
		startp = lph->p_offset;           /* starting here */
		addr = lph->p_vaddr;
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
		rec->recType = REC_DATA;
		while ( amt > 0 )
		{
			size_t len;
			len = rec->recBufLen;
			if ( len > amt )
				len = amt;
			if ( (sts = fread(rec->recBuf, 1, len, rec->recFile)) != len )
			{
				char tmp[132];
				sprintf(tmp, "Error reading section %d data. Wanted %d, got %d\n", ii, len, sts);
				perror(tmp);
				goto clean_up;
			}
			rec->recLen = len;
			rec->recSAddr = addr;
			rec->recEAddr = addr+len-1;
/*			save_data(rec, addr - lo + base, gpf); */
			save_data(rec, lo, hi, relocation, gpf);
			amt -= len;
			addr += len;
		}
	}
clean_up:
	if ( llph != 0 )
		free(llph);
	sts = -1;        /* this means ok */
	return sts;
}
#else
void dummy_elf(void)
{
	/* Keep gcc from complaining */
}
#endif	/* INCLUDE_ELF */
