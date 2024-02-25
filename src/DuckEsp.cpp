#include "include/DuckEsp.h"

namespace duckesp {

#ifdef ESP32

  void restartDuck() { ESP.restart(); }
  int freeHeapMemory() {return ESP.getFreeHeap();}
  int getMinFreeHeap() { return ESP.getMinFreeHeap(); }
  int getMaxAllocHeap() { return ESP.getMaxAllocHeap(); }

  std::string getDuckMacAddress(boolean format) {
  char id1[15];
  char id2[15];

  uint64_t chipid = ESP.getEfuseMac(); // The chip ID is essentially its MAC
                                       // address(length: 6 bytes).
  uint16_t chip = (uint16_t)(chipid >> 32);

  snprintf(id1, 15, "%04X", chip);
  snprintf(id2, 15, "%08X", (uint32_t)chipid);

  std::string ID1 = id1;
  std::string ID2 = id2;

  std::string unformattedMac = ID1 + ID2;

  if (format == true) {
    std::string formattedMac = "";
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
#else
  void restartDuck() {}
  int freeHeapMemory() {return -1;}
  int getMinFreeHeap() {return -1;}
  int getMaxAllocHeap() {return -1;}
  std::string getDuckMacAddress(boolean format) {return "unknown";}
#endif
} // namespace duckesp