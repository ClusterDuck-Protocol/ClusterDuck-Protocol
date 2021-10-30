/**
 * @file DuckCrypto.h
 * @brief This file is internal to CDP and provides the library access to
 * encryption functions. Currently only supports AES-256
 *
 * @version
 * @date 2021-02-10
 *
 * @copyright
 */

// Crypto Libraries
#include <Crypto.h>
#include <AES.h>
#include <CTR.h>
#include "../DuckLogger.h"


namespace duckcrypto {
   
   namespace {
      /**
       * @brief Enable or disable encryption flag.
       *
       */
      bool encryptOn = false;

      /**
       * @brief Enable or disable decryption flag.
       *
       */
      bool decryptOn = false;

      /**
       * @brief Default encryption key, can be set in application layer using `void setAESKey(uint8_t newKEY[32])`.
       *
       */
      uint8_t KEY[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
                        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};

      /**
       * @brief Default encryption IV, can be set in application layer using `void setAESIV(uint8_t newIV[16])`.
       *
       */
      uint8_t IV[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
   }

   /**
    * @brief Setter for encryption flag.
    * 
    * @param state sets `encryptOn` flag. 
    */
   void setEncrypt(bool state);

   /**
    * @brief Getter for encryption flag.
    * 
    */
   bool getState();

   /**
    * @brief Setter for decryption flag.
    * 
    * @param state sets `decryptOn` flag. 
    */
   void setDecrypt(bool state);

   /**
    * @brief Getter for decryption flag.
    * 
    */
   bool getDecrypt();

   /**
    * @brief Encrypt data function.
    * 
    * @param text pointer for data to be encrypted. 
    * @param encryptedData pointer for where encrypted data should be stored.
    * @param inc .
    */
   void encryptData(uint8_t* text, uint8_t* encryptedData, size_t inc);

   /**
    * @brief Encrypt data function.
    * 
    * @param encryptedData pointer for data to be decrypted. 
    * @param text pointer for where decrypted data should be stored.
    * @param inc .
    */
   void decryptData(uint8_t* encryptedData, uint8_t* text, size_t inc);

   /**
    * @brief Setter encryption key.
    * 
    * @param newKEY sets key to be used for encryption. 
    */
   void setAESKey(uint8_t newKEY[32]);

   /**
    * @brief Setter encryption IV.
    * 
    * @param newIV sets IV to be used for encryption. 
    */
   void setAESIV(uint8_t newIV[16]);
   

};