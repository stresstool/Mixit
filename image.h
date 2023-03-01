#if 0 
===========================================================================

I M A G E . H


	Copyright 1989 Atari Games.  All rights reserved.
	Author: Lyle Rains
	===========================================================================
#endif

#ifndef IMAGE_DEFINED
#define IMAGE_DEFINED 1

#include "mixit.h"

#if 0
# define SHOW(x) x
#else
# define SHOW(x)
#endif

#ifndef min
# define min(a,b) ((a) <= (b) ? (a) : (b))
#endif

	/*
	 * Exported objects:
	 *
	 *  DEFAULT_PAGESIZE, LogicalAddr, BlockGroup, MemBlock, Chunk, Page, Image,
	 *  imageWrite(), imageRead(), imageFree(), imageDump(), imageCheck()
	 */

#define DEFAULT_PAGESIZE 0x4000

typedef struct blockgroup 
{
	struct blockgroup *next;
} BlockGroup;


/*  MemBlock is the structure used by both Pages and Chunks. */

typedef struct memblock 
{
	struct memblock	*next;           /* CAUTION: see KLUDGE WARNING below!! */
	LogicalAddr 	begin;
	LogicalAddr 	end;
	struct memblock *chunkList;      /* only used on `Page's. */ 
	char        	*data;
} MemBlock, Chunk, Page;


typedef struct image    
{
	BlockGroup  *blockGroupList;
	MemBlock    *freeBlockList;
	Page        *pageList;
	Page        *symbolList;
	size_t      pageSize;
	Page        **oldPageLink;
	Chunk       **oldChunkLink;
	uchar       skipBytes;
	uchar       bytesPerAddr;
	uchar       fillChar;
} Image;


typedef struct inrecord 
{
	LogicalAddr recSAddr;	/* Start address of segment (inclusive) */
	LogicalAddr	recEAddr;	/* End address of segment (inclusive) */
	LogicalAddr recSegBase; /* segment offset for intel format (always 0 otherwise) */
/*	int			beenConverted; */
	uchar       *recBuf;
	size_t      recBufLen;
	uchar       *recData;
	size_t      recLen;
	size_t		recConvertedLen;
	int    	 	recType;
	FILE        *recFile;
	void        *recPrivate;  /* so the GetRec() routines can save some state info */
} InRecord;

#define REC_XFER	 15
#define REC_TRANSPARENT  13
#define REC_DATA         0
#define REC_EOF         -1
#define REC_ERR         -2
#define REC_UNKNOWN     -3

#endif /* IMAGE_DEFINED */
