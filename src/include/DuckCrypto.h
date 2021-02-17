/**
 * @file DuckCrypto.h
 * @brief This file is internal to CDP and provides the library access to
 * encryption functions.
 *
 * The implementation is conditioned by the `CDPCFG_ENCRYPTION_NONE` flag which may be
 * defined in `cdpcfh.h` to disable encryption.
 * @version
 * @date 2021-02-10
 *
 * @copyright
 */

// Crypto Libraries
#include <Crypto.h>
#include <AES.h>
#include <CTR.h>
#include <vector>
#include <string>


class DuckCrypto {

public:
   int setAESKey(uint8_t newKEY[32]);
   int setAESIV(uint8_t newIV[16]);

   void encryptData(std::uint8_t* text, std::uint8_t* encryptedData, std::size_t inc);

private:
   uint8_t KEY[32];
   uint8_t IV[16];

};