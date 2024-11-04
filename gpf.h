#ifndef GPF_H
#define GPF_H

#include "mixit.h"      /* a prerequisite */

/* "flag" bit values */
#define GPF_M_WORD      0x00000001  /* word mode */
#define GPF_M_APPND     0x00000002  /* append to existing file */
#define GPF_M_DISPL     0x00000004  /* display message about file create/append */
#define GPF_M_ZAP       0x00000008  /* zap the buffers after writing */
#define GPF_M_START     0x00000010  /* start address supplied */
#define GPF_M_END       0x00000020  /* end address supplied */
#define GPF_M_FILL      0x00000040  /* fill character supplied */
#define GPF_M_GROUP     0x00000080  /* /GROUP flag specified */
#define GPF_M_SYMBOL    0x00000100  /* include symbols in array */
#define GPF_M_NOEOF     0x00000200  /* don't write an EOF mark (for PUTFILE) */
#define GPF_M_EOFONLY   0x00000400  /* only write an EOF mark */
#define GPF_M_MOVE      0x00000800  /* new address specified to begin writing */
#define GPF_M_MAU       0x00001000  /* bits_per_word was specified */
#define GPF_M_MODESPEC  0x00002000  /* /MODE=which was specified */
#define GPF_M_SWAP      0x00004000  /* Swap byte order was specified */
#define GPF_M_NOPAD		0x00008000  /* don't pad the eof on DIO and IMG formats */
#define GPF_M_EVENW     0x00010000  /* select even words from input */
#define GPF_M_ODDW      0x00020000  /* select odd words from input */
#define GPF_M_UNUSED    0x00040000  /* unused bits this bit and above */

/* Values for 'rectype' */

typedef enum FILE_TYPES
{
	GPF_K_UNKNOWN,	/* Undefined */
	GPF_K_LDA,		/* RT11 .LDA (16 and 32 bit addresses, I/O binary) */
	GPF_K_ROM,		/* mixit .ROM (I/O ASCII) */
	GPF_K_MAC,		/* macxx .MAC (O only ASCII)*/
	GPF_K_HEX,		/* TekHex .HEX (I/O ASCII) */
	GPF_K_DLD,		/* MOS Technology? .DLD (I/O ASCII) */
	GPF_K_VLDA,		/* Atari .VLDA or .VLD (32 bit addresses, I/O binary) */
	GPF_K_GNU,		/* GNU .ASM or .AS68K () (O only ASCII) */
	GPF_K_INTEL,	/* Intel .INTEL (I/O ASCII) */
	GPF_K_MOT,		/* Motorola .MOT (I/O ASCII) */
	GPF_K_DUMP,		/* mixit dump .DUMP, .DUM or .DMP (O only same as IMG) */
	GPF_K_DIO,		/* Atari DataIO .DIO (I/O binary) */
	GPF_K_COFF,		/* Generic COFF .COFF (I only binary) */
	GPF_K_ELF,		/* Generic ELF .ELF (I only binary) */
	GPF_K_CPE,		/* Sony Playstation .CPE (I/O binary) */
	GPF_K_IMG = 127	/* Anything else is assumed image format (I/O binary) */
} FileFormat;

/*
 *  The arguments "low_add" and "high_add" are used by GETFILE if
 *  the coresponding bits are set in the flag word (see above) otherwise
 *  GETFILE will fill in the values. If the values are present, they are
 *  assumed to be limits and consequently GETFILE won't include any file
 *  data that is found outside the limit(s). Each can be present without
 *  the other. (these variables will contain addresses of buffers and
 *  the file FAB and RAB if the file is determined to be special mode). 
 */

typedef struct gpf {
	uchar   bits_per_word;  /* number of bits per target word */
	uchar   bytes_per_word;  /* number of bits per target word */
	uchar   group_code;     /* group number */
	long    array_size;     /* size of data array */
	ulong   flags;          /* flag storage */
	Image   image;          /* root of target tree if any exists */
	ulong   low_add;        /* lowest address found in file */
	ulong   high_add;       /* highest address found in file */
	ulong   xfer_add;       /* xfer address found in the file */
	ulong	low_limit;		/* lowest address to look for (from command line) */
	ulong	high_limit;		/* highest address to look for (from command line) */
	ulong   out_add;        /* output file address (from command line) */
	ushort  rec_size;       /* default output record size */
	uchar   fill_char;      /* value to fill uninit'd bytes */
	FileFormat rec_type;    /* file format */
	ulong   sym_add;        /* symbol table address? */
	ulong   sym_end;        /* symbol table end? */
	int		recordNumber;	/* record number on files that have records */
	ulong	recordOffset;	/* Offset in input file of start of record (not all formats update this) */
	int		reportedSkippedBytes; /* Unique flag for lda input */
} GPF;                  /* this whole structure is now a GPF */

extern GPF ingpf, outgpf;

extern int getfile(char *filespec, GPF *gpf);
extern int putfile(char *filespec, GPF *gpf);
extern int init_reader(GPF *gpf);
extern int readImage(GPF *gpf, uchar *bufferPtr, ulong bufferSpace,
					 ulong low_address, ulong high_address, ulong *new_low,
					 int *bytesRead);
extern int ioparsebad(int input_parse, GPF *gpfp);

#endif /* GPF_H */
