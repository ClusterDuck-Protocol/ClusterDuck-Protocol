#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

// two-phase bloom filter
class BloomFilter {
private:

  unsigned int* filter1;
  unsigned int* filter2;
  int activeFilter; // 1 or 2 
  int M; // power of 2
  int K;  
  int W; // power of 2 (less than M)
  int nMsg;
  int maxN;
  int* Seeds;

  static unsigned int djb2Hash(unsigned char* str, int seed, int msgSize);

public:

  friend class BloomFilterTester;

  /**
  * @brief Initialize a bloom filter
  * 
  * @param m, The Bloom filter size
  * @param k, The number of hash functions
  * @param w, The number of slots per array entry
  * @param maxN, The maximum number of messages until the next filter is used.
  */
  BloomFilter(int m, int k, int w, int maxN);

  ~BloomFilter();

  /**
   * @return 1 if we (possibly) found word; for a new word returns 0
   */
  int bloom_check(unsigned char* msg, int msgSize);

  void bloom_add(unsigned char* msg, int msgSize);

};

#endif
