#include <unity.h>
#include "bloomfilter.h"

void test_bloomfilter_constructor_default(void) {
    BloomFilter bf;
    TEST_ASSERT_EQUAL(DEFAULT_NUM_SECTORS, bf.get_numSectors());
    TEST_ASSERT_EQUAL(DEFAULT_NUM_HASH_FUNCS, bf.get_numHashes());
    TEST_ASSERT_EQUAL(DEFAULT_BITS_PER_SECTOR, bf.get_bitsPerSector());
    TEST_ASSERT_EQUAL(DEFAULT_MAX_MESSAGES, bf.get_maxMsgs());

    TEST_ASSERT_EQUAL(0, bf.get_nMsg());
}

void test_bloomfilter_constructor(void) {
    BloomFilter bf(10, 5, 16, 50);
    TEST_ASSERT_EQUAL(10, bf.get_numSectors());
    TEST_ASSERT_EQUAL(5, bf.get_numHashes());
    TEST_ASSERT_EQUAL(16, bf.get_bitsPerSector());
    TEST_ASSERT_EQUAL(50, bf.get_maxMsgs());

    TEST_ASSERT_EQUAL(0, bf.get_nMsg());
}

void test_bloomfilter_bloom_add(void) {
    BloomFilter bf;
    unsigned char msg[] = "ABCDE";
    int msgSize = sizeof(msg);
    bf.bloom_add(msg, msgSize);
    TEST_ASSERT_EQUAL(1, bf.get_nMsg());
}

void test_bloomfilter_bloom_check(void) {
    BloomFilter bf;
    unsigned char msg[] = "ABCDE";
    int msgSize = sizeof(msg);
    TEST_ASSERT_EQUAL(0, bf.bloom_check(msg, msgSize));
    bf.bloom_add(msg, msgSize);
    TEST_ASSERT_EQUAL(1, bf.bloom_check(msg, msgSize));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_bloomfilter_constructor_default);
    RUN_TEST(test_bloomfilter_constructor);
    RUN_TEST(test_bloomfilter_bloom_add);
    RUN_TEST(test_bloomfilter_bloom_check);
    UNITY_END();
}

void loop() {
}