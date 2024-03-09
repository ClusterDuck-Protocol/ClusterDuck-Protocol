#include <unity.h>
#include "DuckLogger.h"

void test_DuckLoger_loginfo_ln(void) {
    loginfo_ln("Hello, World!");
}


void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_DuckLoger_loginfo_ln);
    UNITY_END();
}

void loop() {
}