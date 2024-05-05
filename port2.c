/* PORT.C -- routines that are (probably or definitely) system specific */

#include "mixit.h"
#include "port.h"

/*==========================================================================*
 |  VAX C sscanf() is deficient; this routine is specific to that 
 |  weakness.  Specifically, it expects a percent sign, a number,
 |  and a scanset in its format string.  All other uses are void.
 |  It does not even believe in white space, nor multiple items.
 |  It does believe in ranges, close brackets, and terminator sets.
 *==========================================================================*/
int subscanf( char *source, char *format, char *out )
{
	char *fp = format; 					/* working pointer 					 */
	char *sp = source; 					/* sameo sameo 						 */
	char *op = out; 					/* more of the same 				 */

	int len; 							/* for number between percent and bracket*/
	char c, lastc = '\0'; 				/* work cells 						 */
	int i; 								/* more of those 					 */

	char array[256]; 					/* to decide on each char we encounter*/
	char seen; 							/* what to stuff in array[] if we find a char*/
	char err[] = "Bad argument to subscanf:";


										/* first we build a table to decide what to keep & toss*/

	if (*fp++ != '%') 					/* was exit(FATAL) 					 */
		err_exit( "%s '%s' has no percent", err, format );

	if (!isdigit(*fp))
		err_exit( "%s '%s' has no number", err, format );

	if ( (sscanf(fp, "%d%c", &len, &c) != 2) || c != '[' )
		err_exit( "%s '%s' found no bracket", err, format );

	if ( len <= 0 )
		err_exit( "%s '%s' too short an output", err, format );

	while ( *fp++ != '[' ) ; 			/* skip to the set 					 */

	if ( *fp == '^' )
		{
		fp++;
		for (i = 0; i < 256; i++)
			array[i] = 1; 				/* keep 'em all 					 */
		seen = 0;
		}
	else
		{
		for (i = 0; i < 256; i++)
			array[i] = 0; 				/* toss 'em all 					 */
		seen = 1;
		}
	array[0] = 0; 						/* always stop on string terminator  */

	if ( *fp == ']' ) 					/* special case for first char 		 */
		array[(int)(*fp++)] = seen;

	if ( *fp == '-' ) 					/* (yet) another special case 		 */
		array[(int)(*fp++)] = seen;

	while ((c = *fp++) != 0)
		{ 								/* while there are characters to check*/
		switch (c)
			{
			case ']': 					/* done at last 					 */
				if (*fp)
					err_exit( "%s '%s' is too long", err, format );
				lastc = c;
				break; 					/* table is finally built 			 */
			case '-': 					/* specifies a range 				 */
				if (! lastc)
					err_exit( "%s '%s' range starts in middle?", err, format );
				if (*fp <= lastc) 		/* 0 always will be 				 */
					err_exit( "%s '%s' range goes wrong way", err, format );
				for ( lastc++ ; lastc < *fp; lastc++)/* all but last 		 */
					array[(int)lastc] = seen;
				array[(int)(*fp++)] = seen; 	/* last one too 					 */
				lastc = '\0'; 			/* break the range next time 		 */
				break;
			default:
				array[(int)(lastc = c)] = seen;
				break;
			} 							/* end of switch 					 */
		} 								/* end of while 					 */
	if ( lastc != ']' )
		err_exit( "%s '%s' has no closing bracket", err, format );

										/* table is finally built, time to look at source string*/
	if (!array[(int)(*sp)]) return 0; 			/* all that, and no match 			 */
	for( i=0; array[(int)(*sp)] && i < len; )
		*op++ = *sp++;
	*op = '\0';
	return 1; 							/* we can only find one item 		 */
} /* end subscanf */

#if 0
/*==========================================================================*/
int eprintf( const char *format, ... ) 
{
	va_list ap; 						/* argument pointer 				 */
	int val; 							/* return value 					 */
	FILE *fp; 							/* channel to use 					 */

	fp = isatty( fileno( stdout ) ) ? stdout : errFile;

	va_start(ap, format); 				/* set up the argument pointer 		 */

	val = vfprintf( fp, format, ap );

	va_end(ap); 						/* close up shop 					 */
	return val; 						/* last bit of emulation 			 */
} /* end eprintf */

/*==========================================================================*/
int eprintl( const char *format, ... ) 
{
	va_list ap; 						/* argument pointer 				 */
	int val; 							/* return value 					 */
	FILE *fp; 							/* channel to use 					 */

	fp = isatty( fileno( stdout ) ) ? stdout : errFile;

	va_start(ap, format); 				/* set up the argument pointer 		 */

	val = vfprintf( fp, format, ap );
	val += fprintf( fp, "\n" );

	va_end(ap); 						/* close up shop 					 */
	return val; 						/* last bit of emulation 			 */
} /* end eprintl */
#endif
