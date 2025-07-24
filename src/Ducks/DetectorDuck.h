#ifndef DETECTORDUCK_H
#define DETECTORDUCK_H

#include "Duck.h"

template <typename RadioType = DuckLoRa>
class DetectorDuck : public Duck<RadioType> {
public:
  using Duck<RadioType>::Duck;

  DetectorDuck(std::string name = "DETECTOR") : Duck<RadioType>(std::move(name)) {}
  ~DetectorDuck() {}
  
  /**
   * @brief Send a ping message to devices in the mesh network.
   * 
   * @param startReceive `true` if the device must to be ready to receive a response immediately,
   * `false` if response needs to be deffered. 
   */
  void sendPing() {
    loginfo("Sending PING...");
    int err = DUCK_ERR_NONE;
    std::vector<byte> data(1, 0);
    err = this->txPacket->prepareForSending(&this->filter, BROADCAST_DUID, DuckType::DETECTOR, reservedTopic::ping, data);
  
    if (err == DUCK_ERR_NONE) {
      err = this->duckRadio.sendData(this->txPacket->getBuffer());
    } else {
      logerr("ERROR Failed to build packet, err = %s\n",err);
    }
  };
  

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
  int getType() { return DuckType::DETECTOR; }

private:
  rssiCallback rssiCb;
  int setupWithDefaults(std::array<byte,8> deviceId, std::string ssid = "", std::string password = "") {
    int err = Duck<RadioType>::setupWithDefaults(deviceId, ssid, password);
    if (err != DUCK_ERR_NONE) {
      logerr("ERROR setupWithDefaults rc = %s\n",err);
      return err;
    }
  
    err = this->setupRadio();
    if (err != DUCK_ERR_NONE) {
      logerr("ERROR setupWithDefaults - setupRadio rc = %s\n",err);
      return err;
    }
  
    loginfo("DetectorDuck setup done");
    return DUCK_ERR_NONE;
  }
  
  void handleReceivedPacket() {
  
    loginfo("handleReceivedPacket()...");
  
    std::vector<byte> data;
    int err = this->duckRadio.readReceivedData(&data);
  
    if (err != DUCK_ERR_NONE) {
      logerr("ERROR Failed to get data from DuckRadio. rc = %s\n",err);
      return;
    }
  
    if (data[TOPIC_POS] == reservedTopic::pong) {
      logdbg("run() - got ping response!");
      rssiCb(this->duckRadio.getRSSI());
    }
  }
};
#endif