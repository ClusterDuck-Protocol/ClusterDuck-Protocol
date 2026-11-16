#ifndef DETECTORDUCK_H
#define DETECTORDUCK_H

#include "Duck.h"

template <typename WifiCapability = DuckWifiNone, typename RadioType = DuckLoRa>
class DetectorDuck : public Duck<DuckWifiNone, RadioType> {
public:
  using Duck<WifiCapability, RadioType>::Duck;

  DetectorDuck(std::string name = "DETECTOR") : Duck<DuckWifiNone, RadioType>(std::move(name)) {}
  ~DetectorDuck() {}

  /// callback definition for receiving the signal data of each pong
  using rxDoneCallback = void (*)(CdpPacket data);

  /**
   * @brief Register a callback for the signal data of each pong. The packet's
   * data is JSON such as {"rssi":-87.5,"snr":9.25}: RSSI in dBm, SNR in dB.
   *
   * @param cb a callback with the signature `void (*)(CdpPacket data)`
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
    logdbg_ln("====> handleReceivedPacket: START");

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