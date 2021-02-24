/**
 * @file DuckCrypto.h
 * @brief This file is internal to CDP and provides the library access to
 * encryption functions.
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
      bool encryptOn = false;

      uint8_t KEY[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
                        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};

      uint8_t IV[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
   }

   void setEncrypt(bool state);
   bool getState();

   void encryptData(uint8_t* text, uint8_t* encryptedData, size_t inc);
   void decryptData(uint8_t* encryptedData, uint8_t* text, size_t inc);

   void setAESKey(uint8_t newKEY[32]);
   void setAESIV(uint8_t newIV[16]);
   

};