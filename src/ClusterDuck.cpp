#include "ClusterDuck.h"
#include "include/DuckEsp.h"

ClusterDuck::ClusterDuck() { duckutils::setDuckInterrupt(true); }


// Get Duck MAC address
String ClusterDuck::duckMac(boolean format) {
  return duckesp::getDuckMacAddress(format);
}

// Create a uuid
String ClusterDuck::uuidCreator() { return duckutils::createUuid(); }


void ClusterDuck::flipInterrupt() {
  duckutils::setDuckInterrupt(!duckutils::getDuckInterrupt());
}

//TODO: Move this in a separate module. Either DuckOta or DuckNet
#ifndef CDPCFG_WIFI_NONE
// handle the upload of the firmware
void handleFirmwareUpload(AsyncWebServerRequest* request, String filename,
                          size_t index, uint8_t* data, size_t lenght,
                          bool final) {
  // handle upload and update
  if (!index) {
    //FIXME: printf may not be implemented on some Arduiono platforms
    Serial.printf("Update: %s\n", filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { // start with max available size
      Update.printError(Serial);
    }
  }
  // flashing firmware to ESP
  if (lenght) {
    Update.write(data, lenght);
  }
  if (final) {
    if (Update.end(true)) { // true to set the size to the current progress
      Serial.printf("Update Success: %ub written\nRebooting...\n",
                    index + lenght);
    } else {
      Update.printError(Serial);
    }
  }
}
#endif