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
	OPT_IN_SKIP=1,
	OPT_OUT_SKIP,
	OPT_BYTE_SKIP,
	OPT_COUNT,
	OPT_FILL,
	OPT_HELP,
	OPT_IN_FILE,
	OPT_OUT_FILE,
	OPT_MAX
} Options_t;

static struct option long_options[] = {
   {"infile",	required_argument,	0,  OPT_IN_FILE },
   {"outfile",	required_argument,	0,  OPT_OUT_FILE },
   {"inskip",	required_argument,	0,  OPT_IN_SKIP },
   {"outskip",	required_argument,	0,  OPT_OUT_SKIP },
   {"byteskip",	required_argument,	0,	OPT_BYTE_SKIP },
   {"count",	required_argument,	0,	OPT_COUNT },
   {"fill",		required_argument,	0,	OPT_FILL },
   {"help",		no_argument,		0,	OPT_HELP },
   {NULL,		0,					0,	0 }
};

unsigned char buff[8192];

static int help_em(const char *name)
{
			fprintf(stderr, "Usage: %s [opts]\n"
					"Where: (NOTE: For all the 'n' below, they may be expressed as hex by prefixing a 0x)\n"
					"On Linux, Input is expected from stdin, output is directed to stdout. Errors are directed to stderr.\n"
					"-i, --infile=path   - Optional path to input file. (Default is stdin on Linux; required parameter on Windows.)\n"
					"-o, --outfile=path  - Optional path to output file. (Default is stdout on Linux; required parameter on Windows.)\n"
					"-s, --inskip=n      - Set the number of bytes to skip on input. (Default is 0.)\n"
					"-S, --outskip=n     - Set the number of bytes to pre-fill output with fill character before beginning to copy input. (Default is 0.)\n"
					"-b, --byteskip=n    - Set the number of input bytes to skip while copying to output. (Can only be 1, 2, 3 or 4; see NOTE below.)\n"
					"-c, --count=n       - Sets max number of bytes to write to output not including any outskip. If larger than can be provided by input, it will fill.\n"
					"-f, --fill=n        - Sets the output fill character. Output bytes skipped will be filled. (Defaults to 0.) \n"
					"-h,-?, --help       - This message\n"
					"\n"
					"NOTE: Use byteskip to split a multi-byte per word input into a smaller one. I.e. byteskip=2 converts a 16 bit\n"
					"file by outputting every other byte. Using byteskip=3 copies every third byte, etc. To select the starting\n"
					"byte, use the --inskip option. I.e. with --byteskip=2 --inskip=1 (or any odd address) will output just the odd\n"
					"bytes of the input.\n"
					"\n"
					"Some examples:\n"
					"# Will simply copy file foo to bar without changes:\n"
					"%s < foo > bar\n"
					"# Will skip the first 256 bytes of foo and copy the rest to bar:\n"
					"%s --inskip=0x100 < foo > bar\n"
					"# Will first fill 128 bytes of bar with the fill character, then beginning with the 0x201's byte on input, copy the\n"
					"# remaining odd bytes of foo to bar:\n"
					"%s --inskip=0x201 --outskip=0x80 --byteskip=2 < foo > bar\n"
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
	U32 inSkip=0, outSkip=0, skipInAmt, skipOutAmt;
	int opt, byteSkip=1, totalOutCount=0, outWritten=0;
	int fillChr=0, option_index;
	int len, limit, ifd, ofd;
	char *endp;
	const char *inpFile=NULL, *outFile=NULL;
	FILE *tifp=stdin, *tofp=stdout;

	while ( (opt = getopt_long(argc, argv, "i:o:s:S:b:c:f:h?",
					long_options, &option_index)) != -1 )
	{
		switch (opt)
		{
		case OPT_IN_FILE:
		case 'i':
			inpFile = optarg;
			break;
		case OPT_OUT_FILE:
		case 'o':
			outFile = optarg;
			break;
		case OPT_IN_SKIP:
		case 's':
			endp = NULL;
			inSkip = strtoul(optarg,&endp,0);
			if ( !endp || *endp )
			{
				fprintf(stderr,"Bad --inskip parameter: '%s'\n", optarg);
				return 1;
			}
			break;
		case OPT_OUT_SKIP:
		case 'S':
			endp = NULL;
			outSkip = strtoul(optarg,&endp,0);
			if ( !endp || *endp )
			{
				fprintf(stderr,"Bad --outskip parameter: '%s'\n", optarg);
				return 1;
			}
			break;
		case OPT_BYTE_SKIP:
		case 'b':
			endp = NULL;
			byteSkip = strtoul(optarg,&endp,0);
			if ( !endp || *endp || byteSkip < 1 || byteSkip > 4 )
			{
				fprintf(stderr,"Bad --byteskip parameter: '%s'. Must be one of 1, 2, 3 or 4\n", optarg);
				return 1;
			}
			break;
		case OPT_COUNT:
		case 'c':
			endp = NULL;
			totalOutCount = strtoul(optarg,&endp,0);
			if ( !endp || *endp || totalOutCount < 0)
			{
				fprintf(stderr,"Bad --count parameter: '%s'\n", optarg);
				return 1;
			}
			break;
		case OPT_FILL:
		case 'f':
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
	/* Skip any leading input */
	skipInAmt = inSkip;
	while ( skipInAmt )
	{
		limit = sizeof(buff);
		if ( limit > skipInAmt )
			limit = inSkip;
		len = read(ifd, buff, limit);
		if ( len < 0 )
		{
			fprintf(stderr,"Error reading %d bytes from stdin: %s\n", limit, strerror(errno));
			return 1;
		}
		else if ( len == 0 )
		{
			fprintf(stderr,"Reached EOF on input before inaddr 0x%lX\n", inSkip);
			return 1;
		}
		skipInAmt -= len;
	}
	/* Skip any leading output */
	skipOutAmt = outSkip;
	memset(buff, fillChr, sizeof(buff));
	while ( skipOutAmt )
	{
		limit = sizeof(buff);
		if ( limit > skipOutAmt )
			limit = skipOutAmt;
		len = write(ofd, buff, limit);
		if ( len <= 0 )
		{
			fprintf(stderr,"Error writing %d bytes to stdout: %s\n", limit, strerror(errno));
			return 1;
		}
		skipOutAmt -= len;
	}
	while ( !totalOutCount || (outWritten < totalOutCount) )
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
		if ( byteSkip != 1 )
		{
			int ii;
			unsigned char *src,*dst;
			src = buff;
			dst = buff;
			for (ii=0; ii < len; )
			{
				*dst++ = *src;
				src += byteSkip;
				ii += byteSkip;
			}
			amtToWrite = dst-buff;
		}
		else
		{
			amtToWrite = len;
		}
		if ( totalOutCount && outWritten+amtToWrite > totalOutCount  )
			amtToWrite = totalOutCount-outWritten;
		len = write(ofd, buff, amtToWrite);
		if ( len <= 0 )
		{
			fprintf(stderr,"Error writing %d bytes to stdout: %s\n", amtToWrite, strerror(errno));
			return 1;
		}
		outWritten += len;
	}
	if ( totalOutCount && outWritten < totalOutCount )
	{
		memset(buff, fillChr, sizeof(buff));
		while ( outWritten < totalOutCount )
		{
			limit = sizeof(buff);
			limit /= byteSkip;
			if ( limit > totalOutCount-outWritten )
				limit = totalOutCount-outWritten;
			len = write(ofd, buff, limit);
			if ( len <= 0 )
			{
				fprintf(stderr,"Error writing %d bytes of fill to stdout: %s\n", limit, strerror(errno));
				return 1;
			}
			outWritten += len;
		}
	}
	return 0;
}

