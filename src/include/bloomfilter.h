#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>


struct BloomFilter;


/**
* @brief Initialize a bloom filter
* 
* @param m, The Bloom filter size
* @param k, The number of hash functions
* @param w, The number of slots per array entry
* @param maxN, The maximum number of messages until the next filter is used.
* 
* @returns An initialized Bloom filter, to be owned by the caller
*/
struct BloomFilter * bloom_init(int m, int k, int w, int maxN);
int bloom_check(struct BloomFilter* filter, unsigned char* msg, int msgSize);
void bloom_add(struct BloomFilter* filter, unsigned char* msg, int msgSize);

unsigned int djb2Hash(unsigned char* str, int seed, int msgSize);

