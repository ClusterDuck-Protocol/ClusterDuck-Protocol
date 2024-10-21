// test/test_CdpPacket/test_CdpPacket.cpp
#include <unity.h>
#include "DuckUtils.h"

void test_DuckUtils_getCdpVersion(void) {
    // expected version as a string
    std::string expectedVersion = std::to_string(CDP_VERSION_MAJOR) + "." + 
                                  std::to_string(CDP_VERSION_MINOR) + "." + 
                                  std::to_string(CDP_VERSION_PATCH);

    std::string version = duckutils::getCDPVersion();
    
    TEST_ASSERT_EQUAL_STRING(expectedVersion.c_str(), version.c_str());
}

void test_DuckUtils_toUpperCase(void) {
    std::string str = "hello";
    std::string upper = duckutils::toUpperCase(str);
    TEST_ASSERT_EQUAL_STRING("HELLO", upper.c_str());

    std::string empty = duckutils::toUpperCase("");
    TEST_ASSERT_EQUAL_STRING("", empty.c_str());

    std::string mixed = duckutils::toUpperCase("HeLlO");
    TEST_ASSERT_EQUAL_STRING("HELLO", mixed.c_str());

    std::string special = duckutils::toUpperCase("hElLo!");
    TEST_ASSERT_EQUAL_STRING("HELLO!", special.c_str());

    std::string numbers = duckutils::toUpperCase("12345");
    TEST_ASSERT_EQUAL_STRING("12345", numbers.c_str());

    std::string mixedSpecial = duckutils::toUpperCase("HeLlO!12345");
    TEST_ASSERT_EQUAL_STRING("HELLO!12345", mixedSpecial.c_str());

    std::string alreadyUpperCase = duckutils::toUpperCase("HELLO");
    TEST_ASSERT_EQUAL_STRING("HELLO", alreadyUpperCase.c_str());
}

void test_DuckUtils_stringToByteVector(void) {
    std::string str = "hello";
    std::vector<byte> bytes = duckutils::stringToByteVector(str);
    TEST_ASSERT_EQUAL(5, bytes.size());
    TEST_ASSERT_EQUAL('h', bytes[0]);
    TEST_ASSERT_EQUAL('e', bytes[1]);
    TEST_ASSERT_EQUAL('l', bytes[2]);
    TEST_ASSERT_EQUAL('l', bytes[3]);
    TEST_ASSERT_EQUAL('o', bytes[4]);
}

void test_DuckUtils_getRandomBytes(void) {
    byte bytes[10];
    duckutils::getRandomBytes(10, bytes);
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_TRUE(bytes[i] >= '0' && bytes[i] <= 'Z');
    }
}

void test_DuckUtils_getRandomBytes_with_zero_length(void) {
    byte bytes[0];
    duckutils::getRandomBytes(0, bytes);
    TEST_ASSERT_EQUAL(0, sizeof(bytes));
}

void test_DuckUtils_createUuid_with_given_length(void) {
    std::string uuid = duckutils::createUuid(36);
    TEST_ASSERT_EQUAL(36, uuid.length());
    for (int i = 0; i < 36; i++) {
        char c = uuid[i];
        TEST_ASSERT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z'));
    }
}

void test_DuckUtils_createUuid_with_default_length(void) {
    std::string uuid = duckutils::createUuid();
    TEST_ASSERT_EQUAL(CDPCFG_UUID_LEN, uuid.length());
    for (int i = 0; i < CDPCFG_UUID_LEN; i++) {
        char c = uuid[i];
        TEST_ASSERT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z'));
    }
}

void test_DuckUtils_convertToHex(void) {
    byte data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    std::string hex = duckutils::convertToHex(data, 5);
    TEST_ASSERT_EQUAL_STRING("0102030405", hex.c_str());

    byte empty[] = {};
    std::string emptyHex = duckutils::convertToHex(empty, 0);
    TEST_ASSERT_EQUAL_STRING("", emptyHex.c_str());

    byte single[] = {0x01};
    std::string singleHex = duckutils::convertToHex(single, 1);
    TEST_ASSERT_EQUAL_STRING("01", singleHex.c_str());

    byte large[] = {0xEF, 0xFF, 0x00, 0xAB, 0xCD, 0x99, 0x12};
    std::string largeHex = duckutils::convertToHex(large, 7);
    TEST_ASSERT_EQUAL_STRING("EFFF00ABCD9912", largeHex.c_str());

    byte zeros[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    std::string zerosHex = duckutils::convertToHex(zeros, 8);
    TEST_ASSERT_EQUAL_STRING("0000000000000000", zerosHex.c_str());
}

void test_DuckUtils_toString_printable_characters() {
    std::vector<byte> vec = {0x48, 0x45, 0x4C, 0x4C, 0x4F};
    std::string str = duckutils::toString(vec);
    TEST_ASSERT_EQUAL_STRING("HELLO", str.c_str());

}

void test_DuckUtils_toString_non_printable_characters(void) {
    std::vector<byte> vec = {0x01, 0x02, 0x03, 0x04, 0x05};
    std::string str = duckutils::toString(vec);
    TEST_ASSERT_EQUAL_STRING("ERROR: Non-printable character", str.c_str());
}


void test_DuckUtils_isEqual_true(void) {
    std::vector<byte> vec1 = {0x01, 0x02, 0x03, 0x04, 0x05};
    std::vector<byte> vec2 = {0x01, 0x02, 0x03, 0x04, 0x05};
    TEST_ASSERT_TRUE(duckutils::isEqual(vec1, vec2));
}

void test_DuckUtils_isEqual_false(void) {
    std::vector<byte> vec1 = {0x01, 0x02, 0x03, 0x04, 0x05};
    std::vector<byte> vec2 = {0x01, 0x02, 0x03, 0x04, 0x06};
    TEST_ASSERT_FALSE(duckutils::isEqual(vec1, vec2));
}

void test_DuckUtils_isEqual_false_with_different_sizes(void) {
    std::vector<byte> vec1 = {0x01, 0x02, 0x03, 0x04, 0x05};
    std::vector<byte> vec2 = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    TEST_ASSERT_FALSE(duckutils::isEqual(vec1, vec2));
}

void test_DuckUtils_toUint32(void) {
    std::vector<byte> vec = {0x01, 0x02, 0x03, 0x04};
    uint32_t value = duckutils::toUint32(&vec[0]);
    TEST_ASSERT_EQUAL(0x01020304, value);
}

void test_DuckUtils_saveWifiCredentials(void) {
    int rc = duckutils::saveWifiCredentials("ssid", "password");
#ifdef CDPCFG_WIFI_NONE
    TEST_ASSERT_EQUAL(DUCK_ERR_NOT_SUPPORTED, rc);
#else
    TEST_ASSERT_EQUAL(DUCK_ERR_NONE, rc);
#endif
}

void test_DuckUtils_saveWifiCredentials_zero_length(void) {
    int rc = duckutils::saveWifiCredentials("", "password");
#ifdef CDPCFG_WIFI_NONE    
    TEST_ASSERT_EQUAL(DUCK_ERR_NOT_SUPPORTED, rc);
#else
    TEST_ASSERT_EQUAL(DUCK_ERR_INVALID_ARGUMENT, rc);
    rc = duckutils::saveWifiCredentials("ssid", "");
    TEST_ASSERT_EQUAL(DUCK_ERR_INVALID_ARGUMENT, rc);
    rc = duckutils::saveWifiCredentials("", "");
    TEST_ASSERT_EQUAL(DUCK_ERR_INVALID_ARGUMENT, rc);
#endif
}

void test_DuckUtils_loadWifiSsid(void) {

    int rc = duckutils::saveWifiCredentials("ssid", "password");
#ifdef CDPCFG_WIFI_NONE
    TEST_ASSERT_EQUAL(DUCK_ERR_NOT_SUPPORTED, rc);    
#else
    TEST_ASSERT_EQUAL(DUCK_ERR_NONE, rc);
#endif

    std::string ssid = duckutils::loadWifiSsid();
#ifdef CDPCFG_WIFI_NONE
    TEST_ASSERT_EQUAL_STRING("unknown", ssid.c_str());
#else
    TEST_ASSERT_EQUAL_STRING("ssid", ssid.c_str());
#endif    
}


void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_DuckUtils_getCdpVersion);
    RUN_TEST(test_DuckUtils_toUpperCase);
    RUN_TEST(test_DuckUtils_stringToByteVector);
    RUN_TEST(test_DuckUtils_getRandomBytes);
    RUN_TEST(test_DuckUtils_createUuid_with_given_length);
    RUN_TEST(test_DuckUtils_createUuid_with_default_length);
    RUN_TEST(test_DuckUtils_convertToHex);
    RUN_TEST(test_DuckUtils_toString_printable_characters);
    RUN_TEST(test_DuckUtils_toString_non_printable_characters);
    RUN_TEST(test_DuckUtils_isEqual_true);
    RUN_TEST(test_DuckUtils_isEqual_false);
    RUN_TEST(test_DuckUtils_isEqual_false_with_different_sizes);
    RUN_TEST(test_DuckUtils_toUint32);
    RUN_TEST(test_DuckUtils_saveWifiCredentials);
    RUN_TEST(test_DuckUtils_saveWifiCredentials_zero_length);
    RUN_TEST(test_DuckUtils_loadWifiSsid);
    UNITY_END();
}

void loop() {
}