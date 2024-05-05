/* MIXIT.C -- read/write files describing target memory */

#include "mixit.h"
#include "port.h"
#include "qa.h"
#if _XOPEN_SOURCE
#include "unistd.h"
#endif
#include "version.h"

char   	ttybuf[BUFSIZ],
*ttp = ttybuf;          /* cmd buf & remainder ptr 					 */
char   	token[BUFSIZ];          /* current token and its work pointer 		 */
BUF	   	inspec,
outspec;                /* accepted file specs for MIXIT routines 	 */
BUF 	filespec;               /* filespec we will collect EXACTLY one of 	 */
GPF 	ingpf, outgpf;          /* environment for getfile() / putfile() 	 */
int 	debug;                     /* Debug flag 								 */
FILE *errFile;

extern FILE *fout;              /* Current output file (NULL if none open) 	 */
/* parsing constructs 						 */
#define SYMNAM  "%256[A-Za-z0-9$_]"

#define wasbad(why) showbad(1,why)
#define isbad(why)  showbad(0,why)

static GPF* initgpf(GPF *gpfp);
static void	write_eof(void);
static void	outcmd(void);
static void	incmd(void);
static void	exitcmd(void);
static void	helpcmd(void);
int			main(int argc, char *argv[]);

/*==========================================================================*
 | zap the structure
 | returns argument for nesting in calls to getfile and putfile
 *==========================================================================*/
static GPF* initgpf(GPF *gpfp)
{
	memset(gpfp, 0, sizeof(GPF));
	/* default to include symbols & Give open msgs*/
	gpfp->flags = GPF_M_SYMBOL | (noisy ? GPF_M_DISPL : 0);
	gpfp->low_add  = (LogicalAddr)-1; /* Default to any addr OK 			 */
	imageInit(&gpfp->image, DEFAULT_PAGESIZE);

	return gpfp;
} /* end initgpf */

/*==========================================================================*/
static void write_eof(void)
{
	if ( !outspec[0] )
		return;       /* no output file 						 */
	if ( !inspec[0] )
		return;        /* it wasn't opened yet 				 */

#if 1
	outgpf.flags = GPF_M_EOFONLY;
#else 								/* Let putfile handle this switch 		 */
	switch (outgpf.rec_type)
	{
		case GPF_K_LDA:
		case GPF_K_HEX:
		case GPF_K_INTEL:
		case GPF_K_MOT:
		case GPF_K_DIO:
		case GPF_K_DLD:
		outgpf.flags = GPF_M_EOFONLY; /* say what we want 				 */
		break;
		default:                    /* other types don't need a special record*/
		break;                  /* nothing to do 						 */
	}
#endif

	memcpy(&outgpf.image, &ingpf.image, sizeof(Image));
	if ( !putfile(outspec, &outgpf) )
	{
		fprintf(errFile, "\n");     /* they've already been told 			 */
	}                           /* but finish marking it anyways 		 */

	outspec[0] = '\0';              /* now we have no output file 			 */
	return;
} /* end writeeof */


/*==========================================================================*/
static void outcmd(void)
{
	FileFormat form;
	/* we use INgpf because it is free;  outgpf is still needed */
	if ( ioparsebad(0, initgpf(&ingpf)) )
		return;

	write_eof();                    /* done with that one 					 */
	inspec[0] = '\0';               /* and no files used yet 				 */

	strcpy(outspec, filespec);
	form = getFormat(filespec);
	if ( form == GPF_K_COFF )
	{
		fprintf(errFile, "Sorry, COFF is not supported for output just yet\n");
		outspec[0] = 0;
		return;
	}
	if ( form == GPF_K_ELF )
	{
		fprintf(errFile, "Sorry, ELF32 is not supported for output just yet\n");
		outspec[0] = 0;
		return;
	}
	memcpy(&outgpf, &ingpf, sizeof(GPF)); /* Copy the structure 			 */
	imageInit(&outgpf.image, DEFAULT_PAGESIZE);
	outgpf.flags |= GPF_M_NOEOF;    /* we plan to add to the end 			 */
	return;                         /* (later) 								 */
} /* end outcmd */

/*==========================================================================*/
static void incmd(void)
{
	long relocation;
	char savec = inspec[0];         /* save former flag 					 */

	if ( ioparsebad(1, initgpf(&ingpf)) )
		return;

	if ( noisy || debug )
		printf("Processing: %s -> %s\n", filespec, outspec );
	if ( !outspec[0] )
	{
		fprintf(errFile, "No output file specified yet (use OUTPUT command first);  command ignored\n\n");
		return;
	}

	strcpy(inspec, filespec);      /* consider the file started 			 */
	if ( !(ingpf.flags & GPF_M_START) )
		ingpf.low_add = 0xFFFFFFFFL; /* lowest found so far is very high 	 */

	if ( !(outgpf.flags & GPF_M_SYMBOL) )
		ingpf.flags &= ~GPF_M_SYMBOL; /* lose 'em on input 					 */

	if ( ingpf.bits_per_word == 0 )
		ingpf.bits_per_word = 8;
	ingpf.bytes_per_word = ingpf.bits_per_word / 8;
	ingpf.rec_size      = outgpf.rec_size;

	if ( !getfile(inspec, &ingpf) )
	{                           /* error message already out 			 */
		fprintf(errFile, "\n");     /* add a blank line 					 */
		inspec[0] = savec;          /* restore state of flag 				 */
		return;
	}
	/* If out has fill copy to the other 	 */
	if ( !(ingpf.flags & GPF_M_FILL) && (outgpf.flags & GPF_M_FILL) )
	{
		ingpf.flags |= GPF_M_FILL;
		ingpf.fill_char  = outgpf.fill_char;
	}

	ingpf.flags |= ((noisy ? GPF_M_DISPL : 0) | GPF_M_ZAP | GPF_M_NOEOF)
				   | (outgpf.flags & (GPF_M_WORD | GPF_M_FILL | GPF_M_APPND | GPF_M_SYMBOL | GPF_M_GROUP | GPF_M_NOPAD));

	if ( !(ingpf.flags & GPF_M_START) ) /* if no specific start 				 */
		ingpf.beg_add = ingpf.low_add; /* start at same plc 					 */

	if ( (outgpf.flags & GPF_M_MAU) && !(ingpf.flags & GPF_M_MAU) )
	{
		ingpf.bits_per_word      = outgpf.bits_per_word;
		ingpf.flags         |= GPF_M_MAU;
	}

	ingpf.rec_type = outgpf.rec_type;
	relocation = 0;
	if ( outgpf.low_add > ingpf.low_add )
		outgpf.low_add = ingpf.low_add;
	if ( outgpf.high_add < ingpf.high_add )
		outgpf.high_add = ingpf.high_add;
	ingpf.high_add = outgpf.high_add;
	ingpf.low_add = outgpf.low_add;
	if ( !(ingpf.flags & GPF_M_START)  )
		ingpf.low_limit = ingpf.low_add;
	if ( !(ingpf.flags & GPF_M_END) )
		ingpf.high_limit = ingpf.high_add;
	if ( (ingpf.flags & GPF_M_MOVE) )
	{
		relocation = ingpf.beg_add;
		if ( (ingpf.flags&GPF_M_START) )
			relocation -= ingpf.low_limit;
	}
	ingpf.low_add += relocation;
	ingpf.high_add += relocation;
	ingpf.low_limit += relocation;
	ingpf.high_limit += relocation;
	if ( debug )
	{
		printf("incmd(): calling putfile(). relocation=0x%lX, low_add=0x%lX, hi_add=0x%lX, lo_lim=0xx%lX, hi_lim=0x%lX\n",
			   relocation, ingpf.low_add, ingpf.high_add, ingpf.low_limit, ingpf.high_limit );
	}
	/* note using INPUT gpf 						 						 */
	if ( !putfile(outspec, &ingpf) )
	{
		fprintf(errFile, "\n");     /* error text already reported 			 */
		outspec[0] = '\0';          /* say we have no file open or known 	 */
		return;
	}

	outgpf.rec_type = ingpf.rec_type; /* remember what we created 			 */
	outgpf.flags   |= GPF_M_APPND;  /* remember we've been here before 		 */
	return;                         /* at last!! 							 */
}
/* end incmd */


/*==========================================================================*/
static void exitcmd(void)
{
	write_eof();                    /* close any output file 				 */
	exit(HAPPY);                  /* back to O/S 							 */
} /* end exitcmd */


/*==========================================================================*/
static void helpcmd(void)
{
#ifdef vms
#include <descrip.h>
#include <hlpdef.h>
	int lbr$output_help(int (*out)(), int* width, char* start, struct dsc$descriptor* libname, int* flags, int (*fin)());
	int lib$put_output();
	int lib$get_input();
	static $DESCRIPTOR(help_lib, "sys$help:mixit");
	static struct dsc$descriptor_s subject =
	{
		0, 0, 0, 0
	};
	/*  globalvalue HLP$M_PROMPT; 			 */
	int help_flags = HLP$M_PROMPT;

	if ( ttp[strlen(ttp) - 1] == '\n' )
		ttp[strlen(ttp) - 1] = '\0';

	subject.dsc$a_pointer = ttp;    /* set up pointer 						 */
	subject.dsc$w_length = strlen(ttp); /* and length 						 */

	lbr$output_help(
		&lib$put_output,            /* routine to produce output 			 */
		0,                          /* ptr to width [80] 					 */
		&subject,                   /* ptr to desc. of subject text 		 */
		&help_lib,                  /* ptr to desc. of library name 		 */
		&help_flags,                /* ptr to flag field 					 */
		&lib$get_input);            /* routine to get more input 			 */
#else
#if 0
	static int n = 0;

	switch (n++)
	{
	case 0:
		printf("Sorry, no help for you!\n");
		break;
	case 1:
		printf("You are totally help-less.\n");
		break;
	case 2:
		printf("You are beyond help.\n");
		break;
	case 3:
		printf("You are helpless and alone...\n");
		break;
	default:
		printf("... totally alone ...\n");
		break;
	}
#else
	printf("Open a browser to the mixit.html file wherever it is you have it: <a href=\"file://mixit.html\" target=\"_blank\">MIXIT.HTML</a>\n");
#endif
#endif
	return;
} /* end exitcmd */

int noisy = 0;

/*==========================================================================*/
int main(int argc, char *argv[])
{
	FILE *fin = stdin;   /* input file */
	int opt,ef=0;
	const char *cmdFile=NULL;
	
	errFile = stderr;
#if 0
	--argc;         /* eat the imagename */
	++argv;
	while ( argc > 0 )
	{
		char *s;
		s = *argv;
		if ( *s++ == '-' )  /* command option? */
		{
			switch (*s)
			{
				case 'q':
				noisy = 0;
				break;
				case 'v':
				noisy = 1;
				break;
				case 'd':
				debug = 1;
				break;
				case 'h':
				case '?':
				default:
				fputs("Usage: mixit [-dqvh?] [command_file[.mix]]\n", errFile);
				return 1;
			}
			--argc;
			++argv;
			continue;
		}
		break;
	}
	if ( argc > 0 )
		cmdFile = *argv;
#else
	while ( (opt = getopt(argc, argv, "evdhqv?")) != -1 )
	{
		switch (opt)
		{
		case 'e':
			errFile = stdout;
			ef = 1;
			break;
		case 'd':
			debug = 1;
			break;
		case 'q':
			noisy = 0;
			break;
		case 'v':
			noisy = 1;
			break;
		case 'h':
		case '?':
		default: /* '?' */
			fputs("Usage: mixit [-dqvh?] [command_file[.mix]]\n", errFile);
			return 1;
		}
	}
	if ( optind < argc )
		cmdFile = argv[optind];
#endif
	if ( debug )
		printf("Options ef=%d, noisy=%d, argc=%d, optind=%d, cmdFile=%s\n", ef, noisy, argc, optind, cmdFile ? cmdFile : "<none>" );
	if ( cmdFile )
	{
		char *fname, extent[10];

		fname = malloc(strlen(cmdFile) + sizeof(extent) + 1);
		strcpy(fname, cmdFile);

		while ( 1 )
		{
			if ( (fin = fopen(fname, "r")) != 0 )
				break;
			else
			{
				fileExtension(extent, fname, 10);
				if ( !*extent )
					strcat(fname, ".mix");
				else
					err_exit("Bad command file! Can't open '%s'", fname);
			}
		}
		free(fname);
	}

	inspec[0] = outspec[0] = '\0';  /* no data files open yet */

	if ( fin == 0 || noisy )
		printf("Mixit version %s. Copyright Atari Games Corp. 1996-1998\n", REVISION);

	while ( 1 )
	{
		if ( qa5(fin, stdout, "MIXIT> ", 256, ttybuf) )
			switch (_qaval_)
			{
			case EOF:
				printf("\n");
				exitcmd();  /* won't come back */

			case EOF - 1:
				fprintf(errFile, "\nI/O error; command ignored\n\n");
				purge_qa2(fin, stdout);
				continue;

			default:
				fprintf(errFile, "\nLine too long; command ignored\n\n");
				purge_qa2(fin, stdout);
				continue;
			};

		ttp = sig(ttybuf);    /* find first word */
		if ( *ttp == '\0' )
			continue; /* blank line */
		if ( *ttp == '!' )
			continue;  /* (commented) blank line */

		if ( subscanf(ttp, SYMNAM, token) != 1 )
		{
			isbad("Invalid characters;  unrecognized command");
			continue;
		}
		ttp = sig(ttp + strlen(token)); /* skip the verb */

		switch (lookup_token(token, "EXIT", "HELP", "INPUT", "OUTPUT", 0))
		{
		case 0:
			if ( *ttp == '\0' || *ttp == '!' )
				exitcmd();  /* won't come back */
			isbad("Data Overload!  Just plain EXIT will work");
			break;
		case 1:
			helpcmd();
			break;
		case 2:
			incmd();
			break;
		case 3:
			outcmd();
			break;
		default:
			wasbad("Unrecognized command;  try again");
		}
	}

	return 1;
} /* end main */
