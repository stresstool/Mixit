/* lclreadline.c - interface to glib's readline() function */
#include <stdio.h>

#ifdef THINK_C
	#include <unix.h>
#endif

#include "mixit.h"
#include "port.h"
#include "qa.h"
#include "lclreadline.h"

#if USE_READLINE
	#include <readline/readline.h>
	#include <readline/history.h>
	#ifndef READLINE_HISTORY_FILENAME
		#define READLINE_HISTORY_FILENAME ".mixit_history"	/* Filename of readline history file */
	#endif
	#ifndef READLINE_HISTORY_LINES
		#define READLINE_HISTORY_LINES (256)				/* maximum number of lines maintained in history file */
	#endif
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/stat.h>
#endif

#if USE_READLINE && defined(READLINE_HISTORY_FILENAME) && READLINE_HISTORY_LINES
static void *beenInited;
#if MSYS2 || MINGW
	#define MKDIR(a,b) mkdir(a)
	#define OPEN_FLAGS O_RDONLY|O_BINARY
#else
	#define MKDIR(a,b) mkdir(a,b)
	#define OPEN_FLAGS O_RDONLY
#endif
static void add_to_history_file(char *ptr)
{
	FILE *ofp = fopen(READLINE_HISTORY_FILENAME, "ab");
	if ( ofp )
	{
		fprintf(ofp, "%s\n", ptr);
		fclose(ofp);
	}
}
static int getHistoryFromFile(char ***linesPtr, char **buffPtr)
{
	struct stat st;
	int sts,index = 0;
	*linesPtr = NULL;
	*buffPtr = NULL;
	/* Start with details of history file */
	sts = stat(READLINE_HISTORY_FILENAME, &st);
	if ( sts )
	{
/*		perror("Unable to stat " READLINE_HISTORY_FILENAME ":"); */
		return 0;
	}
	if ( st.st_size > 0 )
	{
		char **lines, *buff;
		/* Get a place to drop pointers into pre-initialised to NULL */
		lines = (char **)calloc(READLINE_HISTORY_LINES, sizeof(char *));
		/* Get a place to read the entire history file into */
		buff = (char *)malloc(st.st_size + 1);
		if ( buff && lines )
		{
			int ifd;
			/* Open the history file */
			ifd = open(READLINE_HISTORY_FILENAME, OPEN_FLAGS);
			if ( ifd >= 0 )
			{
				/* Read the entire history file */
				sts = read(ifd, buff, st.st_size);
				if ( sts > 0 )
				{
					char *linePtr = buff;
					/* make sure there's a nul at the end of the buffer */
					buff[sts] = 0;
					/* loop through the text isolating and saving pointers to line starts */
					while ( linePtr < buff + st.st_size && *linePtr )
					{
						char *lineEnd;

						/* save the line start */
						lines[index] = linePtr;
						/* advance the line index */
						++index;
						/* wrap if necessary */
						if ( index >= READLINE_HISTORY_LINES )
							index = 0;
						/* find the end of the line */
						lineEnd = strchr(linePtr, '\n');
						if ( !lineEnd )
							break;  /* No end of line, so we're done */
						/* replace the newline with a nul */
						*lineEnd = 0;
						/* point to next line */
						linePtr = lineEnd + 1;
					}
					close(sts);
					*linesPtr = lines;
					*buffPtr = buff;
				}
			}
		}
	}
	return index;
}
static void preLoadHistory(void)
{
	char **lines, *buff, *linePtr;
	int ii,index;
	
	index = getHistoryFromFile(&lines,&buff);
	/* Loop through the saved line pointers and put them in the history */
	/* Although we start by putting the lines into history earliest to latest */
	if ( lines && buff )
	{
		for ( ii = 0; ii < READLINE_HISTORY_LINES; ++ii )
		{
			linePtr = lines[index];
			if ( linePtr )
				add_history(linePtr);
			++index;
			if ( index >= READLINE_HISTORY_LINES )
				index = 0;
		}
	}
	if ( lines )
		free(lines);
	if ( buff )
		free(buff);
}

int lclReadLine(void *fin, const char *prompt, char *buf, int bufsiz)
{
	char *ptr;
#if defined(READLINE_HISTORY_FILENAME) && READLINE_HISTORY_LINES
	if ( !beenInited )
	{
		preLoadHistory();
		beenInited = fin;
	}
#endif
	ptr = readline(prompt);
	if ( ptr )
	{
		char *tptr;
		tptr = ptr;
		while ( isspace(*tptr) )
			++tptr;
		if ( *tptr )
		{
			add_history(ptr);
#if defined(READLINE_HISTORY_FILENAME) && READLINE_HISTORY_LINES
			add_to_history_file(ptr);
#endif
		}
		_qacnt_ = strlen(tptr);        /* find the size 					 */
		strncpy(buf, tptr, bufsiz);
		free(ptr);
		if ( _qacnt_ > 0 )
		{
			if ( buf[_qacnt_ - 1] != '\n' )
			{
				buf[_qacnt_] = '\n';
				++_qacnt_;
				return (_qaval_ = 0);
			}
		}
		return (_qaval_ = (_qacnt_ + 1));
	}
	return (_qaval_ = EOF);           /* end of file with no other chars 	 */
}

#endif	/* READLINE_HISTORY_xxx */

void lclPurgeReadLineHistory(void)
{
#if USE_READLINE && defined(READLINE_HISTORY_FILENAME) && READLINE_HISTORY_LINES
	if ( beenInited )
	{
		char **lines, *buff, *linePtr;
		int ii,index;
	
		index = getHistoryFromFile(&lines,&buff);
		/* Loop through the saved line pointers and put them in the history */
		/* Although we start by putting the lines into history earliest to latest */
		if ( lines && buff )
		{
			FILE *ofp=NULL;
			for ( ii = 0; ii < READLINE_HISTORY_LINES; ++ii )
			{
				linePtr = lines[index];
				if ( linePtr )
				{
					if ( !ofp )
					{
						ofp = fopen(READLINE_HISTORY_FILENAME,"w");
						if ( !ofp )
							break;
					}
					fprintf(ofp,"%s\n",linePtr);
				}
				++index;
				if ( index >= READLINE_HISTORY_LINES )
					index = 0;
			}
			if ( ofp )
				fclose(ofp);
		}
		if ( lines )
			free(lines);
		if ( buff )
			free(buff);
	}
#endif	/* READLINE_HISTORY_xxx */
}


