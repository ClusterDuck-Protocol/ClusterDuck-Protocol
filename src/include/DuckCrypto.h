#ifndef DUCK_CRYPTO_H
#define DUCK_CRYPTO_H

#include <Arduino.h>


class DuckCrypto {
public:
    static const uint8_t KEY_SIZE = 32;
    static const uint8_t IV_SIZE = 16;

    /*
     * @brief Get the singleton instance
     * @return DuckCrypto* The singleton instance
    */
    static DuckCrypto* getInstance();

    /* 
     * @brief Initialize the crypto with default key and IV
     * @param defaultKey The default encryption key
     * @param defaultIV The default encryption IV 
    */
    void init(const uint8_t defaultKey[KEY_SIZE], const uint8_t defaultIV[IV_SIZE]);

    /* 
     * @brief Set the encryption key
     * @param newKEY The new encryption key 
    */
    void setAESKey(const uint8_t newKEY[KEY_SIZE]);

    /*
     * @brief Set the encryption IV
     * @param newIV The new encryption IV 
    */
    void setAESIV(const uint8_t newIV[IV_SIZE]);

   /* 
    * @brief Encrypt data
    * @param text The data to encrypt
    * @param encryptedData The encrypted data
    * @param inc The size of the data
   */
    void encryptData(const uint8_t* text, uint8_t* encryptedData, size_t inc);

   /* 
    * @brief Decrypt data
    * @param encryptedData The encrypted data
    * @param text The decrypted data
    * @param inc The size of the data
   */
    void decryptData(const uint8_t* encryptedData, uint8_t* text, size_t inc);

private:
    // Singleton instance
    static DuckCrypto* instance;

    // Encryption key and IV
    uint8_t KEY[KEY_SIZE];
    uint8_t IV[IV_SIZE];

    // Private constructor to prevent instantiation
    DuckCrypto();
};

#endif // DUCK_CRYPTO_H