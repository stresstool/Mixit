#if 0
/*===========================================================================

I M A G E . C

	Routines to manage an Image data structure.

	imageWrite(): merge a data record from an input file into an
	Image, building the Image as required. Includes memory allocation
	routines for the internal Image reperesentation. The record must have
	already been read and parsed into a Record structure.

	imageRead():	get a data block from an Image and put it into a
					Record structure. Used primarily to get data to
					be reformatted for output.
	imageFree():	free all the data memory in an Image.
	imageDump():	(debug tool) do a text dump of an Image to an output file.
	imageCheck():	(debug tool) perform an internal consistency check on Image.

	Copyright 1989 Atari Games.  All rights reserved.
	Author: Lyle Rains

===========================================================================*/
#endif

/*
 * Imported objects, declarations and definitions:
 *      string.h:  NULL, size_t, memcpy(), memset()
 *       stdio.h:  fprintf()
 *      stdlib.h:  EXIT_FAILURE, exit(), malloc(), free()
 *      limits.h:  ULONG_MAX
 *      stddef.h:  offsetof()
 *       image.h:  LogicalAddr, BlockGroup, MemBlock, Chunk, Page, Image
 */

#include "mixit.h"
#include <limits.h>

#ifdef __STDC__
	#include <stddef.h>
#endif
#ifndef offsetof
	#define offsetof(s, m)    (size_t)(&(((s *)0)->m))
#endif

/*
 * Exported objects (#include "image.h"):
 *
 *   void imageWrite(Image *image, Record *record);
 *   void imageRead(Image *image, Record *record);
 *   void imageFree(Image *image);
 *   void imageInit(Image *image, int pagesize );
 *
 * The following are available only when #ifndef NDEBUG
 *
 *   void imageDump(Image *image, FILE *dumpfile);
 *   void imageCheck(Image *image, FILE *logfile);
 */

static MemBlock* allocBlock(Image *image);
static void	freeBlock(Image *image, MemBlock *block);
static Page* newPage(Image *image, LogicalAddr addr, Page **insertlink);
static Chunk* newChunk(Image *image, LogicalAddr addr, size_t nbytes,
					   Chunk **insertlink, Page *page);
static int	init_chunk_read(Chunk *thisChunk);


/*==========================================================================
 * The following macros for pageList and chnkList manipulation are UNSAFE for use
 * with arguments which have side effects.
 *
 * SEARCH_FOR_ADDR() will search the list indicated by `*link' for a block
 * (page or chunk) which includes the logical address `addr'.  The macro uses
 * `block' as an intermediate value.  At the termination of the macro, if such a
 * block exists, `block' will be a pointer to it.  Otherwise, `addr' is in a
 * currently non-existant block which should be linked between `link' and `block'.
 *
 * INSERT_BLOCK() will insert a new block between `link' and `block'.
 *==========================================================================*/
#define SEARCH_FOR_ADDR(link, block, addr) \
  { for ((block) = *(link); (block) && (block)->end < (addr); (block) = (block)->next) \
      (link) = &((block)->next); }

#define INSERT_BLOCK(link, block) \
  ((block)->next = *(link), *(link) = (block))


/*==========================================================================
 * PREV_CHUNK(page, chunklink)
 *
 * If the link is the root chunkList of the page, the value is NULL,
 * else the value is a pointer to the chunk containing the link.
 *==========================================================================*/
#ifdef __STDC__

	#define PREV_CHUNK(page, link) \
    ((Chunk *)((link) == &((page)->chunkList) ? 0 \
    : ((char *)(link) - (size_t)(offsetof(Chunk, next)))))

#else

/*
 * KLUDGE WARNING:  The following macro definition makes use of the fact
 * that the `.next' field is the first field in a struct chunk, so a
 * pointer to the `.next' field is also a pointer to the struct.
 */

	#define PREV_CHUNK(page, link) \
    ((Chunk *)((link) == &((page)->chunkList) ? 0 : (link)))

#endif /* __STDC__ */


/*==========================================================================
 * get_memory(): Allocates 'n' bytes of memory and returns a pointer to the
 *  memory.  If memory can't be allocated, exits the program.
 *==========================================================================*/
static void* get_memory(int n)
{
	void *ptr;

	if ( (ptr = (void *)malloc(n)) == (void *)NULL )
		err_exit("Not enough memory");
	return ptr;
} /* end get_memory */


/*==========================================================================
 * If no blocks are on the free list, make some.  Abort on allocation error.
 *
 * Return: an uninitialized chunk from the free list.
 *==========================================================================*/
static MemBlock* allocBlock(Image *image)
{
	MemBlock    *block;
	BlockGroup  *root;
	int    	 	i;
#define BLOCK_EXTEND 256

	/*
	 *  If we're out of MemBlock structures, make BLOCK_EXTEND more and 
	 *  link them into the free list.
	 */
	if ( !(image->freeBlockList) )
	{
		root = (BlockGroup *)get_memory(sizeof(BlockGroup) + (sizeof(MemBlock) * BLOCK_EXTEND));

		INSERT_BLOCK(&(image->blockGroupList), root);
		block = image->freeBlockList = (MemBlock *)(root + 1);
		for ( i = 0; i < BLOCK_EXTEND - 1; ++i )
		{
			block->next = block + 1;
			++block;
		}
		block->next = (MemBlock *)NULL;
	}

	/* Remove the first chunk from the free list and return it.  */
	block = image->freeBlockList;
	image->freeBlockList = block->next;
	return (block);
} /* end allocBlock */


/*==========================================================================
 * link a chunk into the head of the free list.
 *==========================================================================*/
static void freeBlock(Image *image, MemBlock *block)
{
	INSERT_BLOCK(&image->freeBlockList, block);
} /* end freeBlock */


/*==========================================================================
 * Allocate, initialize, and link in a new Page.  Abort on allocation error.
 *
 * Return: pointer to newly created page.
 *==========================================================================*/
static Page* newPage(Image *image, LogicalAddr addr, Page **insertlink)
{
	Page *newpage;

	newpage = allocBlock(image);

	/*
	 *  Future rev: In a system with limited memory and no virtual memory paging 
	 *  (MSDOS), it would be a good idea to be able to manually handle the 
	 *  following memory pages by swapping to disk (or EMS memory or whatever).
	 *  Not trivial.  Changes would be required here and other places.
	 */

	newpage->data = (char *)get_memory(image->pageSize);

	if ( image->skipBytes )
		memset(newpage->data, image->fillChar, image->pageSize);
	INSERT_BLOCK(insertlink, newpage);
	addr -= addr % image->pageSize;
	newpage->begin = addr;
	newpage->end = addr + image->pageSize - 1;
	if ( newpage->end < newpage->begin )
	{
		/*
		 *  Oops, user specified weird page size, and we overflowed!
		 *  Trim it back so we don't wrap the address space.
		 */
		newpage->end = ULONG_MAX;
	}
	newpage->chunkList = (Chunk *)NULL;
	return (newpage);
} /* end newPage */


/*==========================================================================
 * See if the requested chunk may be combined with either or both of
 * the preceeding and succeeding chunks.  If so, then do the combination
 * and various fixups on the chunk list, otherwise, create, initialize,
 * and link a new chunk into the list.
 *
 * Return: a pointer to the new or combined chunk containing `addr'.
 *==========================================================================*/
static Chunk* newChunk(Image *image, LogicalAddr addr, size_t nbytes,
					   Chunk **insertlink, Page *page)
{
#define PREV 1
#define NEXT 2
	int combine = 0;
	Chunk   *newchunk;
	Chunk   *prevchunk;
	Chunk   *nextchunk;

	/* See if we can combine new chunk with previous chunk. */
	prevchunk = PREV_CHUNK(page, insertlink);
	if ( prevchunk && prevchunk->end + 1 == addr )
		combine |= PREV;

	/* See if we can combine new chunk with next chunk. */
	nextchunk = *insertlink;
	if ( nextchunk && (nextchunk->begin) <= addr + nbytes )
		combine |= NEXT;

	switch (combine)
	{
	default:
		newchunk = allocBlock(image);
		INSERT_BLOCK(insertlink, newchunk);
		newchunk->begin = addr;
		newchunk->end = (nbytes - 1 < page->end - addr)
			? addr + (nbytes - 1)
			: page->end;
		newchunk->data = page->data + (addr - page->begin);
		break;

	case PREV:
		newchunk = prevchunk;
		newchunk->end = (nbytes - 1 < page->end - addr)
			? addr + (nbytes - 1)
			: page->end;
		break;

	case NEXT:
		newchunk = nextchunk;
		newchunk->data -= newchunk->begin - addr;
		newchunk->begin = addr;
		break;

	case PREV | NEXT:
		newchunk = prevchunk;
		newchunk->next = nextchunk->next;
		newchunk->end = nextchunk->end;
		freeBlock(image, nextchunk);
		break;
	}
	return (newchunk);
} /* end newChunk */


/*==========================================================================
 * Initialize an Image structure.
 *==========================================================================*/
void imageInit(Image *image, int pagesize)
{
	image->pageList     	= (Page *)NULL;
	image->symbolList   	= (Page *)NULL;
	image->oldPageLink  	= &image->pageList;
	image->pageSize     	= pagesize;
	image->blockGroupList   = (BlockGroup *)NULL;
	image->freeBlockList    = (MemBlock *)NULL;
	image->skipBytes    	= 0;
	image->fillChar     	= '\0';
} /* end imageInit */


/*==========================================================================
 * Free all the Pages, Chunks, and data memory in the indicated image.
 *==========================================================================*/
void imageFree(Image *image)
{
	void        *mem;
	Page        *page;
	BlockGroup  *blocks;

	page = image->pageList;
	image->pageList = (Page *)NULL;
	while ( page )
	{
		free(page->data);
		page = page->next;
	}
	blocks = image->blockGroupList;
	image->blockGroupList = (BlockGroup *)NULL;
	while ( blocks )
	{
		mem = blocks;
		blocks = blocks->next;
		free(mem);
	}
	image->freeBlockList = (MemBlock *)NULL;
} /* end imageFree */


/*==========================================================================
 * Merge `nbytes' from the `data' buffer into `image' at logical address `addr'.
 *
 * Assumptions:
 *
 *  (1) The caller should have translated the record to only present data which
 *  is in range and should also have translated "word" addressing and length
 *  into appropriately offset "byte" logical addressing and length.
 *
 *  (2) image->pageList points to the first page of a (possibly empty) linked
 *  list ordered by increasing logical address.  Similarly, page->chunkList points
 *  to the first chunk of a (possibly empty) linked list ordered by increasing
 *  logical/physical address within the page.
 *
 *  (3) newChunk() will combine/extend existing chunks as necessary and fix up
 *  the image->chunkList linkage.  It also handles allocation errors -- it won't
 *  return unless memory was properly allocated.  It may return less than the
 *  requested amount, due to page boundaries, but will always allocate a new
 *  chunk of at least one byte (or die trying). Similarly, newPage() creates
 *  a new page:  if it returns, it succeeded.
 *
 *  (4) Assuming that most records will be at monotonically increasing addresses
 *  with limited jumping around, we speed our list searches by maintaining the
 *  previous page and chunk links in static variables in the image structure to
 *  use as the most likely search starting point.  It will still work if this
 *  assumption isn't true, but will be slower.
 *==========================================================================*/
void imageWrite(Image *image, LogicalAddr addr, size_t nbytes, uchar *data)
{
	Page    **pageLink;
	Chunk   **chunkLink;
	Page    *nextPage;
	Chunk   *nextChunk;
	char    *dest;
	size_t  copylen;
	int     skipcnt = 0;

	/*
	 *  Don't bother if there is no data.  If we are skipping bytes 
	 *  (presumably because we are interleaving several byte files into 
	 *  longer word oriented files) then the effective number of bytes 
	 *  to be transferred needs to be scaled up.
	 */
	if ( !nbytes )
	{
		return;
	}
	if ( debug )
	{
		printf("imageWrite(): Saving %4d byte record. RecordAdd=%08lX-%08lX, skipBytes=%d\n",
			   nbytes, addr, addr+nbytes-1, image->skipBytes );
	}
	if ( image->skipBytes )
	{
		nbytes *= image->skipBytes + 1;
		nbytes -= image->skipBytes; /* trim trailing skipped bytes */
	}

	/* Initialize the page link if NULL or if addr is below last page */
	pageLink  = image->oldPageLink;
	chunkLink = image->oldChunkLink;

	/*
	 * We loop because page and chunk boundaries may require us to split
	 * the data into more than one section.
	 */
	while ( nbytes )
	{
		/* Find or create the page containing `addr'. */
		if ( !(*pageLink) || addr < (*pageLink)->begin )
			pageLink = &(image->pageList);
		SEARCH_FOR_ADDR(pageLink, nextPage, addr);
		if ( !nextPage || addr < nextPage->begin )
		{
			nextPage = newPage(image, addr, pageLink);
			chunkLink = &(nextPage->chunkList);
		}

		/* Find or create chunk containing `addr'. */
		if ( !(*chunkLink) || (*chunkLink)->begin < nextPage->begin
			 || addr < (*chunkLink)->begin )
		{
			chunkLink = &(nextPage->chunkList);
		}
		SEARCH_FOR_ADDR(chunkLink, nextChunk, addr);
		if ( !nextChunk || addr < nextChunk->begin )
			nextChunk = newChunk(image, addr, nbytes, chunkLink, nextPage);

		/* Copy as much data as will fit in this chunk. */
		dest = nextChunk->data + (addr - nextChunk->begin);
		copylen = (nbytes <= (nextChunk->end - addr) + 1)
			? nbytes
			: nextChunk->end - addr + 1;

		if ( image->skipBytes )
		{
			/*
			 * Ugh!  This loop is slow, but it guarantees that the byte
			 * allignment stays correct across chunk and page boundaries.
			 * Possible target of optimization, but use care!
			 */
			int i;
			for ( i = copylen; i > 0; --i )
				if ( !skipcnt )
				{
					*dest++ = *data++;
					skipcnt = image->skipBytes;
				}
				else
				{
					++dest;
					--skipcnt;
				}
		}
		else
		{
			(void)memcpy(dest, data, copylen);
			data += copylen;
		}
		addr += copylen;
		if ( addr == 0 )
		{
			moan("Input record has logical addr overflow - data truncated");
			break;
		}
		nbytes -= copylen;
	}
	image->oldPageLink = pageLink;
	image->oldChunkLink = chunkLink;
} /* end imageWrite */


/*==========================================================================
 * Read 'nbytes' of data from image starting at addr into buffer.  Zero is
 * used to fill uninitialized areas.
 *==========================================================================*/
void imageRead(Image *image, LogicalAddr addr, size_t nbytes, uchar *data)
{
	Page    **pageLink;
	Chunk   **chunkLink;
	Page    *nextPage;
	Chunk   *nextChunk;
	char    *src;
	size_t  copylen;
	int     skipcnt = 0;
	size_t	totalCopyLen=0;
	LogicalAddr addrSave;
	
	/*
	 *  Don't bother if there is no data.  If we are skipping bytes 
	 *  (presumably because we are interleaving several byte files into 
	 *  longer word oriented files) then the effective number of bytes 
	 *  to be transferred needs to be scaled up.
	 */
	if ( !nbytes )
	{
		if ( debug )
			printf("imageRead(): Looking for 0 byte image record starting at %08lX. Skipped search.\n", addr);
		return;
	}
	if ( image->skipBytes )
	{
		nbytes *= image->skipBytes + 1;
		nbytes -= image->skipBytes; /* trim trailing skipped bytes */
	}

	/* Initialize the page link if NULL or if addr is below last page */
	pageLink  = image->oldPageLink;
	chunkLink = image->oldChunkLink;

	/*
	 * We loop because page and chunk boundaries may require us to split
	 * the data into more than one section.
	 */
	if ( debug )
		printf("imageRead(): Looking for %d byte image record starting at 0x%lX ...\n", nbytes, addr);
	addrSave = addr;
	while ( nbytes )
	{
		/* Find or create the page containing `addr'. */
		if ( !(*pageLink) || addr < (*pageLink)->begin )
			pageLink = &(image->pageList);
		SEARCH_FOR_ADDR(pageLink, nextPage, addr);
		if ( !nextPage || addr < nextPage->begin )
		{
			memset(data, 0, nbytes);
			if ( debug )
			{
				if ( totalCopyLen )
					printf("imageRead(): End of list. Copied %d bytes from 0x%lX-0x%lX\n", totalCopyLen, addrSave, addrSave + totalCopyLen - 1);
				else
					printf("imageRead(): End of list. Copied 0 bytes\n");
			}
			return;
		}

		/* Find or create chunk containing `addr'. */
		if ( !(*chunkLink) || (*chunkLink)->begin < nextPage->begin
			 || addr < (*chunkLink)->begin )
		{
			chunkLink = &(nextPage->chunkList);
		}
		SEARCH_FOR_ADDR(chunkLink, nextChunk, addr);
		if ( !nextChunk || addr < nextChunk->begin )
		{
			memset(data, 0, nbytes);
			if ( debug )
			{
				if ( totalCopyLen )
					printf("imageRead(): End of list. Copied %d bytes from 0x%lX-0x%lX\n", totalCopyLen, addrSave, addrSave + totalCopyLen - 1);
				else
					printf("imageRead(): End of list. Copied 0 bytes\n");
			}
			return;
		}

		/* Copy as much data as is in this chunk. */
		src = nextChunk->data + (addr - nextChunk->begin);
		copylen = (nbytes <= nextChunk->end - addr + 1)
			? nbytes
			: nextChunk->end - addr + 1;

		if ( image->skipBytes )
		{
			/*
			 * Ugh!  This loop is slow, but it guarantees that the byte
			 * allignment stays correct across chunk and page boundaries.
			 * Possible target of optimization, but use care!
			 */
			int i;
			for ( i = copylen; i > 0; --i )
				if ( !skipcnt )
				{
					*data++ = *src++;
					skipcnt = image->skipBytes;
				}
				else
				{
					++data;
					--skipcnt;
				}
		}
		else
		{
			(void)memcpy(data, src, copylen);
			src += copylen;
			data += copylen;
		}
		totalCopyLen += copylen;
		addr += copylen;
		if ( addr == 0 )
		{
			moan("Input record has logical addr overflow - data truncated");
			break;
		}
		nbytes -= copylen;
	}
	image->oldPageLink  = pageLink;
	image->oldChunkLink = chunkLink;
	if ( debug )
	{
		if ( totalCopyLen )
			printf("imageRead(): Copied %d bytes from 0x%lX-0x%lX\n", totalCopyLen, addrSave, addrSave + totalCopyLen - 1);
		else
			printf("imageRead(): Copied 0 bytes\n");
	}

} /* end imageRead */


/*==========================================================================
 * Add symbol `sym' into the image symbol list of `image'.
 *
 * Assumptions:
 *
 *  (1) image->symbolList points to the first record of a (possibly empty) 
 *  linked list ordered by symbol arrival order.  
 *==========================================================================*/
void symbolWrite(Image *image, uchar *sym, int len)
{
	Page    *pg;

	/* Don't bother if there is no symbol. */
	if ( !sym || !*sym )
		return;
	printf("Symbols(%d): %s", len, sym);

	/* Allocate a page and point pg to it */
	if ( !image->symbolList )
		pg = image->symbolList = get_memory(sizeof(Page));
	else
	{
		for ( pg = image->symbolList; pg->next; pg = pg->next );
		pg->next = get_memory(sizeof(Page));
		pg = pg->next;
	}
	pg->chunkList = 0;
	pg->begin = 0;
	pg->data = get_memory(pg->end = len);
	strcpy(pg->data, (char *)sym);
} /* end symbolWrite */

/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/

static Page     *curPage;
static Chunk    *curChunk;
static ulong    chunkBegin, chunkEnd;
static ulong    chunkSize;
static uchar    *chunkPtr;

/*==========================================================================*
 *  init_chunk_read - Initializes globals to read the given chunk.
 *==========================================================================*/
static int init_chunk_read(Chunk *thisChunk)
{
	curChunk    = thisChunk;
	chunkBegin  = curChunk->begin;              /* Save chunk's address range ... */
	chunkEnd    = curChunk->end;
	chunkSize   = chunkEnd - chunkBegin + 1;    /* ... and size */
	chunkPtr    = (uchar *)curChunk->data;      /* and point to the data */

	return chunkSize != 0;

} /* end init_chunk_read */


/*==========================================================================*
 *  init_reader - Initializes the ImageRead variables.  Call before using
 *                readImage.
 *==========================================================================*/
int init_reader(GPF *gpf)
{
	if ( !gpf || !gpf->image.pageList )
		return 0;
	curPage     = gpf->image.pageList;  /* Point to the first page in list */

	return init_chunk_read(curPage->chunkList);

} /* end init_reader */

/*==========================================================================*
 * readImage    - This routine extracts data from the image structure init'ed
 *                by init_reader a record at a time.  It handles filling if
 *                needed.
 *  Inputs:
 *      gpf         - GPF structure that contains flags & characteristics to apply.
 *      bufferPtr   - Buffer to hold the data.
 *      bufferSpace - Size of the buffer in bytes.
 *      low_address - low address of data to retrieve.
 *      high_address- high address of data to retrieve.
 *  Outputs:    
 *      buffer      - buffer filled with data (and/or fill character).
 *      new_low     - Actual low_address used.
 *      bytesRead   - Number of bytes returned in the buffer.
 *      returns     - 1 for success, 0 for no more bytes, -1 for errors.
 *==========================================================================*/
int readImage(GPF *gpf, uchar *bufferPtr, ulong bufferSpace,
			  ulong low_address, ulong high_address, ulong *new_low,
			  int *bytesRead)
{
	uchar   fill = (uchar)(gpf->flags & GPF_M_FILL);
	ulong   amount, startSpace;
	int 	err, more_bytes;

	/*
	 *  Requested data is before this chunk, this implies that there is no
	 *  data before this chunk (would have been handled by a previous call),
	 *  so either return a filled buffer, or an empty buffer.
	 */

	*new_low = low_address;
	if ( high_address < chunkBegin )
	{
		if ( fill )
		{
			memset(bufferPtr, gpf->fill_char, *bytesRead = bufferSpace);
			return 1;
		}
		else
		{
			*new_low   = chunkBegin;
			*bytesRead = 0;
			return 1;
		}
	}
	/*
	 *  If the chunk contains part of the data, we may have to fill some
	 *  of the buffer to account for space before the chunk's data.
	 */

	/*  bufferSpace = high_address - low_address + 1; */
	startSpace = bufferSpace;
	if ( low_address < chunkBegin )
	{

		/*
		 *  Calculate how much fill is needed.  Fill the start of the buffer
		 *  if needed, then adjust the pointers to get chunk data into the 
		 *  correct buffer location.  If no fill used, point the actual
		 *  starting address at the beginning of the chunk for the next call,
		 *  and return immediately.
		 */

		if ( fill )
		{
			amount = min(bufferSpace, chunkBegin - low_address);
			memset(bufferPtr, gpf->fill_char, amount);
			bufferSpace -= amount;
			bufferPtr   += amount;
			if ( !bufferSpace )
			{
				*bytesRead = startSpace;
				return 1;
			}
		}
		else
		{
			*new_low    = chunkBegin;
			*bytesRead  = 0;
			return 1;
		}
		low_address = chunkBegin;
	}

	/*
	 *  At this point the requested data lies either within the chunk or
	 *  after the chunk.  Check for data within the chunk and extract it.
	 *  There may be data that sits in the next chunk, so we have to limit
	 *  extraction to the end of this chunk.
	 */

	if ( low_address <= chunkEnd )
	{
		amount = min(bufferSpace, chunkEnd - low_address + 1);
		memcpy(bufferPtr, chunkPtr, amount);
		bufferPtr   += amount;
		bufferSpace -= amount;
		chunkPtr    += amount;
		low_address += amount;

		if ( !bufferSpace )
		{
			*bytesRead = startSpace;
			return 1;
		}
	}

	/*
	 *  If we are still around, there is no more data in this chunk.
	 *  Move to the next chunk and look there.  If there aren't anymore
	 *  chunks, return.
	 */

	if ( !(curChunk = curChunk->next) )
	{
		/*  No more chunks on this page, look to the next one. */
		if ( !(curPage = curPage->next) )
		{

			/*
			 *  No more pages, fill in the end of the buffer if needed
			 *  and return.
			 */

			if ( fill )
			{
				memset(bufferPtr, gpf->fill_char, bufferSpace);
				bufferSpace = 0;
			}

			*bytesRead = startSpace - bufferSpace;
			return 0;
		}

		curChunk = curPage->chunkList;
	}

	if ( !init_chunk_read(curChunk) )
	{
		*bytesRead = startSpace - bufferSpace;
		return 0;
	}

	/*
	 *  Now that we are set up for the next chunk, a recursive call should
	 *  be able to handle any remaining data.
	 */

	err = readImage(gpf, bufferPtr, bufferSpace, low_address, high_address,
					&low_address, &more_bytes);
	*bytesRead = startSpace - bufferSpace + more_bytes;
	return err;

} /* end readImage */


/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/

#ifdef DEBUG
/*==========================================================================
 * Dump the Image to a file.  Outputs a detailed page and chunk
 * description of the entire Image in human decipherable text,
 * for use as a debug tool.
 *==========================================================================*/
void imageDump(Image *image, FILE *dump)
{
	Page    *page;
	Chunk   *chunk;
	int i, j, size, nchunk, npage;
	char    buf[140];
	char    *cp;
	uchar   *data;

	npage = 0;
	for ( page = image->pageList; page; page = page->next )
	{
		fprintf(dump, " page %d (%04lX-%04lX)\n", ++npage, page->begin, page->end);
		nchunk = 0;
		for ( chunk = page->chunkList; chunk; chunk = chunk->next )
		{
			size = chunk->end - chunk->begin + 1;
			fprintf(dump, "  chunk %d at %04lX-%04lX (%d bytes):\n", ++nchunk, chunk->begin, chunk->end, size);
			i = 0 - (int)(chunk->begin & 0xF);
			data = (uchar *)(chunk->data);
			do
			{
				cp = buf;
				sprintf(cp, "   %8.4lX: ", chunk->begin + i);
				cp += strlen(cp);
				for ( j = i + 16; i < j; ++i )
				{
					if ( i < 0 || i >= size )
					{
						sprintf(cp, "    ");
						cp += strlen(cp);
					}
					else
					{
						sprintf(cp, "  %02X", *data++);
						cp += strlen(cp);
					}
				}
				*cp++ = '\n';
				*cp = '\0';
				fputs(buf, dump);
			} while ( i < size );
		}
		putc('\n', dump);
	}
} /* end imageDump */


/*==========================================================================
 * Perform and internal consistency check on an Image and return
 * a status code.  For use as a debug tool.
 *==========================================================================*/
int imageCheck(Image *image, FILE *log)
{
	Page        *page;
	Chunk       *chunk;
	LogicalAddr addr;
	int     pagecnt, chunkcnt;
	int     haserror = 0;

	/* Check for pages and chunks in order and non-overlapping. */
	addr = 0;
	pagecnt = 0;
	for ( page = image->pageList; page; page = page->next )
	{
		++pagecnt;
		if ( page->begin < addr )
		{
			fprintf(log, "Page %d has bad beginning addr 0x%4X\n",
					pagecnt, page->begin);
			haserror = 1;
			addr = page->begin;
		}
		chunkcnt = 0;
		for ( chunk = page->chunkList; chunk; chunk = chunk->next )
		{
			++chunkcnt;
			if ( chunk->begin < addr )
			{
				fprintf(log, "Chunk %d in Page %d has bad beginning addr 0x%4X\n",
						chunkcnt, pagecnt, chunk->begin);
				haserror = 1;
				addr = chunk->begin;
			}
			if ( chunk->end < addr )
			{
				fprintf(log, "Chunk %d in Page %d has bad ending addr 0x%4X\n",
						chunkcnt, pagecnt, chunk->end);
				haserror = 1;
			}
			else
				addr = chunk->end + 1;
		}
		if ( page->end < (addr - 1) )
		{
			fprintf(log, "Page %d has bad ending addr 0x%4X\n",
					pagecnt, page->end);
			haserror = 1;
		}
		else
			addr = page->end + 1;
	}
	return (haserror);
} /* end imageCheck */

#endif /* DEBUG */

