// Part 2 of Heap Assignment using Vmalloc and vfree 
// by Gagandeep Kang: December 6th, 2019 

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



addrs_t *RT = NULL;
int RT_size; // Measured in number of elements, not bytes

void VInit(size_t size)  {
  size_t asize = size - (5 * WSIZE);  /* adjusted size of free allocated memory, taking into account the size of the initialized headers */   
  baseptr = (addrs_t) malloc (size); 
  PUT (baseptr, 0);                                         /* Alignment padding */
  PUT (baseptr + (1 * WSIZE), PACK (DSIZE, 1));             /* Prologue header */
  PUT (baseptr + (2 * WSIZE), PACK (DSIZE, 1));             /* Prologue footer */
  PUT (baseptr + (3 * WSIZE), PACK (asize, 0));             /* New malloc header */
  PUT (baseptr + asize + (3 * WSIZE), PACK (asize, 0));         /* New malloc footer */
  PUT (baseptr + size, PACK (0, 1));                        /* Epilogue header*/
  baseptr += (4 * WSIZE);                           /* Moves the baseptr to the start of the malloc header */
  RT_size = asize / (2 * DSIZE);
  RT = malloc(RT_size * sizeof(*RT));              // Initializes the Redirection table 
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



/* Vfind_first_fit -  Find a fit for a block with size bytes  */

static void *Vfind_first_fit(size_t size) {
    
    void *bp;
    
    // Loops over the heap
    for (bp = baseptr; GET_SIZE (HDRP (bp)) > 0; bp = NEXT_BLKP (bp)) {
        if (!GET_ALLOC (HDRP (bp)) && (size <= GET_SIZE (HDRP (bp)))) {
            return bp;
        }
    }
    return NULL; 
}



// looks for a free spot in the redirection table
int findRTFree() {
  int i;
  for(i = 0; i < RT_size; i++) {
    if (RT[i] == NULL) {
      return i;
    }
  }
  return -1;
}


// VMalloc - Allocates a block of size t while adjusting its size. Also creates and returns pointer to its corresponding entry in the 
// redirection table.
addrs_t *VMalloc(size_t size) {
   totalMallocReq++; //update the malloc call counter
   // Calculates the final size of allocation so that it is aligned
  size_t asize;                            // aligned size + overhead
  addrs_t bp;                              // pointer to iterate over freeList
  int i = findRTFree();             // Index of possible RT entry for VMalloc
  addrs_t *RTbp;                           // pointer pointing to the RT entry containing pointer to malloc block
  if (baseptr == 0) {
    VInit (size);
  }
  // catches requests that can break code
  if (size <= 0){ 
      totalFailReq++;
      return NULL;
  }
  // adjusts block size to make it aligned 
  if (size <= DSIZE)            
        asize = 2 * DSIZE;      
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
  
  
  if(i >= 0) {                                      // Checks if findRTFree() actually found something. AKA if RT has an available entry.
    if ((bp = Vfind_first_fit(asize)) != NULL) {    // Iterates over the heap to find an unallocated block with enough size.
        place(bp,asize);                            // Allocates the block.
        allocatedBlocks++;
        rawBytesAlloc += (size); //update the raw bytes allocated
        paddedBlocks += (asize-DSIZE); //update padded bytes
        RT[i] = bp;
        RTbp = &RT[i];
        return RTbp;
    }
}
  totalFailReq++;
  return NULL;                                      // Returns NULL if no space found to allocate.
  

}

/*  VFree frees both a block of memory with address addr, and its
    corresponding RT entry. Also performs the coalescing and compaction
    of the heap if it leaves a gap.
*/
void VFree (addrs_t *addr) {
    // REMEMBER: *addr dereferences the pointer to the RT element.
    totalFreeReq++;
     if (*addr == 0){
        totalFailReq++;
        return;
     }
    // Gets address of next block
    addrs_t next_bp = NEXT_BLKP(*addr);     // Creates address to the block after addr.
    addrs_t curr_bp = *addr;                // Copies the addr_t that addr is pointing to for loop purposes.
    if (addr == NULL || *addr == NULL)      // If addr or *addr is null, return to avoid changes. Avoid errors when trying to dereference a null pointer.
        return;

    size_t size = GET_SIZE (HDRP (*addr));  // Gets size of *addr (adjusted to be aligned with overhead) from its header
    if (baseptr == 0) {
         VInit(size);
    }
    // Updates the header and footer 
    PUT (HDRP (*addr), PACK (size, 0));     
    PUT (FTRP (*addr), PACK (size, 0));

    // While loop runs until it detects there are no more allocated blocks after its current block. AKA, it compacts.
    // Essentially swaps the free block forward with the allocated blocks until it reaches the last unallocated block.


    //ASK SASAN ABOUT THIS, I think i did this correctly because it essentially swaps the next block  
    while(GET_ALLOC(HDRP (next_bp)) != 0) {
        size_t next_bp_size = GET_SIZE(HDRP (next_bp));     //next_bp
        size_t curr_size = GET_SIZE(HDRP (curr_bp));        



        PUT (HDRP (curr_bp), PACK (next_bp_size, 1));       // Calculates and puts the header of next_bp in curr_bp's location.
        PUT (FTRP (curr_bp), PACK (next_bp_size, 1));       // Same as above, but with the footer.
        memcpy(curr_bp, next_bp, next_bp_size);             
        
        // inefficiency issue
        
        next_bp = NEXT_BLKP(curr_bp);                       // updates its location to the new next block of curr_bp (which is overwritten with next_bp's data).
        PUT (HDRP (next_bp), PACK (curr_size, 0));          
        
        PUT (FTRP (next_bp), PACK (curr_size, 0));          // Same as above, but with footer.
    
    // switch pointer 

        curr_bp = next_bp;                                  // curr_bp now points to its next block.
        next_bp = NEXT_BLKP(next_bp);                       // increments loop, next_bp points to its next block.


    }




    // This is to reflect the changed addresses of the allocated blocks due to compaction, might have to ask sasan about this 
    int i;
    for(i = 0; i < RT_size; i++) {
        if(RT[i] > *addr) {
            RT[i] -= size;
        }
    }
    *addr = NULL;           // Sets RT entry to NULL.
    allocatedBlocks--;
    paddedBlocks -= (size-DSIZE); //update the padded bytes allocated
    coalesce (next_bp);     // Coalesces free'd block with the big unallocated block if it exists.

}

/* Allocate size bytes from M2 using VMalloc(). 
   Copy size bytes of data into Malloc'd memory. 
   You can assume data is a storage area outside M2. 
   Return pointer to redirection table for Malloc'd memory.
*/
addrs_t *VPut (any_t data, size_t size) {
    
    addrs_t *RTbp = VMalloc(size);
    if(RTbp == NULL) {
        return 0;
    }
    memcpy(*RTbp,data,size);        // copies data from data to *RTbp.
    return RTbp;

    
}

/* Copy size bytes from the memory area, M2, to data address. The
    addr argument specifies a pointer to a redirection table entry. 
    As with VPut(), you can assume data is a storage area outside M2. 
    Finally, de-allocate size bytes of memory using VFree() with addr 
    pointing to a redirection table entry. 
*/
void VGet (any_t return_data, addrs_t *addr, size_t size) {
   memcpy(return_data,*addr,size);   // copies data from *addr to return_data.
   VFree(addr);  
   rawBytesAlloc -= (size);

}
 

