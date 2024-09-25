#ifndef MIXIT_H
#define MIXIT_H

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#if defined(WIN32)
#include <io.h>
#endif
#if LINUX
#include <unistd.h>
#endif

#if IRIX
#include <sys/bsd_types.h>
#else
#  if sun || __i386
#    include <sys/types.h>
#    ifdef __USE_MISC
#      define _TYPEDEF_USHORT (1)
#      define _TYPEDEF_UINT   (1)
#      define _TYPEDEF_ULONG  (1)
#    endif
#    if !_TYPEDEF_ULONG
#      define _TYPEDEF_ULONG (1)
typedef unsigned long		ulong;
#    endif
#    if !_TYPEDEF_USHORT
#      define _TYPEDEF_USHORT (1)
typedef unsigned short		ushort;
#    endif
#    if !_TYPEDEF_UINT
#      define _TYPEDEF_UINT	(1)
typedef unsigned int		uint;
#    endif
#  endif
#endif

#if LINUX
# if !_TYPEDEF_ULONG
#   define _TYPEDEF_ULONG (1)
typedef unsigned long		ulong;
# endif
# if !_TYPEDEF_USHORT
#  define _TYPEDEF_USHORT (1)
typedef unsigned short		ushort;
# endif
# if !_TYPEDEF_UINT
#  define _TYPEDEF_UINT	(1)
typedef unsigned int		uint;
# endif
#endif
typedef unsigned char  		uchar;
typedef unsigned long		LogicalAddr;

extern int noisy;
extern int debug;

#define DOING_UNDEFINED	(0)
#define DOING_IN_CMD	(1)
#define DOING_OUT_CMD	(2)
#define DOING_OUTPUT	(4)
extern int whatWeAreDoing;

extern FILE *errFile;

#define in(l,m,h)   			( ((l) <= (m))  &&  ((m) <= (h)))
#define byte_of(x) 				( (x) & 0xFF )
#define PUT_BUF( ptr, byte )    ( *ptr++ = (uchar)byte )

#include "port.h"
#include "image.h"
#include "gpf.h"
#include "formats.h"
#include "hexutl.h"
#include "prototyp.h"

extern BUF  inspec;                 /* accepted file specs for MIXIT routines*/
extern BUF  outspec;                /* accepted file specs for MIXIT routines*/
extern GPF  outgpf;                 /* environment for getfile() / putfile() */
extern BUF  filespec;               /* filespec we will collect EXACTLY one of*/

extern char *error2str( int num );
#endif
