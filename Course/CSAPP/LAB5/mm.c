/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"
static char *heap_listp = 0;
/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "xteam",
    /* First member's full name */
    "Bill Chen",
    /* First member's email address */
    "Bill.Chen@live.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
static void *coalesce(void *bp) {
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	if (prev_alloc && next_alloc) {
		return bp;
	} else if (prev_alloc && !next_alloc) {
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	} else if (!prev_alloc && next_alloc) {
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	} else {
		size += GET_SIZE(FTRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	return bp;
}

static void *extend_heap(size_t words) {
	char *bp;
	size_t size;
	size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
	if ((long)(bp = mem_sbrk(size)) == (void *)-1)
		return NULL;
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
	return coalesce(bp);
}



static void *find_fit(size_t size) {
	void *bp;
	for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
		if (!GET_ALLOC(HDRP(bp)) && (size <= GET_SIZE(HDRP(bp)))) {
			return bp;
		}
	}
	return NULL;
}

static void place(void *bp, size_t asize) {
	size_t csize = GET_SIZE(HDRP(bp));
	if ((csize - asize) >= (2 * DSIZE)) {
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize - asize, 0));
		PUT(FTRP(bp), PACK(csize - asize, 0));
	} else {
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
	}
}
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
	/* Create the initial empty heap */
	if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
		return -1;
	PUT(heap_listp, 0);			       /* Alignment padding */
	PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
	PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
	PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
	heap_listp += (2 * WSIZE);

	/* Extend the empty heap with a free block of CHUNKSIZE bytes */
	if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
		return -1;
	return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
	size_t asize;
	size_t extendsize;
	char *bp;
	if (size == 0)
		return NULL;
	if (size <= DSIZE) {
		asize = 2 * (DSIZE);
	} else {
		asize = (DSIZE) * ((size + (DSIZE) + (DSIZE - 1)) / (DSIZE));
	}
	if ((bp = find_fit(asize)) != NULL) {
		place(bp, asize);
		return bp;
	}
	extendsize = MAX(asize, CHUNKSIZE);
	if ((bp = extend_heap(extendsize / WSIZE)) == NULL) {
		return NULL;
	}
	place(bp, asize);
	return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp) {
	if (bp == 0)
		return;
	size_t size = GET_SIZE(HDRP(bp));
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
	size_t oldsize;
	void *newptr;
	/* If size == 0 then this is just free, and we return NULL. */
	if (size == 0) {
		mm_free(ptr);
		return 0;
	}
	/* If oldptr is NULL, then this is just malloc. */
	if (ptr == NULL) {
		return mm_malloc(size);
	}
	newptr = mm_malloc(size);
	/* If realloc() fails the original block is left untouched  */
	if (!newptr) {
		return 0;
	}
	/* Copy the old data. */
	oldsize = GET_SIZE(HDRP(ptr));
	if (size < oldsize)
		oldsize = size;
	memcpy(newptr, ptr, oldsize);
	/* Free the old block. */
	mm_free(ptr);
	return newptr;
}
