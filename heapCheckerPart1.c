#include "pa31.c"
#include "pa32.c"


void main (int argc, char **argv) {

  int i, n;
  char s[80];
  addrs_t addr1, addr2,addr3;
  char data[80];
  int mem_size = 1 << 20; // Set DEFAULT_MEM_SIZE to 1<<20 bytes for a heap region
  unsigned long tot_alloc_time, tot_free_time;
  unsigned long start, finish;
  tot_alloc_time = 0;
  tot_free_time = 0;
  if  (argc > 2) {
    fprintf (stderr, "Usage: %s [memory area size in bytes]\n", argv[0]);
    exit (1);
  }
  else if (argc == 2)
    mem_size = atoi (argv[1]);

  Init (mem_size);

  for (i = 0;i<(mem_size); i++) {
    n = sprintf (s, "String 1, the current count is %d\n", i);
    rdtsc(&start);
    addr1 = Put (s, n+1);
    rdtsc(&finish);
    tot_alloc_time += finish - start;
    
    if (addr1){
    rdtsc(&start);
    Get ((any_t)data, addr1, n+1);
    rdtsc(&finish);
    tot_free_time += finish - start;
    }
    }

   // heap checker 

   printf("\n<<Part 1 for Region M1>>\n");
   printf("Number of allocated blocks: %ld\n",allocatedBlocks);

   printf("Number of free blocks: %ld\n",freeBlocks);
   printf("Raw total number of bytes allocated: %d\n",rawBytesAlloc);
   printf("Padded total number of bytes allocated: %d\n",paddedBlocks);
   numRawFreeBytes = mem_size-(12)-rawBytesAlloc;
   printf("Raw total number of bytes free: %d\n",numRawFreeBytes);
   alignFreeBytes = mem_size-(12)-paddedBlocks;
   printf("Aligned total number of bytes free: %d\n",alignFreeBytes);
   printf("Total number of Malloc requests: %d\n",totalMallocReq);
   printf("Total number of Free requests: %d\n",totalFreeReq);
   printf("Total number of request failures: %d\n",totalFailReq);
   printf("Average clock cycles for a Malloc request: %d\n",tot_alloc_time/totalMallocReq);
   int x;
   if(totalFreeReq == 0) {         //to avoid flaoting point exception we have to make sure denominator is not zero
        x = 0;
    }
    else {
        x = tot_free_time/totalFreeReq;
    }
   printf("Average clock cycles for a Free request: %d\n",x);
   printf("Total clock cycles for all requests: %d\n",(tot_free_time + tot_alloc_time)); 
}
