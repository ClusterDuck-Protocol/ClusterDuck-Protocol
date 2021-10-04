#include <memory>

#include "Arduino.h"
#include "include/bloomfilter.h"
#include "DuckLogger.h"



// two-phase bloom filter data sturcture 
struct BloomFilter {
   unsigned int* filter1;
   unsigned int* filter2;
   int activeFilter; // 1 or 2 
   int M; // power of 2
   int K;  
   int W; // power of 2 (less than M)
   int nMsg;
   int maxN;
   int* Seeds;
};


struct BloomFilter* bloom_init(int m, int k, int w, int maxN) {
    logdbg("bloom_init start");
    struct BloomFilter * BF = new BloomFilter;
    logdbg("BF created");
    logdbg(m);
    BF->M = m;
    logdbg("added to BF");
    BF->K = k;
    logdbg("added to BF");
    BF->W = w; //Max value is how many bits in an unsigned int
    logdbg("added to BF");
    BF->activeFilter = 1;
    BF->maxN = maxN;
    BF->nMsg = 0;

    int numRows = m; // how much space a bloom filter takes up in rows

    logdbg("initialize bloom filter 1");
    // Initialize the bloom filters, fill with 0's
    BF->filter1 = new unsigned int[numRows];
    printf("Filter 1 address %p\n", BF->filter1);
    if (BF->filter1 == NULL) {
        printf("Memory allocation for Bloom Filter 1 failed!\n");
        exit(0);
    }
    for (int n = 0; n < numRows; n++) {
        BF->filter1[n] = 0;
    }
    printf("Initialized BF1, %d slots, %d numRows\n", m, numRows);
    
    logdbg("initialize bloom filter 2");
    BF->filter2 = new unsigned int[numRows];
    printf("Filter 2 address %p\n", BF->filter2);
    if (BF->filter2 == NULL) {
        printf("Memory allocation for Bloom Filter 2 failed!\n");
        exit(0);
    }
    for (int n = 0; n < numRows; n++) {
        BF->filter2[n] = 0;
    }
    printf("Initialized BF2, %d slots, %d numRows\n", m, numRows);

    logdbg("initialize random seeds");
    //get random seeds for hash functions 
    int* Seeds = new int[k];
    if (Seeds == NULL) {
        printf("Memory allocation for seeds failed!\n");
        exit(0);
    }
    srand(time(NULL));
    int i;
    for (i = 0; i < k; i++){
        int r = rand();
        // Ensure we have no repeat seeds
        int seedCollision = 1;
        while (seedCollision == 1) {
            seedCollision = 0;
            for (int i = 0; i < k; i++){
                if (Seeds[i] == r) {
                    seedCollision = 1;
                }
            }
            if (seedCollision == 1){
                r = rand();
            }
        }
        Seeds[i] = r;
    }
    for (i = 0; i < k; i++){
        printf("random seed %d: %d\n", i+1, Seeds[i]);
    }
    BF->Seeds = Seeds;

    logdbg("bloom_init end");
    return BF;
}

int bloom_check(struct BloomFilter* filter, unsigned char* msg, int msgSize) {

    int m = filter->M;
    int k = filter->K;

    // generate hash indices for string
    std::unique_ptr<unsigned int[]> indSet(new unsigned int[k]); // array to hold string hash indices
    
    for (int i = 0; i < k; i++){
        unsigned int ind = djb2Hash(msg, filter->Seeds[i], msgSize);
        ind = ind % m;
        // ensure index to set is unique
        int indCollision; // collision flag; if it turns to 1, we try a new index 
        do {
            indCollision = 0; 
            for (int j = 0; j < i; j++){
                if (ind == indSet[j]) {
                    indCollision = 1;
                }
            }
            if (indCollision == 1) { // index already taken; try a new one
                ind = (ind + 1) % m;
            }
        } while(indCollision == 1);
        
        indSet[i] = ind; // the final index 
        
    }

    int filter1Collision = 1; // if BOTH these flags turn to 0-- we have a collision!
    int filter2Collision = 1; 
    for (int i = 0; i < k; i++) {

        int slotNum = indSet[i]; // the slot num (bit to set)
        int arrIndex = slotNum / filter->W; // index in BF array; the "row"
        int offset = slotNum % filter->W; // how far from the "right" end of the row the slot is

        // to set the bit: bitshift a leading 1 by "offset", then "OR" result with index "arrIndex" of BF! 
        unsigned int x = pow(2,(filter->W)-1); 
        x = x >> offset;
        
        // check filters for collisions
        if ((filter->filter1[arrIndex] & x) == 0){ // 0 at position of bit in this filter!
            filter1Collision = 0;
            
        }
        if ((filter->filter2[arrIndex] & x) == 0){ // 0 at position of bit in this filter!
            filter2Collision = 0;
        }
        // if filterCollision variables turn to 0, we know we have a new word!
        if (filter1Collision == 0 && filter2Collision == 0) {
            return 0;
        }
    }
    // possibly seen word before
    return 1;
    
}

void bloom_add(struct BloomFilter* filter, unsigned char* msg, int msgSize) {

    filter->nMsg += 1;

    int k = filter->K;
    int m = filter->M;

    // generate hash indices for string
    std::unique_ptr<unsigned int[]> indSet(new unsigned int[k]); // array to hold string hash indices
   
    for (int i = 0; i < k; i++){
      unsigned int ind = djb2Hash(msg, filter->Seeds[i], msgSize);
      ind = ind % m;
      // ensure index to set is unique
      int indCollision; // collision flag; if it turns to 1, we try a new index 
      do {
         indCollision = 0; 
         for (int j = 0; j < i; j++){
               if (ind == indSet[j]) {
                  indCollision = 1;
               }
         }
         if (indCollision == 1) { // index already taken; try a new one
            ind = (ind + 1) % m;
         }
      } while(indCollision == 1);
      
      indSet[i] = ind; // the final index 
      
    }
   // indSet array now contains all indices we need to set
   for (int i = 0; i < k; i++) {
      // printf("index %d: %d\n", i+1, indSet[i]);
   }
   // 2) set corresponding bits in bloom filter 
   for (int i = 0; i < k; i++) {

      int slotNum = indSet[i]; // the slot num (bit to set)
      int arrIndex = slotNum / filter->W; // index in BF array; the "row"
      int offset = slotNum % filter->W; // how far from the "right" end of the row the slot is

      // to set the bit: bitshift a leading 1 by "offset", then "OR" result with index "arrIndex" of BF! 
      unsigned int x = pow(2,(filter->W)-1); 
      x = x >> offset;

      unsigned int bitUpdate;
      if (filter->activeFilter == 1) {
         bitUpdate = filter->filter1[arrIndex] | x;
      }
      else{
         bitUpdate = filter->filter2[arrIndex] | x;
      }
      
      // set bit based on active filter
      if (filter->activeFilter == 1){ 
         filter->filter1[arrIndex] = bitUpdate;
      }
      else{
         filter->filter2[arrIndex] = bitUpdate;
      }
      
   }

    // switch filters if necessary 
    if (filter->nMsg >= filter->maxN) {
        if (filter->activeFilter == 1){
            printf("Freezing filter 1, switching to filter 2\n");
            // clear filter 2 and set it active
            for (int i = 0; i < (filter->M)/(filter->W); i++) {
                    filter->filter2[i] = 0;
            }
            filter->activeFilter = 2;
        }
        else{
            printf("Freezing filter 2, switching to filter 1\n");
            // clear filter 1 and set it active
            for (int i = 0; i < (filter->M)/(filter->W); i++) {
                    filter->filter1[i] = 0;
            }
            filter->activeFilter = 1;
        }
   filter->nMsg = 0;
    }

}

unsigned int djb2Hash(unsigned char* str, int seed, int msgSize){  // "version" of djb2Hash with seed
    unsigned int hash = seed; 
    int c;

   //TODO: test that for loop outputs the same value as while loop
   //  while ((c = *str++))
   //      hash = ((hash << 5) + hash) + c; // (hash * 33) + c 

   for(int i = 0; i < msgSize; i++) {
      c = *str++;
      hash = ((hash << 5) + hash) + c; // (hash * 33) + c
   }
    return hash;
}

   

