#include "include/DuckCrypto.h"
#include "include/DuckUtils.h"

#include "Crypto.h"
#include "../../Crypto/src/AES.h" // some builds can't find this file so we need explicit path
#include "CTR.h"
#include "DuckLogger.h"

// Singleton instance
DuckCrypto* DuckCrypto::instance = nullptr;

// Private constructor
DuckCrypto::DuckCrypto() {}

// Get the singleton instance
DuckCrypto* DuckCrypto::getInstance() {
    if (!instance) {
        instance = new DuckCrypto();
    }
    return instance;
}

// Initialize the crypto with default key and IV
void DuckCrypto::init(const uint8_t defaultKey[KEY_SIZE], const uint8_t defaultIV[IV_SIZE]) {
    setAESKey(defaultKey);
    setAESIV(defaultIV);
}

// Set the encryption key
void DuckCrypto::setAESKey(const uint8_t newKEY[KEY_SIZE]) {
    for(int i = 0; i < KEY_SIZE; i++) {
        KEY[i] = newKEY[i];
    }
}

// Set the encryption IV
void DuckCrypto::setAESIV(const uint8_t newIV[IV_SIZE]) {
    for(int i = 0; i < IV_SIZE; i++) {
        IV[i] = newIV[i];
    }
}

// Encrypt data
void DuckCrypto::encryptData(const uint8_t* text, uint8_t* encryptedData, size_t inc) {
    CTR<AES256> ctraes256;
    ctraes256.clear();
    ctraes256.setKey(KEY, KEY_SIZE);
    ctraes256.setIV(IV, IV_SIZE);
    ctraes256.setCounterSize(4);

    size_t posn, len;
    long t1 = millis();

    for (posn = 0; posn < inc; posn += inc) {
        len = inc - posn;
        if (len > inc) len = inc;
        ctraes256.encrypt(encryptedData + posn, text + posn, len);
    }
    loginfo("Encrypt done in : %ld ms\n",(millis() - t1));
}

// Decrypt data
void DuckCrypto::decryptData(const uint8_t* encryptedData, uint8_t* text, size_t inc) {
    CTR<AES256> ctraes256;
    ctraes256.clear();
    ctraes256.setKey(KEY, KEY_SIZE);
    ctraes256.setIV(IV, IV_SIZE);
    ctraes256.setCounterSize(4);

    size_t posn, len;

    for (posn = 0; posn < inc; posn += inc) {
        len = inc - posn;
        if (len > inc) len = inc;
        ctraes256.encrypt(text + posn, encryptedData + posn, len);
    }
}