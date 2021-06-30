#include "ClusterDuck.h"
#include "include/DuckEsp.h"

ClusterDuck::ClusterDuck() { duckutils::setInterrupt(true); }


// Get Duck MAC address
String ClusterDuck::duckMac(boolean format) {
  return duckesp::getDuckMacAddress(format);
}

// Create a uuid
String ClusterDuck::uuidCreator() { return duckutils::createUuid(); }


void ClusterDuck::flipInterrupt() {
  duckutils::setInterrupt(!duckutils::isInterruptEnabled());
}
