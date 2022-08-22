#if 0
/*===========================================================================

G E T F O R M . C

	This module contains the code to determine the format of a data file based
	on its file extension.  If the file extension is one of the strings listed
	in typeTable[] below, then the associated type is returned, otherwise the
	file is assumed to be an "image" file of raw data.


	Copyright 1989 Atari Games.  All rights reserved.
	Author: Lyle Rains

---------------------------------------------------------------------------
	Revision history:

---------------------------------------------------------------------------
	Known bugs/features/limitations:

	getFormat() is implemented as a simple linear search of a list of known
	file extensions.  As implemented, best speed is obtained by putting the
	most common file extensions first.  Hashing or binary search on an ordered
	table might improve speed, but speed at this point seems unimportant.

===========================================================================*/
#endif

/*
 * Imported objects, declarations and definitions:
 * 
 *  formats.h:  FileFormat
 *  string.h:   strrchr(), strncpy()
 */

#include "mixit.h"

#define DEFAULT_FORMAT GPF_K_IMG
#define MAX_EXTEN 		8

/*
 * fileTypes[]
 *
 * Associate a file extension string with a FileFormat.
 * for speed, the most common extensions should be first in the table.
 *
 * NOTE:  the "{(char *)0, DEFAULT_FORMAT}" entry at the end is used as a
 * sentinel in the search as the default tpye, and SHOULD NOT BE
 * MOVED.  If there are common DEFAULT_FORMAT file extensions, they
 * may be explicitly listed earlier in the table for better
 * performance.
 */

static struct
{
	char        *exten;
	FileFormat  form;
} fileTypes[] = {
	{ "VLDA", GPF_K_VLDA },
	{ "VLD", GPF_K_VLDA },
	{ "LDA", GPF_K_LDA },
	{ "DUMP", GPF_K_DUMP },
	{ "DUM", GPF_K_DUMP },
	{ "DMP", GPF_K_DUMP },
	{ "DIO", GPF_K_DIO },
	{ "ROM", GPF_K_ROM },
	{ "MAC", GPF_K_MAC },
	{ "HEX", GPF_K_HEX },
	{ "SYM", GPF_K_HEX },
	{ "IMG", GPF_K_IMG },
	{ "ASM", GPF_K_GNU },
	{ "AS68K", GPF_K_GNU },
	{ "INTEL", GPF_K_INTEL },
	{ "MOT", GPF_K_MOT },
	{ "DLD", GPF_K_DLD },
	{ "COFF", GPF_K_COFF },
	{ "ELF", GPF_K_ELF },
	{ "CPE", GPF_K_CPE },
	{ (char *)0, DEFAULT_FORMAT }
};

/*==========================================================================
 * Guaranteed to place a nul-terminated string into dest.
 *==========================================================================*/
void fileExtension(char *dest, char *filename, int maxlen)
{
	char 	*tmp;

	dest[0] = '\0';
	if ( filename )
	{
		/* Strip leading path info */
		if ( (tmp = strrchr(filename, ']')) != 0 )
			filename = tmp + 1;
		if ( (tmp = strrchr(filename, '/')) != 0 )
			filename = tmp + 1;
		if ( (tmp = strrchr(filename, '\\')) != 0 )
			filename = tmp + 1;
		if ( (tmp = strrchr(filename, ':')) != 0 )
			filename = tmp + 1;
		/* Find extension and copy it to dest */
		if ( (tmp = strrchr(filename, '.')) != 0 )
		{
			--maxlen;
			strncpy(dest, tmp + 1, maxlen);
			dest[maxlen] = '\0';
		}
	}
} /* end fileExtension */

/*=========================================================================
 * FileFormat getFormat(char *fileExten)
 *
 * Perform a linear search on fileTypes[].
 * Returns: FileFormat associated with the given file extension string;
 *          DEFAULT_FORMAT otherwise.
 *========================================================================*/
FileFormat getFormat(char *filename)
{
	int 	    i;
	FileFormat  format;
	char        filetype[MAX_EXTEN];

	if ( filename )
	{
		fileExtension(filetype, filename, MAX_EXTEN);
		uppercase(filetype);
		for ( i = 0; fileTypes[i].exten && strcmp(fileTypes[i].exten, filetype); ++i ) /*empty*/;
		format = fileTypes[i].form;
	}
	else
		format = DEFAULT_FORMAT;
	return (format);

} /* end getFormat */
