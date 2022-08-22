/* PARSES.C -- subroutines used in command parsing */

#include "mixit.h"

#ifndef gcc
	#include <stdarg.h>
#endif

#include "port.h"

static int	numbad(char base);
static int	valbad(char base);
static int	addrnfg(GPF *gpfp);
static int	addrbad(GPF *gpfp);
static int	typebad(int input_parse, GPF *gpfp);
static int	outqualbad(int input_parse, GPF *gpfp);
static int	qualbad(int input_parse, GPF *gpfp);

/* global to these routines 			 */
extern char ttybuf[BUFSIZ], *ttp;   /* cmd buf & remainder ptr 				 */
extern char token[BUFSIZ];      /* current token and its work pointer 	 */
extern BUF  inspec;                 /* accepted file specs for MIXIT routines*/
extern GPF  outgpf;                 /* environment for getfile() / putfile() */
extern BUF  filespec;               /* filespec we will collect EXACTLY one of*/

static ulong num;                   /* value found by numbad()|valbad() 	 */

/* parsing constructs 					 */
#define SYMNAM  "%256[A-Za-z0-9$_]"

#if defined(VMS)
	#define FILESPEC	"%256[]-[.:;A-Za-z0-9$_]"
#endif
#if defined(MS_DOS)
	#define FILESPEC 	"%256[\\.:A-Za-z0-9$_]"
#endif
#if !defined(VMS) && !defined(MS_DOS)
	#define FILESPEC 	"%256[!-~]"
#endif

#define QUALIFIER	"-/"		/* either a dash or a slash works as qualifier delim */

#define wasbad(why) showbad(1,why)
#define isbad(why)  showbad(0,why)

/*==========================================================================*
 * skip white space at start of text, returning ptr to 1st significant char *
 *==========================================================================*/
char* sig(char *text)
{
	while ( isspace(*text) )
		text++;
	return text;
} /* end sig */

/*==========================================================================*/
int showbad(int before, char *why)
{
	char *cp = ttybuf - 1;

	if ( in(&ttybuf[0], ttp, &ttybuf[BUFSIZ - 1]) )   /* ttp is valid */
	{
		fprintf(stderr, "%s:\n", why);
		fprintf(stderr, "in your line: %s", ttybuf);
		*ttp = '\0';    /* end the string here */
		while ( *++cp )
			if ( *cp != '\t' )
				*cp = ' ';
		fprintf(stderr, "at or %s:%s^\n\n", before ? "before" : "after", ttybuf);
	}
	else
		fprintf(stderr, "%s\n\n", why);

	return 1;
} /* end showbad */

/*==========================================================================*/
static int numbad(char base)  /* name's eaten, including '=' or ':' 	 */
{
	char newbase[] = { '%', 'l', 0, '\0' }; /* conversion area 			 */
	char *str = 0;                /* to decide format string for given base*/
	char *cp0, *cp1, *cp2;          /* work pointers for double checking 	 */

	newbase[2] = base;
	if ( ttp[0] == '0' && (ttp[1] == 'x' || ttp[1] == 'X') )
	{
		newbase[2] = 'x';
		ttp += 2;
	}
	else if ( *ttp == '%' )
	{
		if ( subscanf(++ttp, "%1[oOdDxX]", &newbase[2]) != 1 )
			return isbad("Grody radix on number");
		ttp++;
	}

	newbase[2] = tolower(newbase[2]);
#ifndef vax
	if ( newbase[2] == 'd' )
		newbase[2] = 'u'; /* no signed numbers 			 */
#endif

	if ( sscanf(ttp, newbase, &num) != 1 )
		return isbad("Invalid numeric value, or number too big");

	switch (newbase[2])
	{
	case 'o':
		str = "%100[0-7]";
		break;
	case 'u':
		str = "%100[0-9]";
		break;
	case 'd':
		str = "%100[0-9]";
		break;
	case 'x':
		str = "%100[0-9a-fA-F]";
		break;
	default:
		isbad("LOGIC ERROR!! conversion default can't happen");
		exit(FATAL);
	}

	if ( subscanf(ttp, str, token) != 1 )
	{
		isbad("LOGIC ERROR!! can't find number just converted");
		exit(FATAL);
	}
	cp1 = ttp + strlen(token);    /* just past real number 				 */

	if ( subscanf(ttp, SYMNAM, token) != 1 )
	{
		isbad("LOGIC ERROR!! can't find supernumber just converted");
		exit(FATAL);
	}
	cp2 = ttp + strlen(token);    /* just past real token 				 */

	if ( (cp0 = sig(cp1)) != sig(cp2) )
		return isbad("Garbage in numeric value");

	ttp = cp0;                      /* move past number 					 */

	return 0;                       /* we liked it! 						 */
} /* end numbad */


/*==========================================================================*/
static int valbad(char base)  /* name's eaten, but not '=' or ':' 	 */
{
	if ( *ttp == ':' || *ttp == '=' )
		ttp = sig(++ttp);         /* move to value 						 */
	else
		return wasbad("Value required on qualifier");

	return (numbad(base));
} /* end valbad */


/*==========================================================================*/
static int addrnfg(GPF *gpfp)     /* consist. ck after qual was parsed 	 */
{
	if ( (gpfp->flags & GPF_M_START) && (gpfp->flags & GPF_M_END) )
		if ( gpfp->low_limit > gpfp->high_limit )
			return wasbad("Conflicting START and END addresses");
	return 0;                       /* Was ok 								 */
} /* end addrnfg */

/*==========================================================================*/
static int addrbad(GPF *gpfp)     /* name was eaten, but not ':' or '=' 	 */
{
	if ( *ttp == ':' || *ttp == '=' )
		ttp = sig(++ttp);         /* move to value 						 */
	else
		return wasbad("Value(s) required on qualifier");

	if ( *ttp == '(' )
		while ( 1 )
		{                       /* long form 							 */
			ttp = sig(++ttp);     /* move to keyword 						 */
			if ( subscanf(ttp, SYMNAM, token) != 1 )
				return !isbad("Invalid keyword");

			ttp = sig(ttp + strlen(token)); /* move on 					 */

			switch (lookup_token(token, "START", "END", "OUTPUT", 0))
			{
			case 0:             /* START 								 */
				gpfp->low_limit    = num;
				gpfp->flags |= GPF_M_START;
				break;

			case 1:             /* END 									 */
				gpfp->high_limit   = num;
				gpfp->flags |= GPF_M_END;
				break;

			case 2:             /* OUTPUT 								 */
				gpfp->beg_add    = num;
				gpfp->flags |= GPF_M_MOVE;
				break;

			default:
				if ( valbad('x') )
					return 1;
				isbad("Unrecognized option");
				return 1;
			}

			if ( *ttp == ')' )
			{
				ttp = sig(++ttp);
				return addrnfg(gpfp); /* one way or another 				 */
			}

			if ( *ttp != ',' )
			{
				isbad("Comma or close parenthesis required");
				return 1;
			}
		}                       /* end of long form (and while) 		 */
	else
	{                           /* short form 							 */
		if ( numbad('x') )
			return 1;
		gpfp->low_limit    = num;
		gpfp->flags |= GPF_M_START;
		if ( *ttp == ':' )
			ttp = sig(++ttp);     /* move to value 						 */
		else
			return addrnfg(gpfp); /* one way or another 					 */

		if ( numbad('x') )
			return 1;
		gpfp->high_limit   = num;
		gpfp->flags |= GPF_M_END;
		if ( *ttp == ':' )
			ttp = sig(++ttp);     /* move to value 						 */
		else
			return addrnfg(gpfp); /* one way or another 					 */

		if ( numbad('x') )
			return 1;
		gpfp->beg_add = num;
		gpfp->flags |= GPF_M_MOVE;
		return addrnfg(gpfp);     /* one way or another 					 */
	}                           /* end of short form 					 */
} /* end addrbad */


/*==========================================================================*/
static int typebad(int input_parse, GPF *gpfp)    /* slash already eaten */
{
	/* returns -1 if qual not recognized, 0 for accepted, 1 for bad */

	switch (lookup_token(token, "LDA", "ROM", "MAC", "TEKHEX", "HEX",
						 "MOSTECH", "VLDA", "IMAGE", "IMG", "ASM", "GNU",
						 "INTEL", "MOTOROLA", "COFF", "ELF", "DIO", "CPE", 0))
	{
	case 0:
		gpfp->rec_type = GPF_K_LDA;
		break;
	case 1:
		gpfp->rec_type = GPF_K_ROM;
		break;
	case 2:
		if ( !input_parse )
			gpfp->rec_type = GPF_K_MAC;
		break;
	case 3:
	case 4:
		gpfp->rec_type = GPF_K_HEX;
		break;
	case 5:
		gpfp->rec_type = GPF_K_DLD;
		break;
	case 6:
		gpfp->rec_type = GPF_K_VLDA;
		break;
	case 7:
	case 8:
		gpfp->rec_type = GPF_K_IMG;
		break;
	case 9:
	case 10:
		if ( !input_parse )
			gpfp->rec_type = GPF_K_GNU;
		break;
	case 11:
		gpfp->rec_type = GPF_K_INTEL;
		break;
	case 12:
		gpfp->rec_type = GPF_K_MOT;
		break;
	case 13:
		if ( !input_parse )
		{
			fprintf(stderr, "Sorry, COFF is not supported for output just yet\n");
			return 1;
		}
		gpfp->rec_type = GPF_K_COFF;
		break;
	case 14:
		if ( !input_parse )
		{
			fprintf(stderr, "Sorry, ELF32 is not supported for output just yet\n");
			return 1;
		}
		gpfp->rec_type = GPF_K_ELF;
		break;
	case 15:
		gpfp->rec_type = GPF_K_DIO;
		break;
	case 16:
		gpfp->rec_type = GPF_K_CPE;
		break;
	default:
		return -1;
	}
	return 0;
} /* end typebad */


/*==========================================================================*/
static int outqualbad(int input_parse, GPF *gpfp)     /* slash already eaten */
{
	/*
	 *	returns -1 if qual not recognized, 0 for accepted, 1 for bad
	 * only called if output parse or first input parse
	 */

	switch (lookup_token(token, "FILL", "MODE",
						 "RECORDSIZE", "RECORD_SIZE", "NOSYMBOL",
						 "WORDSIZE", "WORD_SIZE", "NOPAD",
						 "EVEN_HALF", "ODD_HALF", 0))
	{
	case 0: /* FILL */
		if ( valbad('x') )
			return 1;
		gpfp->fill_char = (uchar)num;
		gpfp->flags |= GPF_M_FILL;
		if ( input_parse && (outgpf.flags & GPF_M_FILL) )
		{
			if ( outgpf.fill_char == num )
				return 0;   /* they match */
			return wasbad("Value conflicts with OUTPUT");
		}
		break;

	case 1: /* MODE */
		if ( *ttp == ':' || *ttp == '=' )
			ttp = sig(++ttp); /* move to value */
		else
			return wasbad("Value required on qualifier");

		if ( subscanf(ttp, SYMNAM, token) != 1 )
			return isbad("Invalid keyword");

		ttp = sig(ttp + strlen(token)); /* eat keyword */

		switch (lookup_token(token, "WORD", "BYTE", 0))
		{
		case 0:
			/* WORD */  gpfp->flags |= GPF_M_WORD;
			break;
		case 1:
			/* BYTE */  gpfp->flags &= ~GPF_M_WORD;
			break;
		default:
			return isbad("Unrecognized MODE keyword");
		}

		if ( !input_parse && (outgpf.flags & GPF_M_MAU) )
			return isbad("MODE and WORD_SIZE cannot both be specified in OUTPUT commands");

		gpfp->flags |= GPF_M_MODESPEC;
		if ( input_parse && (outgpf.flags & GPF_M_MODESPEC) )
		{
			if ( (outgpf.flags & GPF_M_WORD) == (gpfp->flags & GPF_M_WORD) )
				return 0;   /* they match */
			return wasbad("Value conflicts with OUTPUT");
		}
		break;

	case 2: /* RECORDSIZE */
	case 3: /* RECORD_SIZE */
		if ( valbad('d') )
			return 1;
		gpfp->rec_size = (ushort)num;
		break;

	case 4: /* NOSYMBOL */
		gpfp->flags &= ~GPF_M_SYMBOL;
		break;

	case 5: /* WORDSIZE */
	case 6: /* WORD_SIZE */
		if ( valbad('d') )
			return 1;
		if ( num & 7 )
			return wasbad("WORD_SIZE must be a multiple of 8");

		if ( !input_parse && (outgpf.flags & GPF_M_MODESPEC) )
			return isbad("MODE and WORD_SIZE cannot both be specified in OUTPUT commands");

		gpfp->flags |= GPF_M_MAU;
		gpfp->bits_per_word = (uchar)num;
		if ( input_parse && (outgpf.flags & GPF_M_MAU) )
		{
			if ( outgpf.bits_per_word == num )
				return 0;   /* they match */
			return wasbad("Value conflicts with OUTPUT");
		}
		break;

	case 7: /* NOPAD */
		gpfp->flags |= GPF_M_NOPAD;
		break;
	case 8: /* EVEN_WORDS */
		gpfp->flags |= GPF_M_EVENW;
		break;
	case 9: /* ODD_WORDS */
		gpfp->flags |= GPF_M_ODDW;
		break;
	default:
		return -1;  /* not recognized */
	}

	return 0;

} /* end outqualbad */


/*==========================================================================*/
static int qualbad(int input_parse, GPF *gpfp)    /* slash already eaten */
{
	int t;

	if ( !(t = typebad(input_parse, gpfp)) || t == 1 )
		return t;           /* 1 is bad, 0 is good */

#if 0
	if ( !input_parse || !inspec[0] ); /* only if no input read yet */
#endif
	if ( !(t = outqualbad(input_parse, gpfp)) || t == 1 )
		return t;       /* 1 is bad, 0 is good */

	switch (lookup_token(token, "ADDRESS", "GROUP", "SWAP",
						 "WORDSIZE", "WORD_SIZE", 0))
	{
	case 0: /* ADDRESS */
		if ( input_parse )
			return addrbad(gpfp);
		break;

	case 1: /* GROUP */
		if ( !input_parse )
			return wasbad("GROUP is only valid as an input qualifier");
		if ( valbad('d') )
			return 1;
		if ( num & 7 )
			return wasbad("GROUP must be a multiple of 8");
		gpfp->group_code = (uchar)(num >> 3);    /* Make it a byte number */
		gpfp->flags |= GPF_M_GROUP;
		break;

	case 2: /* SWAP */
		gpfp->flags |= GPF_M_SWAP;
		break;

	case 3: /* WORDSIZE */
	case 4: /* WORD_SIZE */
		if ( !input_parse )
			return wasbad("WORD_SIZE is only valid as an input qualifier");
		if ( valbad('d') )
			return 1;
		if ( num & 7 )
			return wasbad("WORD_SIZE must be a multiple of 8");

		gpfp->flags |= GPF_M_MAU;
		gpfp->bits_per_word = (uchar)num;
		if ( input_parse && (outgpf.flags & GPF_M_MAU) )
		{
			if ( outgpf.bits_per_word == num )
				return 0;   /* they match */
			return wasbad("Value conflicts with OUTPUT");
		}
		break;

	default:
		return wasbad("Unrecognized qualifier");
	}
	return 0;
} /* end qualbad */


/*==========================================================================*/
int ioparsebad(int input_parse, GPF *gpfp)
{
	/* logical flags for this pass only 	 */
	int gotspec = 0;

	while ( 1 )
	{                           /* we exit only with a return 			 */
		if ( *ttp == '\0' || *ttp == '!' )
		{                       /* parse complete 						 */
			if ( gotspec == 1 )
				return 0; /* 'twerked fine 					 */
			return wasbad("Need a filename");
		}
		if ( gotspec == 0 )
		{
			if ( subscanf(ttp, FILESPEC, filespec) != 1 )
				return isbad("Unrecognized command; invalid character(s)");
			gotspec++;
			ttp = sig(ttp + strlen(filespec)); /* move past filename			*/
			continue;
		}
		if ( strchr(QUALIFIER, *ttp) )
		{                   /* option found 						 */
			ttp++;                  /* skip the opt delimiter */
			if ( subscanf(ttp, SYMNAM, token) != 1 )
				return isbad("Invalid qualifier");

			ttp = sig(ttp + strlen(token)); /* move past name 			 */
			if ( qualbad(input_parse, gpfp) )
				return 1; /* already told 	 */
			continue;
		}
		else
			return isbad("Too many parameters (only one filename allowed)");
	}
} /* end ioparsebad */
