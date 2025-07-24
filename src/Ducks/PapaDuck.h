#ifndef PAPADUCK_H
#define PAPADUCK_H

#include "Duck.h"

template <typename RadioType = DuckLoRa>
class PapaDuck : public Duck<RadioType> {
public:
  using Duck<RadioType>::Duck;
  
  PapaDuck(std::string name = "PAPADUCK") : Duck<RadioType>(std::move(name)) {}
  ~PapaDuck() {}

  /// Papa Duck callback functions signature.
  using rxDoneCallback = void (*)(std::vector<byte> data );
  using txDoneCallback = void (*)(void);
  /**
   * @brief Register callback for handling data received from duck devices
   * 
   * The callback will be invoked if the packet needs to be relayed (i.e not seen before)
   * @param cb a callback to handle data received by the papa duck
   */
  void onReceiveDuckData(rxDoneCallback cb) { this->recvDataCallback = cb; }

  /**
   * @brief Override the default setup method to match the Duck specific
   * defaults.
   *
   * In addition to Serial component, the Radio component is also initialized.
   * When ssid and password are provided the duck will setup the wifi related
   * components.
   *
   * @param ssid wifi access point ssid (defaults to an empty string if not
   * provided)
   * @param password wifi password (defaults to an empty string if not provided)
   * @returns DUCK_ERR_NONE if setup is successfull, an error code otherwise.
   */
   int setupWithDefaults(std::array<byte,8> deviceId, std::string ssid = "", std::string password = "") {
    loginfo_ln("setupWithDefaults...");
  
    int err = Duck<RadioType>::setupWithDefaults(deviceId, ssid, password);
  
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR setupWithDefaults rc = %s",err);
      return err;
    }
  
    err = this->setupRadio();
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR setupWithDefaults  rc = %d",err);
      return err;
    }
  
    std::string name(deviceId.begin(),deviceId.end());

      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR setupWithDefaults  rc = %d",err);
        return err;
      }
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR setupWithDefaults  rc = %d",err);
       
        return err;
      }  
  
    loginfo_ln("setupWithDefaults done");
    return DUCK_ERR_NONE;
  };

  /**
   * @brief Get the DuckType
   *
   * @returns the duck type defined as DuckType
   */
  int getType() { return DuckType::PAPA; }

  /**
   * @brief Send duck command to entire network
   *
   * The PapaDuck can send command messages to remote ducks. There are
   * two parts for each command. Only PapaDucks are able to send 
   * commands to remote ducks. See `MamaDuck.cpp` for available commands.
   * All ducks will execute this command and relay.
   *
   * @param cmd byte enum for command to be executed.
   * @param value contextual data to be used in executed command.
   */
  void sendCommand(byte cmd, std::vector<byte> value) {
    loginfo_ln("Initiate sending command");
    std::vector<byte> dataPayload;
    dataPayload.push_back(cmd);
    dataPayload.insert(dataPayload.end(), value.begin(), value.end());
  
    int err = this->txPacket->prepareForSending(&this->filter, this->dduid, DuckType::PAPA,
      reservedTopic::cmd, dataPayload);
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR handleReceivedPacket. Failed to prepare cmd. Error: %d",err);
    }
  
    err = this->duckRadio->sendData(this->txPacket->getBuffer());
  
    if (err == DUCK_ERR_NONE) {
      CdpPacket packet = CdpPacket(this->txPacket->getBuffer());
      this->filter.bloom_add(packet.muid.data(), MUID_LENGTH);
    } else {
      logerr_ln("ERROR handleReceivedPacket. Failed to send cmd. Error: %d",err);
    }
  };

  /**
   * @brief Send duck command to specific duck
   *
   * The PapaDuck can send command messages to remote ducks. There are
   * two parts for each command. Only PapaDucks are able to send 
   * commands to remote ducks. See `MamaDuck.cpp` for available commands. 
   *
   * @param cmd byte enum for command to be executed.
   * @param value contextual data to be used in executed command.
   * @param dduid destination duck ID for command to be executed.
   */
  void sendCommand(byte cmd, std::vector<byte> value, std::array<byte,8> dduid);

private:

  void handleReceivedPacket() {

    loginfo_ln("handleReceivedPacket() START");
    std::vector<uint8_t> data;
    int err = this->duckRadio->readReceivedData(&data);
  
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR handleReceivedPacket. Failed to get data. rc = %d",err);
      return;
    }
    
    if (data[TOPIC_POS] == reservedTopic::ping) {
      loginfo_ln("PING received. Sending PONG!");
      err = this->sendPong();
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR failed to send pong message. rc = %d",err);
      }
    } else if (data[TOPIC_POS] == reservedTopic::pong) {
      loginfo_ln("PONG received. Ignoring!");
    } else {
      // build our RX DuckPacket which holds the updated path in case the packet is relayed
      bool relay = this->rxPacket->prepareForRelaying(&this->filter, data);
      if (relay) {
        logdbg_ln("relaying:  %s", duckutils::convertToHex(this->rxPacket->getBuffer().data(), this->rxPacket->getBuffer().size()).c_str());
        loginfo_ln("invoking callback in the duck application...");
        
        this->recvDataCallback(this->rxPacket->getBuffer());
      }
          loginfo_ln("handleReceivedPacket() DONE");
          rxDoneCallback recvDataCallback;
    }
  

  }
};

#endif
