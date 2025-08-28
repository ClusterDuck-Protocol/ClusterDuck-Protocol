#ifndef DUCK_H
#define DUCK_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../utils/DuckError.h"
#include "../include/cdpcfg.h"
#include "../radio/DuckLoRa.h"
#include "DuckTypes.h"
#include "../utils/DuckUtils.h"
#include <cassert>
#include "../CdpPacket.h"
#include "../DuckEsp.h"
#include "../wifi/DuckWifiNone.h"
#include "../routing/DuckRouter.h"

enum class NetworkState {SEARCHING, PUBLIC};

//templated class to require some radio capability
template <typename WifiCapability = DuckWifiNone, typename RadioType = DuckLoRa>
class Duck {
  public:
    virtual ~Duck(){
    };

    /**
     * @brief Duck main running loop
     */
    void run(){
      Duck::logIfLowMemory();
      duckRadio.serviceInterruptFlags();
      if(networkState == NetworkState::PUBLIC) {
        handleReceivedPacket();
        // processPortalRequest();
      } else {
        attemptNetworkJoin();
      }
    }

    /**
     * @brief Send data to the CDP network mesh
     * @param topic the message topic
     * @param data a std::string representing the data to send
     * @param targetDevice the device UID to receive the message (default is all papa devices)
     * @return DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendData(byte topic, const std::string data, const std::array<byte,8> targetDevice = PAPADUCK_DUID) {
      if (topic < reservedTopic::max_reserved) {
        logerr_ln("ERROR send data failed, topic is reserved.");
        return DUCKPACKET_ERR_TOPIC_INVALID;
      }
      
      std::vector<byte> app_data;
      app_data.insert(app_data.end(), data.begin(), data.end());
      CdpPacket txPacket = CdpPacket(targetDevice, topic, app_data, this->duid, this->getType());
      int err = sendToRadio(txPacket); //these arguments shouldn't need to be passed, as they are conatined in a cdppacket already
      return err;
    }

    /**
     * @brief Send data to the CDP network mesh
     * @param topic the message topic
     * @param data a vector of bytes representing the data to send
     * @param targetDevice the device UID to receive the message (default is all papa devices)
     * @return DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
    */
    int sendData(byte topic, const byte* data, int length, const std::array<byte,8> targetDevice = PAPADUCK_DUID) {
      if (topic < reservedTopic::max_reserved) {
        logerr_ln("ERROR send data failed, topic is reserved.");
        return DUCKPACKET_ERR_TOPIC_INVALID;
      }
      
      std::vector<byte> app_data(length);
      app_data.insert(app_data.end(), &data[0], &data[length]);
      CdpPacket txPacket = CdpPacket(targetDevice, topic, app_data, this->duid, this->getType());
      int err = sendToRadio(txPacket);
      return err;
    }

    /**
     * @brief Get the duck's unique ID.
     * 
     * @returns A byte vector representing the duck's unique ID
     */ 
    std::vector<uint8_t> getDuckId() {
      return std::vector<uint8_t>(duid.begin(), duid.end());
    }

    /**
     * @brief Get the duck type.
     * @returns A value representing a DuckType
     */
    virtual DuckType getType() = 0;


  protected:
    /**
     * @brief Construct a new Duck object.
     *
     */
    Duck(std::string name){ //check this for correct length
      std::copy(name.begin(), name.end(),duid.begin());
    }

    RadioType duckRadio;
    NetworkState networkState = NetworkState::SEARCHING;
    static constexpr int MEMORY_LOW_THRESHOLD = PACKET_LENGTH + sizeof(CdpPacket);
    std::array<uint8_t,8> duid;
    DuckRouter router;


    
    /**
     * @brief Duck-type specific handler for different packet topics
     */ 
    virtual void handleReceivedPacket() = 0;

    unsigned long lastRreqTime = 0L;

    /**
     * @brief Set up USB serial port
     * @param baudRate baud rate  matching serial monitor
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */ 
    int setupSerial(int baudRate) {
      // // This gives us 10 seconds to do a hard reset if the board is in a bad state after power cycle
      // while (!Serial && millis() < 10000);
    
      Serial.begin(baudRate);
      loginfo_ln("setupSerial rc = %d",DUCK_ERR_NONE);
      loginfo_ln("Running CDP Version: %s",duckutils::getCDPVersion().c_str());
      return DUCK_ERR_NONE;
    }

    /**
     * @brief Configure LoRa radio
     * @param baudRate baud rate  matching serial monitor
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */ 
    int setupLoRaRadio(const LoRaConfigParams& config = RadioType::defaultRadioParams){
      int err = duckRadio.setupRadio(config);

      if (err == DUCKLORA_ERR_BEGIN) {
        logerr_ln("ERROR setupRadio. Starting LoRa Failed. rc = %d",err);
        return err;
      }
      if (err == DUCKLORA_ERR_SETUP) {
        logerr_ln("ERROR setupRadio. Setup LoRa Failed. rc = %d",err);
        return err;
      }
      if (err == DUCKLORA_ERR_RECEIVE) {
        logerr_ln("ERROR setupRadio. Receive LoRa Failed. rc = %d",err);
        return err;
      }
    
      loginfo_ln("setupRadio rc = %d",DUCK_ERR_NONE);
    
      return DUCK_ERR_NONE;
    }

    /**
     * @brief NetworkState if the Duck joins or disconnects from a CDP network
     * @param newState The new NetworkState to join
     */ 
    void setNetworkState(NetworkState newState){
      if (networkState != newState) {
          NetworkState oldState = networkState;
          networkState = newState;
          networkTransition(oldState, newState);
      }
    }

    /**
     * @brief Join a visible CDP network if existing
     */ 
    void attemptNetworkJoin(){
      std::optional<CdpPacket> cdpNode = checkForNetworks();

      if(cdpNode.has_value()){
        // updateRoutingTable(cdpNode);
        networkTransition(networkState, NetworkState::PUBLIC);
      } else {
        if((millis() - this->lastRreqTime) > 5000L){
          sendRouteRequest(BROADCAST_DUID, getDuckId());
          lastRreqTime = millis();
        }
      }
    };

    /**
     * @brief sendData that allows sending for reserved topic rreq
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendRouteRequest(Duid targetDevice, std::vector<uint8_t> data){
      int err = sendReservedTopicData(targetDevice, reservedTopic::rreq, data);
      if (err != DUCK_ERR_NONE){
        logerr_ln("ERR: failed to send pong");
      }
      return err;
    }

    /**
     * @brief sendData that allows sending for reserved topic pong
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendPong(Duid targetDevice = BROADCAST_DUID){
      std::vector<byte> data(1, 0);
      int err = sendReservedTopicData(targetDevice, reservedTopic::pong, data);
      if (err != DUCK_ERR_NONE){
        logerr_ln("ERR: failed to send pong");
      }
      return err;
    }
    
    /**
     * @brief sendData that allows sending for reserved topic ping
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendPing(Duid targetDevice = BROADCAST_DUID){
      std::vector<byte> data(1, 0);
      int err = sendReservedTopicData(targetDevice, reservedTopic::ping, data);
      if (err != DUCK_ERR_NONE){
        logerr_ln("ERR: failed to send ping");
      }
      return err;
    }
    

    /**
     * @brief Enable duck radio to start receiving packets from the mesh network
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int startReceive(){
      int err = duckRadio.startReceive();
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR Restarting Duck...");
        duckesp::restartDuck();
      }
      return err;
    };

    /**
     * @brief Log an error message if the system's memory is too low.
     */
    static void logIfLowMemory() {
      if (duckesp::getMinFreeHeap() < MEMORY_LOW_THRESHOLD
        || duckesp::getMaxAllocHeap() < MEMORY_LOW_THRESHOLD
      ) {
        //logwarn_ln("WARNING heap memory is low");
      }
    }


  private:
    Duck(Duck const&) = delete;
    Duck& operator=(Duck const&) = delete;

    /**
     * @brief NetworkState transition for NetworkState FSM
     * @param oldState NetworkState to transition out of
     * @param newState NetworkState to transition in to
     */
    void networkTransition(NetworkState oldState, NetworkState newState){
      if (oldState == NetworkState::SEARCHING && newState == NetworkState::PUBLIC) {
        logdbg_ln("public network joined");
      }
    }

    /**
     * @brief Read packets from CDP nodes responding to our network join request
     * @returns Optional<CdpPacket> if network join response is found, nullopt if not 
     */
    std::optional<CdpPacket> checkForNetworks(){ 
      std::optional<CdpPacket> result;

      if (duckRadio.getReceiveFlag()){
        CdpPacket rxPacket;
        // int err = duckRadio.readReceivedData(rxPacket.rawBuffer());
        // if (err != DUCK_ERR_NONE) {
        //   logerr_ln("ERROR failed to get data from DuckRadio. rc = %d",err);
        //   result = std::nullopt;
        // }
        // logdbg_ln("Got data from radio, prepare for relay. size: %d",rxPacket.rawBuffer().size());
        result = (rxPacket.topic == reservedTopic::rrep) ? std::optional<CdpPacket>{rxPacket} : std::nullopt; //need to make sure rrep is addressed to us

        } else {
          result = std::nullopt;
        }
        return result;
    }

    /**
     * @brief Control access for public APIs to send certain reserved topic types
     *
     * @param targetDevice device uid to send to
     * @param topic reserved topic of data
     * @param data byte vector representing data to send
     * @return DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendReservedTopicData(Duid targetDevice, reservedTopic topic, std::vector<byte> data){
      int err = DUCK_ERR_NONE;
      CdpPacket txPacket = CdpPacket(targetDevice, topic, data, this->duid, this->getType());
      router.getFilter().assignUniqueMessageId(txPacket);
      err = txPacket.prepareForSending();
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR Failed to build ping packet, err = " + err);
        return err;
      }
      err = duckRadio.sendData(txPacket.rawBuffer());
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR Lora sendData failed, err = %d", err);
      }
      return err;
    }

    /**
     * @brief Passes data to radiolib for rx
     *
     * @param txPacket CdpPacket to be sent 
     * @return DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendToRadio(CdpPacket txPacket) {
      int err = DUCK_ERR_NONE;
      router.getFilter().assignUniqueMessageId(txPacket);
      err = txPacket.prepareForSending();
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR Failed to build ping packet, err = " + err);
        return err;
      }
      err = duckRadio.sendData(txPacket.rawBuffer());
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR Lora sendData failed, err = %d", err);
      }
      router.getFilter().bloom_add(txPacket.muid.data(), MUID_LENGTH);
    
      return err;
    }
};

#endif
