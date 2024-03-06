#include "include/bloomfilter.h"
#include "DuckLogger.h"


BloomFilter::BloomFilter(int numSectors, int numHashes, int bitsPerSector, int maxMsgs) {
    this->numSectors = numSectors; 
    this->numHashes = numHashes;
    this->bitsPerSector = bitsPerSector;
    this->activeFilter = 1;
    this->maxMsgs = maxMsgs;
    this->nMsg = 0;
    logdbg_ln("BloomFilter constructor: numSectors: %d, numHashes: %d, bitsPerSector: %d, maxMsgs: %d", 
               numSectors, numHashes, bitsPerSector, maxMsgs);

    logdbg("initialize bloom filter 1");
    // Initialize the bloom filters, fill with 0's
    this->filter1 = new unsigned int[this->numSectors];
    logdbg("Filter 1 address %p\n", this->filter1);
    if (this->filter1 == NULL) {
        logdbg("Memory allocation for Bloom Filter 1 failed!\n");
        exit(0);
    }
    for (int n = 0; n < this->numSectors; n++) {
        this->filter1[n] = 0;
    }
    logdbg("Initialized BF1, %d slots, %d this->numSectors\n", numSectors, this->numSectors);
    
    logdbg("initialize bloom filter 2");
    this->filter2 = new unsigned int[this->numSectors];
    logdbg("Filter 2 address %p\n", this->filter2);
    if (this->filter2 == NULL) {
        logdbg("Memory allocation for Bloom Filter 2 failed!\n");
        exit(0);
    }
    for (int n = 0; n < this->numSectors; n++) {
        this->filter2[n] = 0;
    }
    logdbg("Initialized BF2, %d slots, %d this->numSectors\n", numSectors, this->numSectors);

    logdbg("initialize random seeds");
    //get random seeds for hash functions 
    int* Seeds = new int[numHashes];
    if (Seeds == NULL) {
        logdbg("Memory allocation for seeds failed!\n");
        exit(0);
    }
    srand(time(NULL));
    bool seedCollision = true;
    bool collision;
    while (seedCollision) {
        collision = false;
        for (int i = 0; i < numHashes; i++) {
            int r = rand();
            if (Seeds[i] == r) {
                r = rand();
                collision = true;
                break;
            }
            Seeds[i] = r;
        }
        if (!collision) {
            seedCollision = false;
        }
    }

    for (int i = 0; i < numHashes; i++) {
        logdbg("random seed %d: %d\n", i+1, Seeds[i]);
    }
    this->Seeds = Seeds;

    logdbg("bloom_init end\n");
}

BloomFilter::~BloomFilter()
{
    delete this->Seeds;
    delete this->filter2;
    delete this->filter1;
}

void BloomFilter::set_hash_results(unsigned char* msg, int msgSize,
    std::unique_ptr<unsigned int[]> & hashResults
) {
    for (int i = 0; i < this->numHashes; i++){
        unsigned int hashResult = djb2Hash(msg, this->Seeds[i], msgSize);
        unsigned int totalBitSize = this->numSectors * this->bitsPerSector;
        hashResult = hashResult % totalBitSize;

        int hashCollision;
        do { 
            hashCollision = 0; 
            for (int j = 0; j < i; j++){
                if (hashResult == hashResults[j]) {
                    hashCollision = 1;
                } 
            }
            if (hashCollision == 1) {
                hashResult = (hashResult + 1) % totalBitSize;
            }
        } while(hashCollision == 1);

        hashResults[i] = hashResult;
    }
}

int BloomFilter::bloom_check(unsigned char* msg, int msgSize) {

    std::unique_ptr<unsigned int[]> hashResults(new unsigned int[this->numHashes]); 
    set_hash_results(msg, msgSize, hashResults);

    std::unique_ptr<int[]> sectors(new int[this->numHashes]);
    std::unique_ptr<unsigned int[]> slots(new unsigned int[this->numHashes]);
    set_sectors_and_slots(hashResults, sectors, slots);

    if (is_collision(filter1, sectors, slots)) {
        return 1;
    }
    if (is_collision(filter2, sectors, slots)) {
        return 1;
    }
    return 0;
}

int BloomFilter::is_collision(const unsigned int* filter,
    const std::unique_ptr<int[]> & sectors,
    const std::unique_ptr<unsigned int[]> & slots
) {
    for (int i = 0; i < this->numHashes; i++) {
        if ((filter[sectors[i]] & slots[i]) == 0) { //
            return 0;
        }
    }
    return 1;
}

void BloomFilter::set_sectors_and_slots(const std::unique_ptr<unsigned int[]> & hashResults,
    std::unique_ptr<int[]> & sectors, std::unique_ptr<unsigned int[]> & slots
) {
    for (int i = 0; i < this->numHashes; i++) {
        sectors[i] = hashResults[i] / this->bitsPerSector;
        int offset = hashResults[i] % this->bitsPerSector; 

        unsigned int x = pow(2,(this->bitsPerSector)-1); 
        slots[i] = x >> offset;
    }
}

void BloomFilter::bloom_add(unsigned char* msg, int msgSize) {

    this->nMsg += 1;

    std::unique_ptr<unsigned int[]> hashResults(new unsigned int[this->numHashes]);
    set_hash_results(msg, msgSize, hashResults);

    std::unique_ptr<int[]> sectors(new int[this->numHashes]);
    std::unique_ptr<unsigned int[]> slots(new unsigned int[this->numHashes]);
    set_sectors_and_slots(hashResults, sectors, slots);

    for (int i = 0; i < this->numHashes; i++){
      if (this->activeFilter == 1) {
         this->filter1[sectors[i]] = this->filter1[sectors[i]] | slots[i];
      }
      else{
         this->filter2[sectors[i]] = this->filter2[sectors[i]] | slots[i];
      }
    }
      
    if (this->nMsg >= this->maxMsgs) {
        if (this->activeFilter == 1){
            logdbg("Freezing filter 1, switching to filter 2\n");
            for (int i = 0; i < (this->numSectors)/(this->bitsPerSector); i++) {
                    this->filter2[i] = 0;
            }
            this->activeFilter = 2;
        }
        else{
            logdbg("Freezing filter 2, switching to filter 1\n");
            for (int i = 0; i < (this->numSectors)/(this->bitsPerSector); i++) {
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

   

