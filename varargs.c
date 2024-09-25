#if 0
/*=============================================================================
VARARGS - general purpose routines for displaying messages and getting
	input values.  The routines provided all allow variable numbers
	of parameters, just like printf.
  =============================================================================*/

void err_exit( char *fmt, ... ); 			/* prints error and exits the program.*/
void ev_exit( int eval, char *fmt, ... ); 	/* prints error and exits with code eval.*/
void perr_exit( char *fmt, ... ); 			/* prints perror and exits the program.*/
int  err_ret( int ret, char *fmt, ... ); 	/* prints error and returns ret. */
int  perr_ret( int ret, char *fmt, ... ); 	/* prints perror and returns ret.*/
void moan( char *fmt, ... ); 				/* prints an error message. 	 */
void warn( char *fmt, ... ); 				/* prints a warning message. 	 */
void info( char *fmt, ... ); 				/* Prints an info message to errFile.*/
void DBUG( char *fmt, ... ); 				/* prints a debug message. 		 */
void DBUGL( int lvl, char *fmt, ... ); 	/* prints a debug msg if debug > lvl.*/
int  getstr( char *result, char *fmt, ... );/* prompts and reads a string.  */
 											/* prompts and gets an int in [lo,hi],*/
 											/*   or uses def for default value.*/
int  get_value(int *val, int def,int lo, int hi,char *fmt, ...);
int  lookup_token(char *token, int count, ... );
void strucpy( char *d, char *s ); 			/* Copies s to d converted to upper case.*/
void uppercase(char *s); 					/* Converts s to upper case in place.*/

=============================================================================*/
#endif

#include "mixit.h"
#include <stdarg.h>

/*==========================================================================*
 | err_exit -   Prints out the error message described by fmt then exits with
 |      error code -1 to indicate an error.
 *==========================================================================*/
void err_exit(char *fmt, ...)
{
	va_list ap;

	fprintf( errFile, "ERROR: " );
	va_start( ap, fmt );
	vfprintf( errFile, fmt, ap );
	va_end(ap);
	fprintf( errFile, "\n" );
	exit(-1);

} /* end err_exit */
#if 0
/*==========================================================================*
 | ev_exit -    Prints out the error message described by fmt then exits with
 |      error code -1 to indicate an error.
 *==========================================================================*/
void ev_exit(int eval, char *fmt, ...)
{
	va_list ap;

	fprintf( errFile, "\nERROR: " );
	va_start( ap, fmt );
	vfprintf( errFile, fmt, ap );
	va_end(ap);
	fprintf( errFile, "\n \n" );
	exit(eval);

} /* end ev_exit */

/*==========================================================================*
 | perr_exit -  Prints out the perror message described by fmt then exits with
 |      error code -1 to indicate an error.
 *==========================================================================*/
void perr_exit(char *fmt, ...)
{
	va_list ap;
	char    string[128];

	va_start( ap, fmt );
	vsprintf( string, fmt, ap );
	va_end(ap);
	perror( string );
	exit(-1);

} /* end perr_exit */
#endif

/*==========================================================================*
 | err_return -    Prints out the error message described by fmt then returns
 |      'ret'.
 *==========================================================================*/
int err_return(int ret, char *fmt, ...)
{
	va_list ap;

	fprintf( errFile, "ERROR: " );
	va_start( ap, fmt );
	vfprintf( errFile, fmt, ap );
	va_end(ap);
	fprintf( errFile, "\n" );
	return ret;
} /* end err_return */

/*==========================================================================*
 | perr_return - Prints out the perror message described by fmt then returns
 |      'ret'.
 *==========================================================================*/
int perr_return(int ret, char *fmt, ...)
{
	va_list ap;
	char    string[128];

	va_start( ap, fmt );
	vsprintf( string, fmt, ap );
	va_end(ap);
	perror( string );
	return ret;
} /* end perr_return */

/*==========================================================================*
 | moan -   Prints out the error message described by fmt, then returns.
 *==========================================================================*/
void moan(char *fmt, ...)
{
	va_list ap;

	if ( whatWeAreDoing == DOING_IN_CMD && inspec[0] )
		fprintf(errFile, "%d:%s\n", ingpf.recordNumber, inspec);
	else if ( whatWeAreDoing == DOING_OUTPUT && outspec[0] )
		fprintf(errFile, "%d:%s\n", outgpf.recordNumber, outspec);
	fprintf(errFile, "ERROR: ");
	va_start( ap, fmt );
	vfprintf( errFile, fmt, ap );
	va_end(ap);
	fprintf( errFile, "\n" );
	fflush( errFile );

} /* end moan */

#if 0
/*==========================================================================*
 | warn -   Prints out the warning message described by fmt, then returns.
 *==========================================================================*/
void warn(char *fmt, ...)
{
	va_list ap;

	fprintf( errFile, "\nWARNING: " );
	va_start( ap, fmt );
	vfprintf( errFile, fmt, ap );
	va_end(ap);
	fprintf( errFile, "\n \n" );
	fflush( errFile );

} /* end warn */

/*==========================================================================*
 | info -   Prints out the error message described by fmt, then returns.
 *==========================================================================*/
void info(char *fmt, ...)
{
	va_list ap;

	va_start( ap, fmt );
	vfprintf( errFile, fmt, ap );
	va_end(ap);
	fprintf( errFile, "\n" );
	fflush( errFile );

} /* end info */

/*==========================================================================*
 | DBUG -   Prints a debug message if the debug flag is active.
 *==========================================================================*/
void DBUG(char *fmt, ...)
{
	va_list ap;
	extern int debug;

	if (!debug) return;

	fprintf( errFile, "\nDEBUG: " );
	va_start( ap, fmt );
	vfprintf( errFile, fmt, ap );
	va_end(ap);
	fflush( errFile );

} /* end DBUG */

/*==========================================================================*
 | DBUGL -  Prints a debug message if the debug flag > lvl.
 *==========================================================================*/
void DBUGL(int lvl, char *fmt, ...)
{
	va_list ap;
	extern int debug;

	if (debug <= lvl) return;

	va_start( ap, fmt );
	vfprintf( errFile, fmt, ap );
	va_end(ap);
	fflush( errFile );

} /* end DBUGL */

/*==========================================================================*
 | getstr - Prints a prompt string based on fmt, then gets a string
 |      response.  Returns length of response string.
 *==========================================================================*/
int getstr(char *result, char *fmt, ...)
{
	va_list ap;

	printf( "ENTER: " );
	va_start( ap, fmt );
	vprintf( fmt, ap );
	va_end(ap);
	printf( ": ");
	gets( result );
	return ( strlen(result) );
} /* end getstr */

/*==========================================================================*
 | get_value -  Prints a prompt string based on fmt, then gets an integer
 |      response.  The response must be within oklo and okhi, or an
 |      error message is printed.  If no value is entered, def is
 |      used for value.  Returns 1 if (oklo <= value <= okhi), else
 |      returns 0.
 *==========================================================================*/
int get_value(int *value, int def, int oklo, int okhi, char *fmt, ...)
{
	va_list ap;
	extern int verbose;
	char result[132];

	printf( "ENTER: " );
	va_start( ap, fmt );
	vprintf( fmt, ap );
	va_end(ap);
	printf( " (%d-%d) : ", oklo, okhi );
	gets( result );
	if (!verbose)
		puts(result);
	if ( !strlen(result) )
		{
		*value = def;
		return 1;
		}
	sscanf( result, "%d", value );
	if ( in(oklo, *value, okhi) )
		return 1;
	moan( "value must range from %d thru %d", oklo, okhi );
	return 0;
} /* end get_value */

/*==========================================================================*
 * strucpy - Copies string s to string d, converting s to uppercase in d.
 *==========================================================================*/
void strucpy( register char *d, register char *s )
{
	if (!s || !d) return;
	do  
		*d++ = toupper( *s );
	while (*s++);
} /* end strucpy */
#endif

/*============================================================================*
 * uppercase - Converts string s to upper case in place.
 *============================================================================*/
void uppercase(char *s)
{
	if (!s || !*s) return;
	do 
		*s = toupper( *s );
	while (*++s);
} /* end uppercase */

/*============================================================================*
 * lookup_token - Searches for a token in a list of tokens and returns the
 *		  index of the matching token.  Token will match any unique
 *		  abbreviation of the check list, and will also match if
 *		  an exact match occurs, even if longer nonunique versions of
 *		  the token are present in the list.
 *	token	- the token to look up in the list.
 *	...	- list of tokens to check against, last entry must be 0.
 *	returns - index of matching token (0 is first entry in list), 
 *		  or -1 if no match or multiple matches occur.
 *============================================================================*/
int  lookup_token(char *token, ... )
{
	va_list	ap;
	int		maxlen = strlen(token);
	int		matches, count;
	int		index  = -1;
	char	*list;

	va_start( ap, token );

  uppercase( token );

	for( matches = count = 0; (list = va_arg(ap, char *)) != 0; ++count )
		{
                        if ( !strncmp(token, list, maxlen) )
  		        {
  			       if (!matches++)		/* Multiple matches */
  			               index = count;
  			       else
  			       {
                                       index = -1;
                                       break;
                               }
  		        }
		}
	va_end(ap);
	return index;
} /* end lookup_token */
