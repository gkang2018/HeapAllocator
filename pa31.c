// Part 1 of Heap Assignment by Gagandeep Kang: December 6th, 2019 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef char *addrs_t;
typedef void *any_t; 

#define WSIZE               4       /* Word and header/footer size (bytes) */   
#define DSIZE               8           /* Doubleword size (bytes) */
#define DEFAULT_MEM_SIZE    1<<20

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))   

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p)) 
#define PUT(p, val)  (*(unsigned int *)(p) = (val)) 

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)    
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)   
#define FTRP(bp)             ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))    
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define rdtsc(x)      __asm__ __volatile__("rdtsc \n\t" : "=A" (*(x)))
/* Global variables */



/* Function prototypes for internal helper routines */

static void place(void *bp, size_t asize);
static void *find_first_fit(size_t size);
static void *coalesce(void *bp);



// global variables that will be used throughout and for heap checker purposes 
addrs_t baseptr;    /* Pointer to first block */
int allocatedBlocks = 0; //keeps track of the allocated blocks 
int freeBlocks = 1; //keeps track of the number of free blocks 
int rawBytesAlloc = 0; //which is the actual total bytes requested 
int paddedBlocks = 0; //which is the total bytes requested plus internally fragmented blocks wasted due to padding/alignment
int numRawFreeBytes = 0; //pretty self-explanatory, refers to the number of raw free bytes 
int alignFreeBytes = 0; //which is sizeof(M1) minus the padded total number of bytes allocated. You should account for meta-datastructures inside M1 also



// malloc free 
int totalMallocReq = 0; // refers to the total number of malloc requests 
int totalFreeReq = 0; // refers to the total number of free requests 
int totalFailReq = 0;// refers to the total number of request failures, which were unable to satisfy the allocation or de-allocation requests, hopefully don't have to use this

// clock cycles 

int mallocAvg = 0; // 
int freeAvg = 0;
int totalClockCycles = 0;


// Init function to allocate our size number of bytes for our initial memory Area 

void Init (size_t size) {

  size_t asize = size - (5 * WSIZE); //  adjusted size of free allocated memory, takes into account the size of the initialized headers // 
  baseptr = (addrs_t) malloc (size);  // starting address of M1 
  PUT (baseptr, 0);                                         /* Alignment padding */
  PUT (baseptr + (1 * WSIZE), PACK (DSIZE, 1));             /* Prologue header */
  PUT (baseptr + (2 * WSIZE), PACK (DSIZE, 1));             /* Prologue footer */
  PUT (baseptr + (3 * WSIZE), PACK (asize, 0));             /* New malloc header */
  PUT (baseptr + asize + (3 * WSIZE), PACK (asize, 0));         /* New malloc footer */
  PUT (baseptr + size, PACK (0, 1));                        /* Epilogue header*/
  baseptr += (4 * WSIZE);                           // shifts the baseptr so its at the start of the malloc header 

}


addrs_t Malloc (size_t size) {

  /*
    Implement your own memory allocation routine here. 
   This should allocate the first contiguous size bytes available in M1.
    Since some machine architectures are 64-bit, it should be safe to allocate space starting
    at the first address divisible by 8. Hence align all addresses on 8-byte boundaries!
    If enough space exists, allocate space and return the base address of the memory.
    If insufficient space exists, return NULL.
  */

  totalMallocReq++; //increment our malloc call

  
  size_t asize; // adjusted block size 

  // pointer to iterate over freeList
  addrs_t bp;
  
  if (baseptr == 0) {
    Init (size);
  }
  // any requests that fail 
  if (size <= 0) {
    totalFailReq++;
    return NULL;
  }
	/* Adjust block size to include overhead and alignment reqs. */
  if (size <= DSIZE)            
        asize = 2 * DSIZE;      
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    
  if((bp = find_first_fit(asize)) != NULL) {
      
      rawBytesAlloc += (size); //update the raw bytes allocated
      allocatedBlocks++;
      paddedBlocks += (asize-DSIZE); //update padded bytes
      place(bp,asize);
      return bp;
    }
  totalFailReq++;
  return NULL;
    
  }

  
  
/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 */

static void place (void *bp, size_t size)
{

    size_t csize = GET_SIZE (HDRP (bp)); // this is the current size of the parameter free block

    // checks to see if there is enough space to split the unallocated block.
    if ((csize - size) >= (2 * DSIZE)) {
        PUT (HDRP (bp), PACK (size, 1));            // overwrites free header with alloc header
        PUT (FTRP (bp), PACK (size, 1));            // overwrites free footer with alloc footer
        bp = NEXT_BLKP (bp);                        // address moves to the remainder of the free block.
        PUT (HDRP (bp), PACK (csize - size, 0));    // creates updated free header
        PUT (FTRP (bp), PACK (csize - size, 0));    // creates updated free footer
    }
    else {                                          // Else overwrite the header and footer to be allocated.
        PUT (HDRP (bp), PACK (csize, 1));           
        PUT (FTRP (bp), PACK (csize, 1));
        freeBlocks--;
    }
}

/* find_first_fit -Find a fit for a block with asize bytes, however return null if we cant .*/ 
 
static void *find_first_fit(size_t size) {
    
    void *bp;
    
    for (bp = baseptr; GET_SIZE (HDRP (bp)) > 0; bp = NEXT_BLKP (bp)) {
        if (!GET_ALLOC (HDRP (bp)) && (size <= GET_SIZE (HDRP (bp)))) {
            return bp;
        }
    }
    return NULL; 
}


  /*
    This frees the previously allocated size bytes starting from address addr in the
    memory area, M1. You can assume the size argument is stored in a data structure after
    the Malloc() routine has been called, just as with the UNIX free() command.
  */
void Free (addrs_t bp) {
    totalFreeReq++;
    if (bp == 0){
        totalFailReq++;
        return;
    }
    size_t size = GET_SIZE (HDRP (bp));
    if (baseptr == 0) {
          Init(size);
    }
    PUT (HDRP (bp), PACK (size, 0));
    PUT (FTRP (bp), PACK (size, 0));
    allocatedBlocks--;
    paddedBlocks -= (size-DSIZE); //update the padded bytes allocated
    coalesce (bp);
}


/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
static void *coalesce (void *bp)
{
    size_t prev_alloc = GET_ALLOC (FTRP (PREV_BLKP (bp)));
    size_t next_alloc = GET_ALLOC (HDRP (NEXT_BLKP (bp)));
    size_t size = GET_SIZE (HDRP (bp));

    if (prev_alloc && next_alloc) { // Case 1 : Can't coalesce because previous and next are already allocated 
        freeBlocks++;  
        return bp;
    }

    else if (prev_alloc && !next_alloc) {   // Case 2 : next block is free, previous is not .
        size += GET_SIZE (HDRP (NEXT_BLKP (bp)));
        PUT (HDRP (bp), PACK (size, 0));
        PUT (FTRP (NEXT_BLKP (bp)), PACK (size, 0));
    }

    else if (!prev_alloc && next_alloc) {   // Case 3 : previous block is free, next is not.
        size += GET_SIZE (HDRP (PREV_BLKP (bp)));
        PUT (FTRP (bp), PACK (size, 0));
        PUT (HDRP (PREV_BLKP (bp)), PACK (size, 0));
        bp = PREV_BLKP (bp);
    }

    else {                      // Case 4 : previous and next block are free.
        size += GET_SIZE  (HDRP (PREV_BLKP (bp)));
        size += GET_SIZE  (HDRP (NEXT_BLKP (bp)));
        PUT (HDRP (PREV_BLKP (bp)), PACK (size, 0));
        PUT (FTRP (NEXT_BLKP (bp)), PACK (size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}



  /*
   Allocate size bytes from M1 using Malloc().
   Copy size bytes of data into Malloc'd memory.
   You can assume data is a storage area outside M1.
   Return starting address of data in Malloc'd memory.
  */
addrs_t Put (any_t data, size_t size) {
 
  addrs_t bp = Malloc(size);
  if(bp == NULL){
      return 0;
  }
  memcpy(bp,data,size);         // copies data into the heap
  return bp;
}

/*
    Copy size bytes from addr in the memory area, M1, to data address. 
    As with Put(), you can assume data is a storage area outside M1. 
    De-allocate size bytes of memory starting from addr using Free(). 
  */

void Get (any_t return_data, addrs_t addr, size_t size) {
// takes the data at the address, uses memcpy function to return to return_data
   memcpy(return_data,addr,size);   
   Free(addr);  
   rawBytesAlloc -= (size);  
}

