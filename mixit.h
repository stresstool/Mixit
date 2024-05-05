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
extern FILE *errFile;

#define in(l,m,h)   			( ((l) <= (m))  &&  ((m) <= (h)))
#define byte_of(x) 				( (x) & 0xFF )
#define PUT_BUF( ptr, byte )    ( *ptr++ = (uchar)byte )

#include "image.h"
#include "gpf.h"
#include "formats.h"
#include "hexutl.h"
#include "prototyp.h"

extern char *error2str( int num );
#endif
