#include "include/DuckCrypto.h"
#include "include/DuckUtils.h"

#include "Crypto.h"
#include "../../Crypto/src/AES.h" // some builds can't find this file so we need explicit path
#include "CTR.h"

namespace duckcrypto {

   // Initializing the cipher (CTR with AES256)
   CTR<AES256> ctraes256;

   void setEncrypt(bool state) {
      encryptOn = state;
   }

   bool getState() { return encryptOn; }

   void setDecrypt(bool state) {
      decryptOn = state;
   }

   bool getDecrypt() { return decryptOn; }


   void encryptData(uint8_t* text, uint8_t* encryptedData, size_t inc)
   {
      
      size_t posn, len;
      long t1 = millis();

      ctraes256.clear();
      ctraes256.setKey(KEY, 32);
      ctraes256.setIV(IV, 16);
      ctraes256.setCounterSize(4);

      for (posn = 0; posn < inc; posn += inc) {
         len = inc - posn;
         if (len > inc) len = inc;
         ctraes256.encrypt(encryptedData + posn, text + posn, len);
      }
      loginfo("Encrypt done in : %ld ms\n",(millis() - t1));
   }

   void decryptData(uint8_t* encryptedData, uint8_t* text, size_t inc) {

      size_t posn, len;

      ctraes256.clear();
      ctraes256.setKey(KEY, 32);
      ctraes256.setIV(IV, 16);
      ctraes256.setCounterSize(4);

      for (posn = 0; posn < inc; posn += inc) {
         len = inc - posn;
         if (len > inc) len = inc;
         ctraes256.encrypt(text + posn, encryptedData + posn, len);
      }
   }

   void setAESKey(uint8_t newKEY[32]) {
      
      for(int i = 0; i < 32; i++) {
         KEY[i] = newKEY[i];
      }

   }

   void setAESIV(uint8_t newIV[16]) {
      
      for(int i = 0; i < 16; i++) {
         IV[i] = newIV[i];
      }
      
   }
}
