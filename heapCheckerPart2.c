#include "pa32.c"



void main (int argc, char **argv) {

  int i, n;
  char s[80];
  addrs_t addr1, addr2,addr3;
  char data[80];
  int mem_size = 1000000; // Set DEFAULT_MEM_SIZE to 1<<20 bytes for a heap region
  unsigned long tot_alloc_time, tot_free_time;
  unsigned long start, finish;
  tot_alloc_time = 0;
  tot_free_time = 0;
  char s2[80];
  char data2[80];

  if  (argc > 2) {
    fprintf (stderr, "Usage: %s [memory area size in bytes]\n", argv[0]);
    exit (1);
  }
  else if (argc == 2)
    mem_size = atoi (argv[1]);

  VInit (mem_size);

  for (i = 0;i<(mem_size); i++) {
    n = sprintf (s2, "String 1, the current count is %d\n", i);
    rdtsc(&start);
    addr2 = VPut (s, n+1);
    rdtsc(&finish);
    tot_alloc_time += finish - start;
    
    if (addr2){
    rdtsc(&start);
    VGet ((any_t)data2, addr2, n+1);
    rdtsc(&finish);
    tot_free_time += finish - start;
    }
    }

   printf("\n<<Part 2 for Region M2>>\n");
   printf("Number of allocated blocks: %ld\n",allocatedBlocks);
   printf("Number of free blocks: %ld\n",freeBlocks);
   printf("Raw total number of bytes allocated: %d\n",rawBytesAlloc);
   printf("Padded total number of bytes allocated: %d\n",paddedBlocks);
   numRawFreeBytes = mem_size-(12)-rawBytesAlloc;
   printf("Raw total number of bytes free: %d\n",numRawFreeBytes);
   alignFreeBytes = mem_size-(12)-paddedBlocks;
   printf("Aligned total number of bytes free: %d\n",alignFreeBytes);
   printf("Total number of VMalloc requests: %d\n",totalMallocReq);
   printf("Total number of VFree requests: %d\n",totalFreeReq);
   printf("Total number of request failures: %d\n",totalFailReq);
   printf("Average clock cycles for a VMalloc request: %d\n",tot_alloc_time/totalMallocReq);
   
   int x; 
   if(totalFreeReq == 0) {         //to avoid flaoting point exception we have to make sure denominator is not zero
        x = 0;
    }
    else {
        x = tot_free_time/totalFreeReq;
    }
   printf("Average clock cycles for a VFree request: %d\n",x);
   printf("Total clock cycles for all requests: %d\n",(tot_free_time + tot_alloc_time)); 

}

