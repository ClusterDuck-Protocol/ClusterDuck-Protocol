#include "include/DuckCrypto.h"

// Initializing the cipher (CTR with AES256)
CTR<AES256> ctraes256;

// uint8_t KEY[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
//                   0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
//                   0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
//                   0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};

// uint8_t IV[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
//                   0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

// void encryptData(std::vector<uint8_t> text, std::vector<uint8_t> encryptedData, size_t inc)
// {
//   // Processing message for encryption
//   //int msgLength = msg.length() + 1;
//   //msg.getBytes(text, msgLength);
//   //Serial.println(sizeof(text));

//    size_t posn, len;

//    ctraes256.clear();
//    ctraes256.setKey(KEY, 32);
//    ctraes256.setIV(IV, 16);
//    ctraes256.setCounterSize(4);

//    for (posn = 0; posn < text.size(); posn += inc) {
//       len = text.size() - posn;
//       if (len > inc) len = inc;
//       ctraes256.encrypt(encryptedData, text, len);
//    }
// }

// int setAESKey(uint8_t newKEY[32]) {

//    return -1;
// }


// int setAESIV(uint8_t newIV[16]) {

//    return -1;
// }