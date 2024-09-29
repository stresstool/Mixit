#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>

typedef unsigned long U32;
typedef unsigned short U16;

typedef enum
{
	OPT_INADDR=1,
	OPT_OUTADDR,
	OPT_WORD_SIZE,
	OPT_BYTE,
	OPT_OUT_SIZE,
	OPT_FILL,
	OPT_HELP,
	OPT_IN_FILE,
	OPT_OUT_FILE,
	OPT_MAX
} Options_t;

static struct option long_options[] = {
   {"infile",	required_argument,	0,  OPT_IN_FILE },
   {"outfile",	required_argument,	0,  OPT_OUT_FILE },
   {"inaddr",	required_argument,	0,  OPT_INADDR },
   {"outaddr",	required_argument,	0,  OPT_OUTADDR },
   {"word_size",required_argument,	0,  OPT_WORD_SIZE },
   {"byte",		required_argument,	0,	OPT_BYTE },
   {"out_size",	required_argument,	0,	OPT_OUT_SIZE },
   {"fill",		required_argument,	0,	OPT_FILL },
   {"help",		no_argument,		0,	OPT_HELP },
   {NULL,		0,					0,	0 }
};

unsigned char buff[65536*2];

static int help_em(const char *name)
{
			fprintf(stderr, "Usage: %s [opts]\n"
					"Where: (NOTE: For all the 'n' below, they may be expressed as hex by prefixing a 0x)\n"
					"Input is expected from stdin, output is directed to stdout. Errors are directed to stderr.\n"
					"--infile=path   - Optional path to input file (default is stdin on Linux; required parameter on Windows)\n"
					"--outfile=path  - Optional path to output file (default is stdout on Linux; required parameter on Windows)\n"
					"--inaddr=n      - set the input file offset in bytes (default is 0)\n"
					"--outaddr=n     - set the output file offset in bytes (default is 0; NOTE below)\n"
					"--word_sizez=n  - sets the word_size (default is 8; Can only be 8, 16, 24 or 32\n"
					"--byte=n        - selects which byte when word_size != 8. Can only be 0, 1, 2 or 3 for word_sizes of 8, 16, 24 and 32 respectively.\n"
					"                - I.e. if word_size==16, then --byte can be 0 or 1 for even or odd\n"
					"--out_size=n    - sets max number of bytes written to output\n"
					"--fill=n        - sets the output fill character (defaults to 0)\n"
					"--help          - this message\n"
					"\n"
					"NOTE: the outaddr is always the BYTE offset in the output file and will not\n"
					"follow that of the input if word_size != 8. Be sure to account for word_size\n"
					"when computing a respective outaddr.\n"
					"\n"
					"Some examples:\n"
					"%s < foo > bar   # Will simply copy file foo to bar without changes\n"
					"%s --inaddr=0x100 < foo > bar # Will skip the first 256 bytes of foo and copy the rest to bar\n"
					"%s --inaddr=0x200 --outadd=0x80 --word_size=16 --byte=1 < foo > bar # will copy odd bytes of foo to bar\n"
					"etc.\n"
					,name
					,name
					,name
					,name
					);
			return 1;
}

int main(int argc, char *argv[])
{
	U32 inAddr=0, skipInAmt, skipOutAmt, outAddr=0;
	int opt, wordSize=8, bytesPerWord, outSize=0, outWritten=0, selByte=0;
	int fillChr=0, option_index;
	int len, limit, ifd, ofd;
	char *endp;
	const char *inpFile=NULL, *outFile=NULL;
	FILE *tifp=stdin, *tofp=stdout;

	while ( (opt = getopt_long(argc, argv, "h?",
					long_options, &option_index)) != -1 )
	{
		switch (opt)
		{
		case OPT_IN_FILE:
			inpFile = optarg;
			break;
		case OPT_OUT_FILE:
			outFile = optarg;
			break;
		case OPT_INADDR:
			endp = NULL;
			inAddr = strtoul(optarg,&endp,0);
			if ( !endp || *endp )
			{
				fprintf(stderr,"Bad --inaddr parameter: '%s'\n", optarg);
				return 1;
			}
			break;
		case OPT_OUTADDR:
			endp = NULL;
			outAddr = strtoul(optarg,&endp,0);
			if ( !endp || *endp )
			{
				fprintf(stderr,"Bad --outaddr parameter: '%s'\n", optarg);
				return 1;
			}
			break;
		case OPT_WORD_SIZE:
			endp = NULL;
			wordSize = strtoul(optarg,&endp,0);
			if ( !endp || *endp || (wordSize != 8 && wordSize != 16 && wordSize != 24 && wordSize != 32))
			{
				fprintf(stderr,"Bad --word_size parameter: '%s'. Must be one of 8, 16, 24 or 32\n", optarg);
				return 1;
			}
			break;
		case OPT_BYTE:
			endp = NULL;
			selByte = strtoul(optarg,&endp,0);
			if ( !endp || *endp || selByte < 0 || selByte > 3)
			{
				fprintf(stderr,"Bad --byte parameter: '%s'. Must be one of 0, 1, 2 or 3\n", optarg);
				return 1;
			}
			break;
		case OPT_OUT_SIZE:
			endp = NULL;
			outSize = strtoul(optarg,&endp,0);
			if ( !endp || *endp )
			{
				fprintf(stderr,"Bad --inaddr parameter: '%s'\n", optarg);
				return 1;
			}
			break;
		case OPT_FILL:
			endp = NULL;
			fillChr = strtoul(optarg,&endp,0);
			if ( !endp || *endp || fillChr < -256 || fillChr > 255 )
			{
				fprintf(stderr,"Bad --fill parameter: '%s'. Must be between 0 and 255.\n", optarg);
				return 1;
			}
			break;
		case OPT_HELP:
		case 'h':
		case '?':
			return help_em(argv[0]);
		default: /* '?' */
			fprintf(stderr,"Invalid command line option '%c' (0%o, 0x%02X)\n", isprint(opt)?opt:'.',opt,opt);
			help_em(argv[0]);
			return 1;
		}
	}
	bytesPerWord = wordSize/8;
	switch (bytesPerWord)
	{
	default:
	case 1:
		if ( selByte != 0 )
		{
			fprintf(stderr,"Option --byte can only be 0 with word_size==8\n");
			return 1;
		}
	case 2:
		if ( (inAddr&1) )
		{
			fprintf(stderr,"Since word_size==16, inAddr must be even\n");
			return 1;
		}
		if ( selByte > 1)
		{
			fprintf(stderr,"Option --byte can only be 0 or 1 (for even or odd) with word_size==16\n");
			return 1;
		}
		break;
	case 3:
		if ( selByte > 2)
		{
			fprintf(stderr,"Option --byte can only be 0, 1 or 2 with word_size==24\n");
			return 1;
		}
		if ( (inAddr&3) )
			fprintf(stderr,"Warning, Since word_size==24, inAddr probably should be a multiple of 4 to make the --byte option work correctly.\n");
		break;
	case 4:
		if ( (inAddr&3) )
		{
			fprintf(stderr,"Since word_size==32, inAddr must be multiple of 4. Select byte with --byte.\n");
			return 1;
		}
		break;
	}
	/* Skip any leading input */
	skipInAmt = inAddr;
	if ( inpFile )
	{
		tifp = fopen(inpFile,"rb");
		if ( !tifp )
		{
			fprintf(stderr,"Failed to open '%s' for input: %s\n", inpFile, strerror(errno));
			return 1;
		}
	}
#if MSYS2
	else
	{
		fprintf(stderr,"Sorry, On Windows systems, the stdin stdout thing doesn't work. So you have to specify in/out filenames with --infile and --outfile\n");
		return 1;
	}
#endif
	if ( outFile )
	{
		tofp = fopen(outFile, "wb");
		if ( !tofp )
		{
			fprintf(stderr,"Failed to open '%s' for output: %s\n", outFile, strerror(errno));
			return 1;
		}
	}
#if MSYS2
	else
	{
		fprintf(stderr,"Sorry, On Windows systems, the stdin stdout thing doesn't work. So you have to specify in/out filenames with --infile and --outfile\n");
		return 1;
	}
#endif
	ifd = fileno(tifp);
	ofd = fileno(tofp);
	while ( skipInAmt )
	{
		limit = sizeof(buff);
		if ( limit > skipInAmt )
			limit = inAddr;
		len = read(ifd, buff, limit);
		if ( len < 0 )
		{
			fprintf(stderr,"Error reading %d bytes from stdin: %s\n", limit, strerror(errno));
			return 1;
		}
		else if ( len == 0 )
		{
			fprintf(stderr,"Reached EOF on input before inaddr 0x%lX\n", inAddr);
			return 1;
		}
		skipInAmt -= limit;
	}
	/* Skip any leading output */
	skipOutAmt = outAddr;
	memset(buff, fillChr, sizeof(buff));
	while ( skipOutAmt )
	{
		limit = sizeof(buff);
		limit /= bytesPerWord;
		if ( limit > skipOutAmt )
			limit = skipOutAmt;
		len = write(ofd, buff, limit);
		if ( len <= 0 )
		{
			fprintf(stderr,"Error writing %d bytes to stdout: %s\n", limit, strerror(errno));
			return 1;
		}
		skipOutAmt -= limit;
	}
	while ( !outSize || (outWritten < outSize) )
	{
		int amtToWrite;
		
		limit = sizeof(buff);
		len = read(ifd, buff, limit);
		if ( len < 0 )
		{
			fprintf(stderr,"Error reading %d bytes from stdin: %s\n", limit, strerror(errno));
			return 1;
		}
		if ( len == 0 )
			break;
		amtToWrite = len/bytesPerWord;
		if ( outSize && (outWritten+amtToWrite > outSize) )
			amtToWrite = outSize-outWritten;
		if ( bytesPerWord != 1 )
		{
			int ii;
			unsigned char *src,*dst;
			src = buff+selByte;
			dst = buff;
			for (ii=0; ii < amtToWrite; ++ii)
			{
				*dst++ = *src;
				src += bytesPerWord;
			}
		}
		len = write(ofd, buff, amtToWrite);
		if ( len <= 0 )
		{
			fprintf(stderr,"Error writing %d bytes to stdout: %s\n", amtToWrite, strerror(errno));
			return 1;
		}
		outWritten += len;
	}
	if ( outSize && outWritten < outSize )
	{
		memset(buff, fillChr, sizeof(buff));
		while ( outSize && outWritten < outSize )
		{
			limit = sizeof(buff);
			limit /= bytesPerWord;
			if ( limit > outSize-outWritten )
				limit = outSize-outWritten;
			len = write(ofd, buff, limit);
			if ( len <= 0 )
			{
				fprintf(stderr,"Error writing %d bytes of fill to stdout: %s\n", limit, strerror(errno));
				return 1;
			}
			outWritten += limit;
		}
	}
	return 0;
}

