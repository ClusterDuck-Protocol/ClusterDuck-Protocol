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
  using rxDoneCallback = void (*)(CdpPacket data);
  
  /**
   * @brief Regsiter a callback for receiving and handling RSSI value
   *
   * @param rssiCb a call back defined with the following signature: `void (*)(const int)`
   */
  void onReceiveDuckData(rxDoneCallback cb) { this->recvDataCallback = cb; }

  /**
   * @brief Get the DuckType
   *
   * @returns the duck type defined as DuckType
   */
  DuckType getType() { return DuckType::DETECTOR; }

private:
  rxDoneCallback recvDataCallback;

  void handleReceivedPacket(CdpPacket rxPacket) {
    loginfo_ln("====> handleReceivedPacket: START");

    if (rxPacket.topic == reservedTopic::pong) {
      logdbg("run() - got ping response!");
      CdpPacket signalDataPacket = rxPacket;

      JsonDocument doc;
      doc["rssi"] = this->duckRadio.getRSSI();
      doc["snr"] = this->duckRadio.getSNR();

      std::string jsonString;
      serializeJson(doc, jsonString);

      signalDataPacket.data = std::vector<byte>(jsonString.begin(), jsonString.end());

      if (recvDataCallback) recvDataCallback(signalDataPacket);
    } 
  }
};
#endif