/* QA.C -- Question & Answer routine for stdout and stdin */

/*  SPECIAL CASES ARE SITE SPECIFIC     */
/*      That is to say whether you get data on a read with eof/error. */

int _qaval_;    /* global cell for returned value (guaranteed to match) */
int _qacnt_;    /* global cell for number of characters read */

#include <stdio.h>

#ifdef THINK_C
#include <unix.h>
#endif

#include "mixit.h"
#include "port.h"

int _qaflg_ = 1;    /* flag for these routines to bypass prompt */

#if !defined(fileno)
extern int fileno();	/* non-ANSI */
#endif

/*==========================================================================*/
/* returns 0 if a normal read occurs (at least '\n', and '\n' at end). 	 */
/* returns the number of characters read, plus one, for other valid reads.  */
/*  These could be caused by a line too long for your buffer, 				 */
/*  end of file, or other I/O errors. 										 */
/* returns EOF for end of file with no other characters. 					 */
/* returns (EOF - 1) for any other detected errors (in and/or out) 		 */
/*==========================================================================*/
int qa5( FILE *fin, FILE *fout, char *prompt, int bufsiz, char *buf ) 
{
	_qaval_ = ( EOF ) - 1; 				/* assume death 					 */
	_qacnt_ = 0; 						/* in case we die anytime soon 		 */

	if ( _qaflg_ && isatty ( fileno( fout ) ) && isatty ( fileno( fin ) ) )
		{
 										/* this only happens if we should & all I/O is to a tty*/
		clearerr( fout ); 				/* start with no errors (ha ha) 	 */
		fflush( fout ); 				/* shouldn't be necessary;  doesn't hurt*/
		if ( ferror( fout ) )
			return _qaval_; 			/* arggh! 							 */
		fputs( prompt, fout ); 			/* present prompt 					 */
		if ( ferror( fout ) )
			return _qaval_; 			/* arggh! 							 */
		fflush( fout ); 				/* shouldn't be necessary;  doesn't hurt*/
		if ( ferror( fout ) )
			return _qaval_; 			/* arggh! 							 */
		}

	clearerr( fin );
	if ( fgets( buf, bufsiz, fin ) ) 
		{ 								/* get the answer 					 */
		_qacnt_ = strlen( buf ); 		/* find the size 					 */
		if ( ferror( fin ) )
			return _qaval_; 			/* arggh! 							 */
#if 0
		if ( feof( fin ) )
			return ( _qaval_ = EOF ); 	/* oops! 							 */
#endif
		if ( _qacnt_ > 0 )
			if ( buf[_qacnt_ - 1] == '\n' )
				return ( _qaval_ = 0 );
		return ( _qaval_ = ( _qacnt_ + 1 ) );
		}

	if ( ferror( fin ) )
		return _qaval_;					/* arggh! 							 */

	return ( _qaval_ = EOF ); 			/* end of file with no other chars 	 */
} /* end qa5 */


/*==========================================================================*/
/* error recovery for QA and QA5; flush the input buffer */
/*==========================================================================*/
void purge_qa2( FILE *fin, FILE *fout )
{
	char mybuf[256]; 					/* a place to eat the garbage 		 */

	_qaflg_ = 0; 						/* turn off prompting entirely 		 */
	while ( ( _qaval_ > 0 ) && ( _qaval_ < EOF ) )
		qa5( fin, fout, "", 256, mybuf );/* get some text 					 */

	_qaflg_ = 1; 						/* turn prompting back on 			 */
} /* end purge_qa2 */
