#include <assert.h>

#include "src/include/bloomfilter.h"

#define MUID_LENGTH 4 // TODO: Remove, and use CdpPacket.h's definition instead.

class BloomFilterTester{
public:

  static void SetSeed(BloomFilter & filter, unsigned seedIndex, int seed) {
    filter.Seeds[seedIndex] = seed;
  }

  static unsigned int djb2Hash(unsigned char* str, int seed, int msgSize) {
    return BloomFilter::djb2Hash(str, seed, msgSize);
  }

  static unsigned int* filter1(BloomFilter & filter) {
    return filter.filter1;
  }

  static unsigned int* filter2(BloomFilter & filter) {
    return filter.filter2;
  }

  static int activeFilter(BloomFilter & filter) {
    return filter.activeFilter;
  }

  static int numSectors(BloomFilter & filter) {
    return filter.numSectors;
  }

  static int numHashes(BloomFilter & filter) {
    return filter.numHashes;
  }

  static int bitsPerSector(BloomFilter & filter) {
    return filter.bitsPerSector;
  }

  static int nMsg(BloomFilter & filter) {
    return filter.nMsg;
  }

  static int maxMsgs(BloomFilter & filter) {
    return filter.maxMsgs;
  }

  static int* Seeds(BloomFilter & filter) {
    return filter.Seeds;
  }

};

int main() {
  const int NUM_SECTORS = 16;
  const int NUM_HASH_FUNCS = 2;
  const int BITS_PER_SECTOR = 32;
  const int MAX_MESSAGES = 8;
  BloomFilter filter = BloomFilter(NUM_SECTORS, NUM_HASH_FUNCS,
    BITS_PER_SECTOR, MAX_MESSAGES);

  const int SEED_0 = 0;
  const int SEED_1 = 1;
  BloomFilterTester::SetSeed(filter, 0, SEED_0);
  BloomFilterTester::SetSeed(filter, 1, SEED_1);

  assert(BloomFilterTester::activeFilter(filter) == 1);
  assert(BloomFilterTester::nMsg(filter) == 0);
  unsigned int * filter1 = BloomFilterTester::filter1(filter);
  for (int i = 0; i < NUM_SECTORS; i++) {
    assert(filter1[i] == 0);
  }


  unsigned char MUID00[MUID_LENGTH] = {'T', 'S', 'T', '0'};
  unsigned char MUID01[MUID_LENGTH] = {'T', 'S', 'T', '1'};
  unsigned char MUID02[MUID_LENGTH] = {'T', 'S', 'T', '2'};
  unsigned char MUID03[MUID_LENGTH] = {'T', 'S', 'T', '3'};
  unsigned char MUID04[MUID_LENGTH] = {'T', 'S', 'T', '4'};
  unsigned char MUID05[MUID_LENGTH] = {'T', 'S', 'T', '5'};
  unsigned char MUID06[MUID_LENGTH] = {'T', 'S', 'T', '6'};
  unsigned char MUID07[MUID_LENGTH] = {'T', 'S', 'T', '7'};
  unsigned char MUID08[MUID_LENGTH] = {'T', 'S', 'T', '8'};
  unsigned char MUID09[MUID_LENGTH] = {'T', 'S', 'T', '9'};
  unsigned char MUID10[MUID_LENGTH] = {'T', 'S', '1', '0'};
  unsigned char MUID11[MUID_LENGTH] = {'T', 'S', '1', '1'};
  unsigned char MUID12[MUID_LENGTH] = {'T', 'S', '1', '2'};
  unsigned char MUID13[MUID_LENGTH] = {'T', 'S', '1', '3'};
  unsigned char MUID14[MUID_LENGTH] = {'T', 'S', '1', '4'};
  unsigned char MUID15[MUID_LENGTH] = {'T', 'S', '1', '5'};

  // const unsigned int HASH_MUID00_SEED_0 = 3111915;
  // const unsigned int HASH_MUID00_SEED_1 = 4297836;
  // const unsigned int HASH_MUID01_SEED_0 = 3111916;
  // const unsigned int HASH_MUID01_SEED_1 = 4297837;
  // const unsigned int HASH_MUID02_SEED_0 = 3111917;
  // const unsigned int HASH_MUID02_SEED_1 = 4297838;
  // const unsigned int HASH_MUID03_SEED_0 = 3111918;
  // const unsigned int HASH_MUID03_SEED_1 = 4297839;

  assert(!filter.bloom_check(MUID00, MUID_LENGTH));
  filter.bloom_add(MUID00, MUID_LENGTH);
  assert(filter.bloom_check(MUID00, MUID_LENGTH));
  assert(BloomFilterTester::activeFilter(filter) == 1);
  assert(BloomFilterTester::nMsg(filter) == 1);

  assert(!filter.bloom_check(MUID01, MUID_LENGTH));
  filter.bloom_add(MUID01, MUID_LENGTH);
  assert(filter.bloom_check(MUID01, MUID_LENGTH));

  assert(!filter.bloom_check(MUID02, MUID_LENGTH));
  filter.bloom_add(MUID02, MUID_LENGTH);
  assert(filter.bloom_check(MUID02, MUID_LENGTH));

  assert(!filter.bloom_check(MUID03, MUID_LENGTH));
  filter.bloom_add(MUID03, MUID_LENGTH);
  assert(filter.bloom_check(MUID03, MUID_LENGTH));

  assert(!filter.bloom_check(MUID04, MUID_LENGTH));
  filter.bloom_add(MUID04, MUID_LENGTH);
  assert(filter.bloom_check(MUID04, MUID_LENGTH));

  assert(!filter.bloom_check(MUID05, MUID_LENGTH));
  filter.bloom_add(MUID05, MUID_LENGTH);
  assert(filter.bloom_check(MUID05, MUID_LENGTH));

  assert(!filter.bloom_check(MUID06, MUID_LENGTH));
  filter.bloom_add(MUID06, MUID_LENGTH);
  assert(filter.bloom_check(MUID06, MUID_LENGTH));
  assert(BloomFilterTester::activeFilter(filter) == 1);
  assert(BloomFilterTester::nMsg(filter) == 7);

  assert(!filter.bloom_check(MUID07, MUID_LENGTH));
  filter.bloom_add(MUID07, MUID_LENGTH);
  assert(filter.bloom_check(MUID07, MUID_LENGTH));
  assert(BloomFilterTester::activeFilter(filter) == 2);
  assert(BloomFilterTester::nMsg(filter) == 0);

  // Switch to other filter

  assert(!filter.bloom_check(MUID08, MUID_LENGTH));
  filter.bloom_add(MUID08, MUID_LENGTH);
  assert(filter.bloom_check(MUID08, MUID_LENGTH));
  assert(BloomFilterTester::activeFilter(filter) == 2);
  assert(BloomFilterTester::nMsg(filter) == 1);

  assert(!filter.bloom_check(MUID09, MUID_LENGTH));
  filter.bloom_add(MUID09, MUID_LENGTH);
  assert(filter.bloom_check(MUID09, MUID_LENGTH));

  assert(!filter.bloom_check(MUID10, MUID_LENGTH));
  filter.bloom_add(MUID10, MUID_LENGTH);
  assert(filter.bloom_check(MUID10, MUID_LENGTH));

  assert(!filter.bloom_check(MUID11, MUID_LENGTH));
  filter.bloom_add(MUID11, MUID_LENGTH);
  assert(filter.bloom_check(MUID11, MUID_LENGTH));

  assert(!filter.bloom_check(MUID12, MUID_LENGTH));
  filter.bloom_add(MUID12, MUID_LENGTH);
  assert(filter.bloom_check(MUID12, MUID_LENGTH));

  assert(!filter.bloom_check(MUID13, MUID_LENGTH));
  filter.bloom_add(MUID13, MUID_LENGTH);
  assert(filter.bloom_check(MUID13, MUID_LENGTH));

  assert(!filter.bloom_check(MUID14, MUID_LENGTH));
  filter.bloom_add(MUID14, MUID_LENGTH);
  assert(filter.bloom_check(MUID14, MUID_LENGTH));

  assert(!filter.bloom_check(MUID15, MUID_LENGTH));
  filter.bloom_add(MUID15, MUID_LENGTH);
  assert(filter.bloom_check(MUID15, MUID_LENGTH));

}
