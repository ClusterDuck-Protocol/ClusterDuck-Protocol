#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <memory>

// two-phase bloom filter
class BloomFilter {
private:

  unsigned int* filter1;
  unsigned int* filter2;
  int activeFilter; // 1 or 2 
  int numSectors; // power of 2
  int numHashes;  
  int bitsPerSector; //power of 2, smaller than numSectors
  int nMsg;
  int maxMsgs;
  int* Seeds;

  static unsigned int djb2Hash(unsigned char* str, int seed, int msgSize);

  /**
   * Generates a unique unsigned int index for each hash
   * 
   * @param msg, The message being hashed and converted into an index
   * @param msgSize, The size of msg
   * @param hashResults, Output as the bit index within the filter
   */
  void set_hash_results(unsigned char* msg, int msgSize,
    std::unique_ptr<unsigned int[]> & hashResults);

  void set_sectors_and_slots(const std::unique_ptr<unsigned int[]> & hashResults,
    std::unique_ptr<int[]> & sectors, std::unique_ptr<unsigned int[]> & slots
  );

  int is_collision(const unsigned int* filter,
    const std::unique_ptr<int[]> & sectors,
    const std::unique_ptr<unsigned int[]> & slots
  );

public:

  friend class BloomFilterTester;

  /**
  * @brief Initialize a bloom filter
  * 
  * @param numSectors, The number of sectors in filter
  * @param numHashes, The number of hash functions
  * @param bitsPerSector, The size of a sector in bits
  * @param maxMsgs, The maximum number of messages until the next filter is used.
  */
  BloomFilter(int numSectors, int numHashes, int bitsPerSector, int maxMsgs);

  ~BloomFilter();

  /**
   * @return 1 if we (possibly) found word; for a new word returns 0
   */
  int bloom_check(unsigned char* msg, int msgSize);

  void bloom_add(unsigned char* msg, int msgSize);

};

#endif
