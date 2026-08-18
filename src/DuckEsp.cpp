#include "DuckEsp.h"

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
#elif defined(ARDUINO_ARCH_NRF52)

  // sd_softdevice_disable() was confirmed (via earlier bisection) to block
  // forever on this board instead of returning, so a software reset can never
  // be reached that way. Use the Watchdog Timer instead: it is independent
  // hardware that resets the chip unconditionally when it expires, with no
  // dependency on SoftDevice/interrupt state and nothing for software to
  // deadlock on.
  //
  // Diagnostic only: this board has exactly one real user LED (D11/P1.01 --
  // PIN_LED2/LED_BLUE is actually wired to the buzzer, not a second LED; see
  // variant.cpp's g_ADigitalPinMap comments, which are authoritative over
  // variant.h's stale "P1.15" comment). Hold that LED solidly on for several
  // seconds BEFORE touching the WDT at all, so reachability of this function
  // is unambiguous no matter what happens afterward.
  void restartDuck() {
    pinMode(LED_GREEN, OUTPUT);
    digitalWrite(LED_GREEN, LED_STATE_ON);  // solid ON = reached restartDuck()
    for (volatile uint32_t d = 0; d < 200000000u; d++) {}  // long hold, several seconds

    NRF_WDT->CONFIG = (WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos) |
                       (WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos);
    NRF_WDT->CRV = 32768;  // ~1s timeout (WDT runs off the 32.768kHz clock)
    NRF_WDT->RREN |= WDT_RREN_RR0_Msk;
    NRF_WDT->TASKS_START = 1;
    // LED stays solidly on (already set above) while waiting for the WDT to fire.
    // Do not feed/reload the watchdog -- let it expire and reset the chip.
    while (true) {}
  }
  int freeHeapMemory() {return -1;}
  int getMinFreeHeap() {return -1;}
  int getMaxAllocHeap() {return -1;}

  std::string getDuckMacAddress(boolean format) {
  // NRF_FICR->DEVICEADDR[0..1] holds the factory-programmed 48-bit Bluetooth
  // device address for this chip -- the nRF52 equivalent of a WiFi MAC address.
  char id1[15];
  char id2[15];

  snprintf(id1, 15, "%04X", (unsigned int)(NRF_FICR->DEVICEADDR[1] & 0xFFFF));
  snprintf(id2, 15, "%08X", (unsigned int)NRF_FICR->DEVICEADDR[0]);

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