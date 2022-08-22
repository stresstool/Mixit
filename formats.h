#if 0
===========================================================================

F O R M A T S . H

	An enumerated list of known file formats.

	Copyright 1989 Atari Games.  All rights reserved.
	Author: Lyle Rains

===========================================================================
#endif

#ifndef FORMATS_H
#define FORMATS_H

#if 0			/* replaced with GPF_K_xxx enums */
typedef enum
{
	UNKNOWN, VLDA, LDA, IMG, DUMP, MAC, 
		TEKHEX, ASM68K, INTEL, MOT, DLD, ROM, DIO, COFF, ELF
} FileFormat;
#endif

extern FileFormat   getFormat( char *fspec );

#endif /* FORMATS_H */
