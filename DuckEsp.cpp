#include "DuckEsp.h"

namespace duckesp {
    
void restartDuck() { ESP.restart(); }

String getDuckMacAddress(boolean format) {
  char id1[15];
  char id2[15];

  uint64_t chipid = ESP.getEfuseMac(); // The chip ID is essentially its MAC
                                       // address(length: 6 bytes).
  uint16_t chip = (uint16_t)(chipid >> 32);

  snprintf(id1, 15, "%04X", chip);
  snprintf(id2, 15, "%08X", (uint32_t)chipid);

  String ID1 = id1;
  String ID2 = id2;

  String unformattedMac = ID1 + ID2;

  if (format == true) {
    String formattedMac = "";
    for (int i = 0; i < unformattedMac.length(); i++) {
      if (i % 2 == 0 && i != 0) {
        formattedMac += ":";
        formattedMac += unformattedMac[i];
      } else {
        formattedMac += unformattedMac[i];
      }
    }
    return formattedMac;
  } else {
    return unformattedMac;
  }
  }
  } // namespace duckesp