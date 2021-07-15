#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>


struct BloomFilter;

struct BloomFilter * bloom_init(int m, int k, int w, int activeFilter, int maxN);
int bloom_check(struct BloomFilter* filter, unsigned char* msg, int msgSize);
void bloom_add(struct BloomFilter* filter, unsigned char* msg, int msgSize);

unsigned int djb2Hash(unsigned char* str, int seed, int msgSize);

