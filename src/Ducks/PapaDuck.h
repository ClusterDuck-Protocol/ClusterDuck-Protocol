#ifndef PAPADUCK_H
#define PAPADUCK_H

#include "Duck.h"
#include "../wifi/DuckWifi.h"

template <typename WifiCapability = DuckWifi, typename RadioType = DuckLoRa>
class PapaDuck : public Duck<WifiCapability, RadioType> {
public:
  using Duck<WifiCapability, RadioType>::Duck;
  
  PapaDuck(std::string name = "PAPADUCK") : Duck<WifiCapability, RadioType>(std::move(name)) {}
  ~PapaDuck() {}

  /// Papa Duck callback functions signature.
  using rxDoneCallback = void (*)(CdpPacket data);
  using txDoneCallback = void (*)(void);
  /**
   * @brief Register callback for handling data received from duck devices
   * 
   * The callback will be invoked if the packet needs to be relayed (i.e not seen before)
   * @param cb a callback to handle data received by the papa duck
   */
  void onReceiveDuckData(rxDoneCallback cb) { this->recvDataCallback = cb; }

  /**
   * @brief Get the DuckType
   *
   * @returns the duck type defined as DuckType
   */
  DuckType getType() { return DuckType::PAPA; }

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
    
    int err = this->sendReservedTopicData(this->dduid, reservedTopic::cmd, dataPayload);
  
    if (err != DUCK_ERR_NONE) {
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

  //remove this when mqtt quack pack is added
  bool isWifiConnected(){
    return this->duckWifi.connected();
  }

private:
  rxDoneCallback recvDataCallback;
  
  void handleReceivedPacket() {
        int err;
        std::optional<std::vector<uint8_t>> rxData = this->duckRadio.readReceivedData();
        if (!rxData) {
        logerr_ln("ERROR failed to get data from DuckRadio.");
        return;
        }
        CdpPacket rxPacket(rxData.value());
        logdbg_ln("Got data from radio, prepare for relay. size: %d",rxPacket.size());

        recvDataCallback(rxPacket.asBytes());
        loginfo_ln("handleReceivedPacket: packet RELAY START");

      if (rxPacket.topic == reservedTopic::ping) {
        loginfo_ln("PING received. Sending PONG!");
        err = this->sendPong();
        if (err != DUCK_ERR_NONE) {
          logerr_ln("ERROR failed to send pong message. rc = %d",err);
        }
      } else if (rxPacket.topic == reservedTopic::pong) {
        loginfo_ln("PONG received. Ignoring!");
      } else {
          err = this->broadcastPacket(rxPacket);
          
          // this->recvDataCallback(this->rxPacket->asBytes());
      }
      
      loginfo_ln("handleReceivedPacket() DONE");
    
    }
  };

#endif
