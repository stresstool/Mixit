/* PORT.H -- header file for portable programs */
#ifndef PORT_H
#define PORT_H

#include <stdlib.h>     /* so we know the types of all the functions */

/* exit status as in exit(ERROR); */
#ifdef vms
#define WARN  0x10000000
#define HAPPY 0x10000001
#define ERROR 0x10000002
#define INFO  0x10000003
#define FATAL 0x10000004
#else
#define WARN  1
#define HAPPY 0
#define ERROR 1
#define INFO  0
#define FATAL 1
#endif

/* for portable editable files */
#ifdef vms
#define RFM_VAR ,"rfm=var"
#define RAT_CR  ,"rat=cr"
#else
#define RFM_VAR
#define RAT_CR
#endif

#define TRUE 1  /* always works */
#define FALSE 0 /* type won't matter */

#if 1   /* to disable voids */
#define fnvoid int  /* for now... */
#define argvoid     /* literally no args */
#else
#define fnvoid void
#define argvoid void
#endif

#ifdef vms
#define byte char
#define word short
#else
#ifndef BYTE_DEFINED
typedef char byte;
#define BYTE_DEFINED
#endif
#ifndef WORD_DEFINED
typedef short word;
#define WORD_DEFINED
#endif
#endif

#ifndef BUFSIZ
#define BUFSIZ	512
#endif

typedef char BUF[BUFSIZ];

#if 0
extern int eprintf( const char *format, ... );
extern int eprintl( const char *format, ... );
#define OLDBUG( text ) { eprintl( "\n%s %d\nin %s,\n(compiled %s, %s):", \
    "Impossible condition detected near line", __LINE__, __FILE__, \
    __TIME__, __DATE__); eprintl( text ); exit( FATAL ); }
#define BUG { eprintl( "\n%s %d\nin %s,\n(compiled %s, %s):", \
    "Impossible condition detected near line", __LINE__, __FILE__, \
    __TIME__, __DATE__); eprintl(
#define GUB ); exit( FATAL ); }
#endif

#endif /* PORT_H */
