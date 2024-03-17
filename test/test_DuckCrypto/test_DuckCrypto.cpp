#include <unity.h>
#include <DuckCrypto.h>
#include <DuckLogger.h>

uint8_t clearTestString[32] = "This is a test string";
uint8_t encryptedTestString[32] = {0x0E,0x06,0x6D,0x24,0x28,0x92,0x02,0xB6,
                                       0x91,0x0E,0x21,0x58,0x71,0xB7,0x86,0xE1,
                                       0x14,0x81,0x79,0xBB,0xA4,0x85,0x58,0x5D,
                                       0x55,0x16,0xFB,0x51,0x72,0xE5,0x20,0xCF};


const uint8_t key[DuckCrypto::KEY_SIZE] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
                                         0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
                                         0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
                                         0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};

const uint8_t iv[DuckCrypto::IV_SIZE] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                       0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

void test_DuckCrypto_encryptData(void) {
    DuckCrypto* dc = DuckCrypto::getInstance();


    dc->init(key, iv);
    uint8_t encryptedData[32];
    dc->encryptData((uint8_t*)clearTestString, encryptedData, 32);
    for(int i = 0; i < 32; i++) {
        TEST_ASSERT_EQUAL(encryptedTestString[i], encryptedData[i]);
    }
}

void test_DuckCrypto_decryptData(void) {
    DuckCrypto* dc = DuckCrypto::getInstance();
    dc->init(key, iv);
    uint8_t decryptedData[32];
    dc->decryptData(encryptedTestString, decryptedData, 32);
    for(int i = 0; i < 32; i++) {
        TEST_ASSERT_EQUAL(clearTestString[i], decryptedData[i]);
    }
}
void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_DuckCrypto_encryptData);
    RUN_TEST(test_DuckCrypto_decryptData);
    UNITY_END();
}

void loop() {
}