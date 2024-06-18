/* 
 * Simple, 32-bit and 64-bit clean allocator based on an implicit free list,
 * first fit placement, and boundary tag coalescing, as described in the
 * CS:APP3e text.  Blocks are aligned to double-word boundaries.  This
 * yields 8-byte aligned blocks on a 32-bit processor, and 16-byte aligned
 * blocks on a 64-bit processor.  However, 16-byte alignment is stricter
 * than necessary; the assignment only requires 8-byte alignment.  The
 * minimum block size is four words.
 *
 * This allocator uses the size of a pointer, e.g., sizeof(void *), to
 * define the size of a word.  This allocator also uses the standard
 * type uintptr_t to define unsigned integers that are the same size
 * as a pointer, i.e., sizeof(uintptr_t) == sizeof(void *).
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "memlib.h"
#include "mm.h"
/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
	/* Team name */
	"啊对对队",
	/* First member's full name */
	"Yuqi Chen",
	/* First member's NetID */
	"yc138",
	/* Second member's full name (leave blank if none) */
	"Zikang Chen",
	/* Second member's NetID (leave blank if none) */
	"zc45"
};
/* Basic constants and macros: */
#define WSIZE      sizeof(void *) /* Word and header/footer size (bytes) */
#define DSIZE      (2 * WSIZE)    /* Doubleword size (bytes) */
#define CHUNKSIZE  (1 << 13)      /* Extend heap by this amount (bytes) */
#define MAX(x, y)  ((x) > (y) ? (x) : (y))  
/* Pack a size and allocated bit into a word. */
#define PACK(size, alloc)  ((size) | (alloc))
/* Read and write a word at address p. */
#define GET(p)       (*(uintptr_t *)(p))
#define PUT(p, val)  (*(uintptr_t *)(p) = (val))
/* Read the size and allocated fields from address p. */
#define GET_SIZE(p)   (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)  (GET(p) & 0x1)
/* Given block ptr bp, compute address of its header and footer. */
#define HDRP(bp)  ((char *)(bp) - WSIZE)
#define FTRP(bp)  ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
/* Given block ptr bp, compute address of next and previous blocks. */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
/* Global variables: */
static char *heap_listp; /* Pointer to first block */  
static struct free_list *head; /* Pointer to first free list */
/* Function prototypes for internal helper routines: */
static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
/* Function prototypes for heap consistency checker routines: */
static void checkblock(void *bp);
static void checkheap(bool verbose);
static void printblock(void *bp);
static void add_free_list(void *bp);
static void delete_free_list(void *bp);
static int find_class(size_t asize);
static void checkfreeblock(struct free_list *bp);



struct free_list{
	struct free_list *prev;
	struct free_list *next;
};
/* 
 * Requires:
 *   None.
 *
 * Effects:
 *   Initialize the memory manager.  Returns 0 if the memory manager was
 *   successfully initialized and -1 otherwise.
 */
int
mm_init(void) 
{
	
	/* Create the initial empty heap. */
	if ((heap_listp = mem_sbrk(5 * sizeof(struct free_list) + 3 * WSIZE)) == (void *)-1)
		return (-1);
	head = (struct free_list *)heap_listp;
	/* size: 1~4, 5~8, 9~16, 17~32, 33~inf */
	for (int i = 0; i < 5; i++) {
		head[i].prev = &head[i];
		head[i].next = &head[i];
	}
	

	PUT(heap_listp + (10 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */ 
	PUT(heap_listp + (11 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */ 
	PUT(heap_listp + (12 * WSIZE), PACK(0, 1));     /* Epilogue header */
	

	heap_listp += (11 * WSIZE);
	
	
	/* Extend the empty heap with a free block of CHUNKSIZE bytes. */
	if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
		return (-1);

	return (0);
}
/* 
 * Requires:
 *   None.
 *
 * Effects:
 *   Allocate a block with at least "size" bytes of payload, unless "size" is
 *   zero.  Returns the address of this block if the allocation was successful
 *   and NULL otherwise.
 */
void *
mm_malloc(size_t size) 
{
	size_t asize;      /* Adjusted block size */
	size_t extendsize; /* Amount to extend heap if no fit */
	void *bp;

	/* Round up the size to the nearest power of 2. */
	if (size <= 512) {
		size--;
		size |= size >> 1;
		size |= size >> 2;
		size |= size >> 4;
		size |= size >> 8;
		size |= size >> 16;
		size++;
	}

	/* Ignore spurious requests. */
	if (size == 0)
		return (NULL);
	/* Adjust block size to include overhead and alignment reqs. */
	if (size <= DSIZE)
		asize = 2 * DSIZE;
	else
		asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);

	/* Search the free list for a fit. */
	if ((bp = find_fit(asize)) != NULL) {
		// printf("In Malloc, find_fit is not null!\n");
		place(bp, asize);
		return (bp);
	}
	/* No fit found.  Get more memory and place the block. */
	extendsize = MAX(asize, CHUNKSIZE);
	if ((bp = extend_heap(extendsize / WSIZE)) == NULL) {
		return (NULL);
	}  

	place(bp, asize);
	return (bp);
} 
/* 
 * Requires:
 *   "bp" is either the address of an allocated block or NULL.
 *
 * Effects:
 *   Free a block.
 */
void
mm_free(void *bp)
{
	size_t size;
	/* Ignore spurious requests. */
	if (bp == NULL)
		return;
	/* Free and coalesce the block. */
	size = GET_SIZE(HDRP(bp));
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	coalesce(bp);
}
/*
 * Requires:
 *   "ptr" is either the address of an allocated block or NULL.
 *
 * Effects:
 *   Reallocates the block "ptr" to a block with at least "size" bytes of
 *   payload, unless "size" is zero.  If "size" is zero, frees the block
 *   "ptr" and returns NULL.  If the block "ptr" is already a block with at
 *   least "size" bytes of payload, then "ptr" may optionally be returned.
 *   Otherwise, a new block is allocated and the contents of the old block
 *   "ptr" are copied to that new block.  Returns the address of this new
 *   block if the allocation was successful and NULL otherwise.
 */
void *
mm_realloc(void *ptr, size_t size)
{
	size_t oldsize;
	void *newptr;
	/* If size == 0 then this is just free, and we return NULL. */
	if (size == 0) {
		mm_free(ptr);
		return (NULL);
	}
	/* If oldptr is NULL, then this is just malloc. */
	if (ptr == NULL)
		return (mm_malloc(size));
	/* If the size is already large enough, just return prt. */
	if (GET_SIZE(HDRP(ptr)) - 2 * WSIZE >= size) {
		return ptr;
	}
	/* Check whether it needs coalescing. */
	void *next_ptr = NEXT_BLKP(ptr);
	size_t coalesce_size = (GET_SIZE(HDRP(next_ptr)) + GET_SIZE(HDRP(ptr)));
	int next_alloc = GET_ALLOC(HDRP(next_ptr));
	if (!next_alloc && size <= coalesce_size - 2 * WSIZE) {
		delete_free_list(next_ptr);
		PUT(HDRP(ptr), PACK(coalesce_size, 1));
		PUT(FTRP(ptr), PACK(coalesce_size, 1));
		return ptr;
	}
	newptr = mm_malloc(size);
	/* If realloc() fails, the original block is left untouched. */
	if (newptr == NULL)
		return (NULL);
	/* Copy just the old data, not the old header and footer. */
	oldsize = GET_SIZE(HDRP(ptr)) - DSIZE;
	/* Copy the old data. */
	// oldsize = GET_SIZE(HDRP(ptr));
	if (size < oldsize)
		oldsize = size;
	memcpy(newptr, ptr, oldsize);
	/* Free the old block. */
	mm_free(ptr);
	return (newptr);
}
/*
 * The following routines are internal helper routines.
 */
/*
 * Requires:
 *   "bp" is the address of a newly freed block.
 *
 * Effects:
 *   Perform boundary tag coalescing.  Returns the address of the coalesced
 *   block.
 */
static void *
coalesce(void *bp) 
{
	size_t size = GET_SIZE(HDRP(bp));
	bool prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	bool next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

	if (prev_alloc && next_alloc) {                 /* Case 1 */

		
		add_free_list(bp);

		return (bp);
	} else if (prev_alloc && !next_alloc) {         /* Case 2 */
		delete_free_list(NEXT_BLKP(bp));
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	} else if (!prev_alloc && next_alloc) {         /* Case 3 */

		delete_free_list(PREV_BLKP(bp));
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	} else {                                        /* Case 4 */

		delete_free_list(PREV_BLKP(bp));
		delete_free_list(NEXT_BLKP(bp));
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
		    GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	add_free_list(bp); 
	return (bp);
}
/* 
 * Requires:
 *   None.
 *
 * Effects:
 *   Extend the heap with a free block and return that block's address.
 */
static void *
extend_heap(size_t words) 
{
	size_t size;
	void *bp;
	/* Allocate an even number of words to maintain alignment. */
	size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
	if ((bp = mem_sbrk(size)) == (void *)-1)  
		return (NULL);
	/* Initialize free block header/footer and the epilogue header. */
	PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
	PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */
	/* Coalesce if the previous block was free. */
	return (coalesce(bp));
}
/*
 * Requires:
 *   None.
 *
 * Effects:
 *   Find a fit for a block with "asize" bytes.  Returns that block's address
 *   or NULL if no suitable block was found. 
 */
static void *
find_fit(size_t asize)
{
	struct free_list *bp;
	int i = find_class(asize);

	/* Find the first appropriate block in linked list. */
	while (i < 5) {
		bp = &head[i];
		bp = bp -> next;
		while (bp != &head[i]) {
			if (asize <= GET_SIZE(HDRP(bp))) {
				return (bp);
			}
			bp = bp -> next;
		}
	 	i++;
	}
	/* No fit was found. */
	return (NULL);
}
/* 
 * Requires:
 *   "bp" is the address of a free block that is at least "asize" bytes.
 *
 * Effects:
 *   Place a block of "asize" bytes at the start of the free block "bp" and
 *   split that block if the remainder would be at least the minimum block
 *   size. 
 */
static void
place(void *bp, size_t asize)
{
	/* Place the block. */
	size_t csize = GET_SIZE(HDRP(bp));   
	if ((csize - asize) >= (2 * DSIZE)) { 
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		delete_free_list(bp);
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize - asize, 0));
		PUT(FTRP(bp), PACK(csize - asize, 0));
		add_free_list(bp);
	} else {
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
		delete_free_list(bp);
	}
}
/* 
 * The remaining routines are heap consistency checker routines. 
 */
/*
 * Requires:
 *   "bp" is the address of a block.
 *
 * Effects:
 *   Perform a minimal check on the block "bp".
 */
static void
checkblock(void *bp) 
{
	if ((uintptr_t)bp % DSIZE)
		printf("Error: %p is not doubleword aligned\n", bp);
	if (GET(HDRP(bp)) != GET(FTRP(bp)))
		printf("Error: header does not match footer\n");
}
/* 
 * Requires:
 *   None.
 *
 * Effects:
 *   Perform a minimal check of the heap for consistency. 
 */
void
checkheap(bool verbose) 
{
	//check free_list
	void *bp;
	int num_free_heap = 0;
	int num_free_list = 0;

	if (verbose)
		printf("Heap (%p):\n", heap_listp);
	if (GET_SIZE(HDRP(heap_listp)) != DSIZE ||
	    !GET_ALLOC(HDRP(heap_listp)))
		printf("Bad prologue header\n");
	checkblock(heap_listp);
	for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
		if (verbose)
			printblock(bp);
		checkblock(bp);
		if (!GET_ALLOC(HDRP(bp))) {
			num_free_heap++;
		}
	}

	if (verbose)
		printblock(bp);
	if (GET_SIZE(HDRP(bp)) != 0 || !GET_ALLOC(HDRP(bp)))
		printf("Bad epilogue header\n");


	for (int i = 0; i < 5; i++) {
		if (&head[i] != NULL) {
			for (bp = (&head[i]) -> next; bp != &head[i]; bp = ((struct free_list *)bp) -> next){
				checkfreeblock(bp);
				num_free_list++;

			}
		} else {
			printf("Bad dummy head %d\n", i);
		}
	}

	if (num_free_heap != num_free_list) {
		printf("Mismatch free blocks between heap and free list\n");
	}
}

/*
 * Requires: 
 *    "bp" is a non-null struct free_list pointer.
 *
 * Effects:
 *    Check the validity of the free_list pointer.
 *
 */
static void
checkfreeblock(struct free_list *bp){
	if (bp -> next -> prev != bp) {
		printf("Error: free list inconsistent at %p\n", bp);
	}
	if (bp -> prev -> next != bp) {
		printf("Error: free list inconsistent at %p\n", bp);
	}
}


/*
 * Requires:
 *   "bp" is the address of a block.
 *
 * Effects:
 *   Print the block "bp".
 */
static void
printblock(void *bp) 
{
	size_t hsize, fsize;
	bool halloc, falloc;
	checkheap(false);
	hsize = GET_SIZE(HDRP(bp));
	halloc = GET_ALLOC(HDRP(bp));  
	fsize = GET_SIZE(FTRP(bp));
	falloc = GET_ALLOC(FTRP(bp));  
	if (hsize == 0) {
		printf("%p: end of heap\n", bp);
		return;
	}
	printf("%p: header: [%zu:%c] footer: [%zu:%c]\n", bp, 
	    hsize, (halloc ? 'a' : 'f'), 
	    fsize, (falloc ? 'a' : 'f'));
}



/*
 * Requirments: 
 *    "bp" is non-null pointer.
 *
 * Effects:
 *    Add "bp" to the free_list.
 *
 */
static void
add_free_list(void *bp){
	size_t asize = GET_SIZE(HDRP(bp));

	struct free_list *curr = (struct free_list *)bp;
	int i = find_class(asize);

	/* Insert the new free list at the first place */
	(head[i].next) -> prev = curr;
	curr -> next = head[i].next;
	curr -> prev = &head[i];
	head[i].next = curr;
}

/*
 * Requirments: 
 *    "bp" is non-null pointer.
 *
 * Effects:
 *    Delete "bp" from the free_list.
 *
 */
static void
delete_free_list(void *bp){
	struct free_list *target = (struct free_list *) bp;

	target -> prev -> next = target -> next;
	target -> next -> prev = target -> prev;
	target -> prev = NULL;
	target -> next = NULL;
}

/*
 * Requirments: 
 *    "asize" is a multiple of "WSIZE".
 *
 * Effects:
 *    Find the proper bin to hold the free block.
 *
 */
static int
find_class(size_t asize){
	int count = asize / WSIZE;
	int idx;

	/* Choose the appropriate size class */
	if (count <= 4 && count > 0) {
		idx = 0;
	} else if (count <= 8) {
		idx = 1;
	} else if (count <= 16) {
		idx = 2;
	} else if (count <= 32) {
		idx = 3; 
	} else {
		idx = 4;
	}
	return idx;
}