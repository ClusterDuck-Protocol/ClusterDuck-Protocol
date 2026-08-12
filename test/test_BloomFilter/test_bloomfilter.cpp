#include <Arduino.h>
#include <unity.h>
#include <string.h>
#include <stdio.h>

#include "routing/bloomfilter.h"

void setUp(void) {}
void tearDown(void) {}

// A fresh filter must not report a never-seen message as a duplicate.
void test_fresh_filter_reports_no_duplicates(void) {
  BloomFilter filter(DEFAULT_NUM_SECTORS, DEFAULT_NUM_HASH_FUNCS, DEFAULT_BITS_PER_SECTOR, DEFAULT_MAX_MESSAGES);

  unsigned char msg[] = "never-seen-message";
  TEST_ASSERT_EQUAL_INT(0, filter.bloom_check(msg, sizeof(msg)));
}

// Once a message is added, bloom_check() must report it as seen.
void test_added_message_is_detected_as_duplicate(void) {
  BloomFilter filter(DEFAULT_NUM_SECTORS, DEFAULT_NUM_HASH_FUNCS, DEFAULT_BITS_PER_SECTOR, DEFAULT_MAX_MESSAGES);

  unsigned char msg[] = "duplicate-message";
  filter.bloom_add(msg, sizeof(msg));
  TEST_ASSERT_EQUAL_INT(1, filter.bloom_check(msg, sizeof(msg)));
}

// Regression test for the reset-loop bug: bloom_add() used to clear only the
// first (numSectors / bitsPerSector) words of the inactive filter when
// switching, instead of all numSectors words. After a few switch cycles the
// unfilled tail of each filter stayed permanently set from earlier cycles,
// so bloom_check() eventually returned "seen" (1) for every fresh, never
// added message -- which made PapaDuck silently drop all incoming traffic
// after enough uptime. With the fix (full numSectors clear on switch), a
// brand new message must still be reported as not-seen after many cycles.
void test_fresh_message_not_falsely_flagged_after_many_filter_switches(void) {
  const int maxMsgs = 5;
  BloomFilter filter(DEFAULT_NUM_SECTORS, DEFAULT_NUM_HASH_FUNCS, DEFAULT_BITS_PER_SECTOR, maxMsgs);

  // Drive enough bloom_add() calls to cycle through several filter switches
  // (each switch happens every maxMsgs additions).
  const int numCycles = 10;
  for (int cycle = 0; cycle < numCycles; cycle++) {
    for (int i = 0; i < maxMsgs; i++) {
      char buf[32];
      snprintf(buf, sizeof(buf), "cycle-%d-msg-%d", cycle, i);
      filter.bloom_add((unsigned char*)buf, strlen(buf));
    }
  }

  unsigned char freshMsg[] = "still-never-seen-after-switches";
  TEST_ASSERT_EQUAL_INT(0, filter.bloom_check(freshMsg, sizeof(freshMsg)));
}

// The two-phase design keeps a message "seen" for one extra switch cycle
// (bloom_check() consults both the active and the previous filter). Confirm
// this rolling-window behavior still works correctly after the reset fix.
void test_recent_duplicate_still_detected_immediately_after_switch(void) {
  const int maxMsgs = 5;
  BloomFilter filter(DEFAULT_NUM_SECTORS, DEFAULT_NUM_HASH_FUNCS, DEFAULT_BITS_PER_SECTOR, maxMsgs);

  unsigned char recent[] = "recently-added-message";

  // Fill up to just below the switch threshold, then add "recent" as the
  // very last message of this cycle. This add() call triggers the switch.
  for (int i = 0; i < maxMsgs - 1; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "filler-%d", i);
    filter.bloom_add((unsigned char*)buf, strlen(buf));
  }
  filter.bloom_add(recent, sizeof(recent));

  TEST_ASSERT_EQUAL_INT(1, filter.bloom_check(recent, sizeof(recent)));
}

void setup() {
  delay(2000); // allow board / serial monitor to settle before running tests

  UNITY_BEGIN();
  RUN_TEST(test_fresh_filter_reports_no_duplicates);
  RUN_TEST(test_added_message_is_detected_as_duplicate);
  RUN_TEST(test_fresh_message_not_falsely_flagged_after_many_filter_switches);
  RUN_TEST(test_recent_duplicate_still_detected_immediately_after_switch);
  UNITY_END();
}

void loop() {}
