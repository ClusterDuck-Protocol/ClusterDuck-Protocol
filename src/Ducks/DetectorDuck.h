#ifndef DETECTORDUCK_H
#define DETECTORDUCK_H

#include "Duck.h"

template <typename WifiCapability = DuckWifiNone, typename RadioType = DuckLoRa>
class DetectorDuck : public Duck<DuckWifiNone, RadioType> {
public:
  using Duck<WifiCapability, RadioType>::Duck;

  DetectorDuck(std::string name = "DETECTOR") : Duck<DuckWifiNone, RadioType>(std::move(name)) {}
  ~DetectorDuck() {}

  /// callback definition for receiving RSSI value
  using rssiCallback = void (*)(const int);
  
  /**
   * @brief Regsiter a callback for receiving and handling RSSI value
   *
   * @param rssiCb a call back defined with the following signature: `void (*)(const int)`
   */
  void onReceiveRssi(rssiCallback rssiCb) { this->rssiCb = rssiCb; }

  /**
   * @brief Get the DuckType
   *
   * @returns the duck type defined as DuckType
   */
  DuckType getType() { return DuckType::DETECTOR; }

private:
  rssiCallback rssiCb;

  void handleReceivedPacket() {
    loginfo_ln("====> handleReceivedPacket: START");

    int err;
    std::optional<std::vector<uint8_t>> rxData = this->duckRadio.readReceivedData();
    if (!rxData) {
    logerr_ln("ERROR failed to get data from DuckRadio.");
    return;
    }
    CdpPacket rxPacket(rxData.value());
    logdbg_ln("Got data from radio. size: %d",rxPacket.size());
  
    if (rxPacket.topic == reservedTopic::pong) {
      logdbg("run() - got ping response!");
      rssiCb(this->duckRadio.getRSSI());
    }
  }
};
#endif