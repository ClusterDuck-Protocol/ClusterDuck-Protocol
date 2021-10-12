#include <memory>

#include "include/bloomfilter.h"
#include "DuckLogger.h"


BloomFilter::BloomFilter(int m, int k, int w, int maxN) {

    logdbg(m);
    this->M = m;
    logdbg("added to BF");
    this->K = k;
    logdbg("added to BF");
    this->W = w; //Max value is how many bits in an unsigned int
    logdbg("added to BF");
    this->activeFilter = 1;
    this->maxN = maxN;
    this->nMsg = 0;

    int numRows = m; // how much space a bloom filter takes up in rows

    logdbg("initialize bloom filter 1");
    // Initialize the bloom filters, fill with 0's
    this->filter1 = new unsigned int[numRows];
    logdbg_f("Filter 1 address %p\n", this->filter1);
    if (this->filter1 == NULL) {
        logdbg("Memory allocation for Bloom Filter 1 failed!\n");
        exit(0);
    }
    for (int n = 0; n < numRows; n++) {
        this->filter1[n] = 0;
    }
    logdbg_f("Initialized BF1, %d slots, %d numRows\n", m, numRows);
    
    logdbg("initialize bloom filter 2");
    this->filter2 = new unsigned int[numRows];
    logdbg_f("Filter 2 address %p\n", this->filter2);
    if (this->filter2 == NULL) {
        logdbg("Memory allocation for Bloom Filter 2 failed!\n");
        exit(0);
    }
    for (int n = 0; n < numRows; n++) {
        this->filter2[n] = 0;
    }
    logdbg_f("Initialized BF2, %d slots, %d numRows\n", m, numRows);

    logdbg("initialize random seeds");
    //get random seeds for hash functions 
    int* Seeds = new int[k];
    if (Seeds == NULL) {
        logdbg("Memory allocation for seeds failed!\n");
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
        logdbg_f("random seed %d: %d\n", i+1, Seeds[i]);
    }
    this->Seeds = Seeds;

    logdbg("bloom_init end");
}

BloomFilter::~BloomFilter()
{
    delete this->Seeds;
    delete this->filter2;
    delete this->filter1;
}

int BloomFilter::bloom_check(unsigned char* msg, int msgSize) {

    int m = this->M;
    int k = this->K;

    // generate hash indices for string
    std::unique_ptr<unsigned int[]> indSet(new unsigned int[k]); // array to hold string hash indices
    
    for (int i = 0; i < k; i++){
        unsigned int ind = djb2Hash(msg, this->Seeds[i], msgSize);
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
        int arrIndex = slotNum / this->W; // index in BF array; the "row"
        int offset = slotNum % this->W; // how far from the "right" end of the row the slot is

        // to set the bit: bitshift a leading 1 by "offset", then "OR" result with index "arrIndex" of BF! 
        unsigned int x = pow(2,(this->W)-1); 
        x = x >> offset;
        
        // check filters for collisions
        if ((this->filter1[arrIndex] & x) == 0){ // 0 at position of bit in this filter!
            filter1Collision = 0;
            
        }
        if ((this->filter2[arrIndex] & x) == 0){ // 0 at position of bit in this filter!
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

void BloomFilter::bloom_add(unsigned char* msg, int msgSize) {

    this->nMsg += 1;

    int k = this->K;
    int m = this->M;

    // generate hash indices for string
    std::unique_ptr<unsigned int[]> indSet(new unsigned int[k]); // array to hold string hash indices
   
    for (int i = 0; i < k; i++){
      unsigned int ind = djb2Hash(msg, this->Seeds[i], msgSize);
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
      // logdbg_f("index %d: %d\n", i+1, indSet[i]);
   }
   // 2) set corresponding bits in bloom filter 
   for (int i = 0; i < k; i++) {

      int slotNum = indSet[i]; // the slot num (bit to set)
      int arrIndex = slotNum / this->W; // index in BF array; the "row"
      int offset = slotNum % this->W; // how far from the "right" end of the row the slot is

      // to set the bit: bitshift a leading 1 by "offset", then "OR" result with index "arrIndex" of BF! 
      unsigned int x = pow(2,(this->W)-1); 
      x = x >> offset;

      unsigned int bitUpdate;
      if (this->activeFilter == 1) {
         bitUpdate = this->filter1[arrIndex] | x;
      }
      else{
         bitUpdate = this->filter2[arrIndex] | x;
      }
      
      // set bit based on active filter
      if (this->activeFilter == 1){ 
         this->filter1[arrIndex] = bitUpdate;
      }
      else{
         this->filter2[arrIndex] = bitUpdate;
      }
      
   }

    // switch filters if necessary 
    if (this->nMsg >= this->maxN) {
        if (this->activeFilter == 1){
            logdbg("Freezing filter 1, switching to filter 2\n");
            // clear filter 2 and set it active
            for (int i = 0; i < (this->M)/(this->W); i++) {
                    this->filter2[i] = 0;
            }
            this->activeFilter = 2;
        }
        else{
            logdbg("Freezing filter 2, switching to filter 1\n");
            // clear filter 1 and set it active
            for (int i = 0; i < (this->M)/(this->W); i++) {
                    this->filter1[i] = 0;
            }
            this->activeFilter = 1;
        }
        this->nMsg = 0;
    }

}

unsigned int BloomFilter::djb2Hash(unsigned char* str, int seed, int msgSize){  // "version" of djb2Hash with seed
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

   

