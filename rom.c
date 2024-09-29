#if 0
/*===========================================================================

R O M . C

	This is a file format with the following format:

	command_line        := [command] ";" comment
	command             := base_command | set_command | data_command
	/Future enhancement      | wordsize_command | byte_order_command

	base_command        := ("BASE=" | "BSE=")  hex_value
	set_command         := "SET"
	wordsize_command    := "WORDSIZE=" hex_value
	byte_order_command  := "BIGENDIAN" | "LITTLEENDIAN"

	data_command        := address_expression "=" data_expression

	address_expession   := start_address [":" end_address] ["," increment]
	data_expression     := wild_card_string ["," data_expression]

	start_address       := wild_card_string
	end_address         := hex_value | binary_value
	increment           := hex_value

	wild_card_string    := wild_hex_string | wild_binary_string

	wild_hex_string     := wild_hex_digit [wild_hex_string]
	wild_binary_string  := "'" wild_binary_data

	wild_binary_data    := wild_binary_digit [wild_binary_data]

	hex_value           := hex_digit [hex_value]
	binary_string       := binary_digit [binary_string]
	binary_value        := "'" binary_string

	wild_binary_digit   := binary_digit | "X"
	wild_hex_digit      := hex_digit | "X"

	binary_digit        := 0|1
	hex_digit           := 0|1|2|3|4|5|6|7|8|9|A|B|C|D|E|F


	What does this all mean?  Mostly the file has data_commands.  They
	define how memory should be filled.  Data gets put in memory starting
	at start_address, and optionally duplicated through end_address.  If
	increment is specified, it defines how much to bump the address between
	data items.  Wildcards allow huge ranges of data to be written with
	only a few commands.

	Data_expression defines a list of data items to put into memory according
	to the address expression.  Wildcards here specify 'do not touch' areas
	that are not written over.  A data_expression that is shorter than the
	address_expression range will be repeated until the range has been filled.

	Spaces and tabs are ignored anywhere in the line, and anything on the
	line after a ';' is considered a comment and stripped out prior to 
	parsing.

	The file DOK:ROMFILES.DOC explains this file format in great detail.


Copyright 1991 Atari Games.  All rights reserved.
Author: Jim Petrick
===========================================================================*/
#endif

#include <time.h>
#include "mixit.h"
#include "version.h"

#define BINARY_MARK     '\''
#define COMMENT_MARK    ';'
#define WILDCARD_MARK   'X'
#define END_ADDR_MARK   ':'
#define INCREMENT_MARK  ','
#define DATA_MARK       ','
#define DATA_START_MARK '='

typedef struct rom_rec
{
	char        start_address[40];
	LogicalAddr end_address;
	ulong       increment;
	char        list[128][35];
	int         count;
	int         radix;
	int         word_size;
	int         big_endian;
}
RomRec;

static RomRec       rom_rec;

extern GPF          ingpf;  /* environment for getfile() / putfile() */
extern LogicalAddr  base_address;
extern char			current_fname[];

static ulong	str_to_ul(char *str, int base);
static int	parse_token(char **str, char *value, const char *ok_chars);
static int	parse_binary(char **str, char *value, int wildcards);
static int	parse_hex(char **str, char *value, int wildcards);
static int	parse_number(char **str, char *value, int wildcards);
static int	parse_command(char *rec, RomRec *rrec);
static void	deposit_data(RomRec *rrec);
static void	deposit_wild_data(RomRec *rrec);

/*==========================================================================
 * Converts a string to a binary value.
 *==========================================================================*/
static ulong str_to_ul(char *str, int base)
{
	ulong   value = 0;

	if ( *str == BINARY_MARK )
		if ( base && base != 2 )
			return err_return(0, "Bad conversion value [%s] base = %d", str, base);
		else
			base = 2;
	else if ( !base )
		base = 16;

	if ( base == 2 )
		while ( *str && (*str == '1' || *str == '0') )
			value = (value << 1) | chartohex[(int)(*str++)];
	else
		while ( *str && chartohex[(int)(*str)] != XX )
			value = (value << 4) | chartohex[(int)(*str++)];

	return value;

}   /* end str_to_ul */


/*==========================================================================
 * Copies a token from the input string **str into 'value'.  The extraction
 * grabs chars fron str until it finds the end of string or any char NOT in
 * 'ok_chars'.  Returns 1 if any chars have been grabbed, else 0.
 *==========================================================================*/
static int parse_token(char **str, char *value, const char *ok_chars)
{
	register char   *s = *str,
		*d = value;
	register int       i;

	for ( i = strspn(s, ok_chars); i > 0; --i )
		*d++ = *s++;

	*str = s;
	*d   = 0;

	return (d > value);

} /* end parse_token */


/*==========================================================================
 * Extracts a binary value from *str into value.  If 'wildcards' is true, 
 * wildcard 'X' characters are allowed, otherwise they are not.  
 *  *str is updated to point to the first non-binary digit found.
 *  Returns 1 for sucess, 0 for failure.
 *==========================================================================*/
static int parse_binary(char **str, char *value, int wildcards)
{
	if ( **str != BINARY_MARK )
		return 0;

	*value++ = *(*str)++;
	return parse_token(str, value, wildcards ? "01Xx" : "01");

} /* end parse_binary */


/*==========================================================================
 * Extracts a hex value from *str into value.  If 'wildcards' is true, 
 * wildcard 'X' characters are allowed, otherwise they are not.  
 *  *str is updated to point to the first non-hex digit found.
 *  Returns 1 for sucess, 0 for failure.
 *==========================================================================*/
static int parse_hex(char **str, char *value, int wildcards)
{
	return parse_token(str, value,
					   wildcards ? "X0123456789ABCDEFabcdefx" : "0123456789ABCDEFabcdef");
} /* end parse_hex */


/*==========================================================================
 * Extracts either a binary or hex value from *str into value.  
 *  If 'wildcards' is true, wildcard 'X' characters are allowed, 
 *      otherwise they are not.  
 *  *str is updated to point to the first non-numeric digit found.
 *  Returns 1 for sucess, 0 for failure.
 *==========================================================================*/
static int parse_number(char **str, char *value, int wildcards)
{
	if ( parse_binary(str, value, wildcards) )
		return 1;

	if ( parse_hex(str, value, wildcards) )
		return 1;

	return 0;

} /* end parse_number */


/*==========================================================================
 * Parses a command line 'rec' into parts. Places start address in a1, 
 * end address in a2, increment in inc, a list of data items in list, and 
 * a the number of items in list in cnt.
 * Returns 1 for sucess, 0 for faliure.
 *==========================================================================*/
static int parse_command(char *rec, RomRec *rrec)
{
	char    *s, *d, *t, *ptr;
	char    hex_str[60];
	int     i;

	if ( !rec )
		return 0;

	/* 
	 *  Remove comments and whitespace and convert the string to
	 *  uppercase.  Skip null command lines.
	 */

	t = 0;
	for ( d = s = rec; *s && *s != COMMENT_MARK; ++s )
		if ( !isspace(*s) )
		{
			if ( !t && !isalnum(*s) )
				t = d;
			*d++ = toupper(*s);
		}
	if ( !t )
		t = d;
	*d = 0;

	*rrec->start_address = 0;
	rrec->end_address	 = 0;
	if ( !*rec )
		return 1;

	/*
	 *  Now determine what type of command we've got here.  
	 */

	i = *t;
	*t = 0;
	switch (lookup_token(rec, "BA", "BASE", "BSE", "SET", "WORDSIZE",
						 "BIGENDIAN", "LITTLEENDIAN", 0))
	{
	case 0:     /* CHECK "BAA", "BAB", "BAC", "BAD", "BAE", "BAF", "BA" */
	default:    /* DATA_COMMAND */
		break;

	case 1:     /* BASE COMMAND */
	case 2:
		s = t + 1;
		if ( i != '=' || !parse_hex(&s, hex_str, 0) )
			return err_return(0, "Illegal BASE statement; should be BASE=hex_value");
		base_address = str_to_ul(hex_str, 16);
		return 1;

	case 3:     /* SET COMMAND */
		return 1;

	case 4:     /* WORDSIZE COMMAND */
		s = t + 1;
		if ( i != '=' || !parse_hex(&s, hex_str, 0) )
			return err_return(0, "Illegal BASE statement; should be BASE=hex_value");
		rom_rec.word_size = str_to_ul(hex_str, 16);
		ingpf.bits_per_word = 8 * rom_rec.word_size;
		return 1;

	case 5:     /* BIGENDIAN COMMAND */
		rom_rec.big_endian = 1;
		return 1;

	case 6:     /* LITTLEENDIAN COMMAND */
		rom_rec.big_endian = 0;
		return 1;
	}

	/*
	 *  If we get here, the line was a data_command.  Extract the start_address
	 */

	ptr = rec;
	if ( !parse_number(&ptr, rrec->start_address, 1) )
		return err_return(0, "Expecting numeric value for start address");
	rrec->radix = (*rrec->start_address == BINARY_MARK) ? 2 : 16;
	*t = i;

	/*
	 *  Now check for an end address.
	 */

	if ( *ptr == END_ADDR_MARK )
	{
		++ptr;
		if ( !parse_number(&ptr, hex_str, 0) )
			return err_return(0, "Expecting numeric value for end address");
		rrec->end_address = str_to_ul(hex_str, 0);
	}
	else
		rrec->end_address = 0;

	/*
	 *  Check for the increment.
	 */

	if ( *ptr == INCREMENT_MARK )
	{
		++ptr;
		if ( !parse_hex(&ptr, hex_str, 0) )
			return err_return(0, "Expecting a hex value for increment");

		rrec->increment = str_to_ul(hex_str, 16);
	}
	else
		rrec->increment = 1;

	/*
	 *  Everything left should be a data expression.  Just check for 
	 *  illegal syntax and count entries.  Copy to the RomRec struct.
	 */

	if ( *ptr++ != DATA_START_MARK )
		return err_return(0, "Expecting '=' between address and data");

	for ( i = 0; *ptr; ++i )
	{
		if ( !parse_number(&ptr, rrec->list[i], 1) )
			return err_return(0, "Bad data expression");
		if ( *ptr && *ptr++ != DATA_MARK )
			return err_return(0, "Expecting ',' between data values");
	}

	if ( !(rrec->count = i) )
		return err_return(0, "No data specified");

	return 1;

} /* end parse_command */


/*==========================================================================*
 * Writes a list of data items to the area starting at start_address.
 *==========================================================================*/
static void deposit_data(RomRec *rrec)
{
	LogicalAddr addrS, addrE;
	LogicalAddr lo, hi;
	long		relocation;
	char        *item;
	uchar       *ptr;
	uchar       buffer[32];
	long        flag;
	ulong       increment;
	int         sIdx, idx, lim, byteCnt;
/*	short       group, group_code, swap; */
	short       bytes_per_word;
	InRecord    in_rec;

	if ( !rrec->count )
		return;

	flag            = ingpf.flags;
	relocation		= 0;
	lo              = (flag & GPF_M_START) ? ingpf.low_limit  :  0;
	hi              = (flag & GPF_M_END) ? ingpf.high_limit : -1;
	if ( (flag & GPF_M_MOVE) )
	{
		relocation = ingpf.out_add;
		if ( (flag&GPF_M_START) )
			relocation -= lo;
	}
	bytes_per_word  = ingpf.bits_per_word / 8;
	in_rec.recData  = buffer;
	in_rec.recLen   = rrec->word_size;
	increment       = rrec->increment;

	if ( flag & GPF_M_MAU )
		increment       *= bytes_per_word;

	/*
	 *  Determine actual address range to fill in.
	 */

	addrS = str_to_ul(rrec->start_address, rrec->radix);
	addrS += base_address;
	addrE = rrec->end_address ? rrec->end_address : (addrS + (rrec->count ? rrec->count-1:0));
	
	if ( hi < addrS )
	{
		if ( debug )
			printf("deposit_data(): Record out of range. addrS=0x%lX, addrE=0x%lX, lo=0x%lX, hi=0x%lX\n", addrS, addrE, lo, hi );
		return;     /* Out of range, skip it */
	}
	if ( lo > addrE )
	{
		if ( debug )
			printf("deposit_data(): Record out of range. addrS=0x%lX, addrE=0x%lX, lo=0x%lX, hi=0x%lX\n", addrS, addrE, lo, hi );
		return;		/* Out of range, skip it */
	}

	if ( addrS < ingpf.low_add )
		ingpf.low_add = addrS;
	if ( addrE > ingpf.high_add )
		ingpf.high_add = addrE;

	if ( debug )
		printf("deposit_data(): Entry before adjustments. addrS=0x%lX, addrE=0x%lX, rrec->count=%d, lo=0x%lX, hi=0x%lX, relocation=0x%lX\n",
			   addrS, addrE, rrec->count, lo, hi, relocation);
	sIdx = 0;
	if ( rrec->end_address )
		byteCnt = (rrec->end_address - addrS + 1);
	else
		byteCnt = rrec->count;
	lim = rrec->count;
	if ( lo > addrS )
	{
		sIdx = lo-addrS;
		byteCnt -= sIdx;
		addrS += sIdx*increment;
	}
	if ( hi < addrE )
	{
		lim -= addrE-hi;
		byteCnt -= addrE-hi;
		addrE = hi;
	}
	if ( debug )
		printf("deposit_data(): Entry after adjustments. addrS=0x%lX, addrE=0x%lX, idx=%d, byteCnt=%d, lim=%d\n",
			   addrS, addrE, sIdx, byteCnt, lim);

	while ( addrS <= addrE )
	{
		/* 
		 *  For each data item in the list, get the existing data, then merge
		 *  in bytes of the replacement data, then write the data back to the
		 *  image structure.
		 */
	
		for ( idx = sIdx; idx < lim && addrS <= addrE; ++idx, ++addrS )
		{
			int j;
	
			/*
			 *  Convert all respective items to binary wildcard masks.
			 */
	
			if ( *(item = rrec->list[idx]) != BINARY_MARK )
			{
				strcpy((char *)buffer, item);
				*item++ = BINARY_MARK;
				for ( ptr = buffer; *ptr; ++ptr )
				{
					if ( *ptr == WILDCARD_MARK )
					{
						*item++ = WILDCARD_MARK;
						*item++ = WILDCARD_MARK;
						*item++ = WILDCARD_MARK;
						*item++ = WILDCARD_MARK;
					}
					else
					{
						uchar hvalue = chartohex[*ptr];
						*item++ = (hvalue & 8) ? '1' : '0';
						*item++ = (hvalue & 4) ? '1' : '0';
						*item++ = (hvalue & 2) ? '1' : '0';
						*item++ = (hvalue & 1) ? '1' : '0';
					}
				}
				*item = 0;
				item  = rrec->list[idx];
			}
	
			/*
			 *  Get the data at the given address.
			 */
	
			if ( debug )
				printf("deposit_data(): calling imageRead(,0x%lX,%d,), addrS=0x%lX, lo=0x%lX, hi=0x%lX, idx=%d, lim=%d, relocation=0x%lX\n",
					   addrS+relocation,
					   rrec->word_size,
					   addrS,
					   lo, hi, idx, lim, relocation);
			imageRead(&ingpf.image, addrS+relocation, rrec->word_size, buffer);
	
			/*
			 *  Merge the data in the buffer with the data in the item list.
			 *  Start at the least significant bit of the item and work to the
			 *  most significant bit, replacing non-wildcard bits of the buffer
			 *  with new data and leaving the wildcard bits alone.  We start at
			 *  the LSB since we don't know the magnitude of the binary value.
			 *  Parsing '000010 and '10 is easier from low order to high order,
			 *  since we know that the last char in the string is the LSB, while
			 *  the first char in the string is an arbitrary bit in the byte.
			 */
	
			item += strlen(item) - 1;
			for ( j = rrec->word_size - 1; j >= 0; --j )
			{
				uchar mask = 1;
				int k, m = rrec->big_endian ? rrec->word_size - j - 1 : j;
				for ( k = 0; k < 8; ++k )
				{
					if ( *item == BINARY_MARK )
						break;
					if ( *item != WILDCARD_MARK )
						buffer[m] = (buffer[m] & ~mask) | ((*item == '1') ?
														   mask : 0);
					mask <<= 1;
					--item;
				}
				if ( *item == BINARY_MARK )
					break;
			}
	
			/*
			 *  Write the data back to the image structure.
			 */
	#if 1
	/*			save_data(&in_rec, map_addr, &ingpf); */
			in_rec.recSAddr = addrS;
			in_rec.recEAddr = addrE;
			if ( debug )
				printf("deposit_data(): calling save_data(), lo=0x%lX, hi=0x%lX, addrS=0x%lX, addrE=0x%lX, byteCnt=%d, E-S=%ld, relocation=0x%lX\n",
					   lo, hi, addrS, addrE, byteCnt, addrE-addrS+1, relocation);
			save_data(&in_rec, lo, hi, relocation, &ingpf);
	#else
			imageWrite( &ingpf.image, map_addr, rrec->word_size, buffer );
	#endif
		}
	}
} /* end deposit_data */



/*==========================================================================*
 * Writes a list of data items to the area starting at start_address.
 *==========================================================================*/
static void deposit_wild_data(RomRec *rrec)
{
	char    *ptr;
	int     handled = 0, i;

	for ( ptr = rrec->start_address; *ptr; ++ptr )
	{
		if ( *ptr == WILDCARD_MARK )
		{
			handled = 1;
			for ( i = 0; i < rrec->radix; ++i )
			{
				*ptr = hex_of[i];
				deposit_wild_data(rrec);
			}
			*ptr = WILDCARD_MARK;
		}
	}

	if ( handled )
		return;
	else
		deposit_data(rrec);

} /* end deposit_data */


/*==========================================================================
 * Read a .ROM file command and parse it.
 *==========================================================================*/
int GetRec_rom(InRecord *rec)
{
	char    recbuf[256];

	/* 
	 *  Check for the initial call to this routine.
	 */

	if ( !rom_rec.word_size )
	{
		if ( !(rom_rec.word_size = ingpf.bits_per_word / 8) )
			ingpf.bits_per_word = (rom_rec.word_size = 1) * 8;
		rom_rec.big_endian  = 1;
	}

	/*
	 *  Get a command from the input file.
	 */

	if ( !fgets(recbuf, rec->recBufLen, rec->recFile) )
	{
		rec->recLen = 0;
		return (rec->recType = feof(rec->recFile) ? REC_EOF : REC_ERR);
	}

	/* 
	 *  Parse the command and check syntax for correctness.
	 */

	if ( !parse_command(recbuf, &rom_rec) )
	{
		rec->recLen = 0;
		return (rec->recType = REC_ERR);
	}
	else if ( !*rom_rec.start_address )
		return (rec->recType = REC_UNKNOWN);

	/*
	 *  Now handle the input command.  It is in the form:
	 *      start_address:end_address,incr = list[count]
	 *
	 *  where: 
	 *      start_address = a wildcard hex or binary string,
	 *      end_address   = a hex value (no wildcards)
	 *      incr          = a hex value (no wildcards)
	 *      list          = a list of data_values (wildcards allowed)
	 *      count         = the number of entries in list
	 */

	deposit_wild_data(&rom_rec);

	return (rec->recType = REC_UNKNOWN);

} /* end GetRec_rom */


/*==========================================================================*
 * Outputs a single record in ROM format.
 *==========================================================================*/
int PutRec_rom(FILE *file, uchar *data, int recsize, ulong recstart)
{
	int j;

	if ( recsize <= 0 )
		return 1;
	/* Output the Address and the 1st byte */
	fprintf(file, "%04lX=%02X", recstart, *data++);
	/* Append data to the record */
	for ( j = 1; j < recsize; ++j )
		fprintf(file, ",%02X", *data++);
	fputs("\n", file);      /* Output the record to the file */
	return 1;

} /* end PutRec_rom */


/*==========================================================================
 * Puts out any header info for the file.
 *==========================================================================*/
int PutHead_rom(FILE *file, ulong addr, ulong hi)
{
	if ( !noDate )
	{
		time_t      aclock = (time_t)addr;
		struct tm   *newtime;
	
		time(&aclock);
		newtime = localtime(&aclock);
		fprintf(file, "; ROM/PROM data file created via Mixit %s %s\n", REVISION, asctime(newtime));
	
		fprintf(file, "; File name = %s\n\n", current_fname);
	}

	return 1;

} /* end PutHead_rom */
