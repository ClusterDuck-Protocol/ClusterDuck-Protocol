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
#include "../routing/RouteJSON.h"

#define NET_JOIN_DELAY 15000L

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
      duckRadio.serviceInterruptFlags();
      Duck::logIfLowMemory();
      if(router.getNetworkState() == NetworkState::PUBLIC) {
        if(duckRadio.getReceiveFlag()){
          handleReceivedPacket();
        }
      } else {
        if(this->getType() == DuckType::DETECTOR){
          loginfo_ln("Detector duck -- bypassing network search.");
          router.setNetworkState(NetworkState::PUBLIC);
        } else{
            attemptNetworkJoin();
            if(router.getNetworkState() == NetworkState::SEARCHING && (millis() > (NET_JOIN_DELAY * 5 + 5000L))){
              loginfo_ln("No existing network found, creating new CDP network...");
              router.setNetworkState(NetworkState::PUBLIC);
            }
        }
      }

    }

    int setupWithDefaults() {
      this->setupSerial(115200);
      int err = this->setupLoRaRadio();
      if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR setupWithDefaults rc = %d",err); 
      }
      return err;
    }

    /**
     * @brief Send data to the CDP network mesh
     * @param topic the message topic
     * @param data a std::string representing the data to send
     * @param targetDevice the device UID to receive the message (default is all papa devices)
     * @return DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendData(uint8_t topic, const std::string data, const std::array<uint8_t,8> targetDevice = PAPADUCK_DUID) {
      int err = DUCK_ERR_NONE;
      if (topic < reservedTopic::max_reserved) {
        logerr_ln("ERROR send data failed, topic is reserved.");
        return DUCKPACKET_ERR_TOPIC_INVALID;
      }
      if(router.getNetworkState() == NetworkState::PUBLIC){
        std::vector<uint8_t> app_data;
        app_data.insert(app_data.end(), data.begin(), data.end());
        CdpPacket txPacket = CdpPacket(targetDevice, topic, app_data, this->duid, this->getType());

        std::optional<Duid> nextHop = router.getBestNextHop(txPacket.dduid);
        if(nextHop.has_value() || txPacket.dduid == PAPADUCK_DUID || txPacket.dduid == BROADCAST_DUID){
          router.getFilter().assignUniqueMessageId(txPacket);
          err = sendToRadio(txPacket);
        } else {
            if((millis() - this->lastRreqTime) > 30000){
              loginfo_ln("[DUCK] Destination not in table, sending new RREQ.");
              RouteJSON rreqDoc = RouteJSON(txPacket.dduid, this->duid);
              rreqDoc.addToPath(this->duid);
              sendRouteRequest(txPacket.dduid, rreqDoc);
              this->lastRreqTime = millis();
            }
        }
      }
        return err;
    }

    /**
     * @brief Send data to the CDP network mesh
     * @param topic the message topic
     * @param data a vector of bytes representing the data to send
     * @param targetDevice the device UID to receive the message (default is all papa devices)
     * @return DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
    */
    int sendData(uint8_t topic, const uint8_t* data, int length, const std::array<uint8_t,8> targetDevice = PAPADUCK_DUID) {
      int err = DUCK_ERR_NONE;
      if (topic < reservedTopic::max_reserved) {
        logerr_ln("ERROR send data failed, topic is reserved.");
        return DUCKPACKET_ERR_TOPIC_INVALID;
      }

      if(router.getNetworkState() == NetworkState::PUBLIC){
        std::vector<uint8_t> app_data;
        app_data.insert(app_data.end(), &data[0], &data[length]);
        CdpPacket txPacket = CdpPacket(targetDevice, topic, app_data, this->duid, this->getType());

        std::optional<Duid> nextHop = router.getBestNextHop(txPacket.dduid);
        if(nextHop.has_value() || txPacket.dduid == PAPADUCK_DUID || txPacket.dduid == BROADCAST_DUID){
          router.getFilter().assignUniqueMessageId(txPacket);
          err = sendToRadio(txPacket);
        } else {
            if((millis() - this->lastRreqTime) > 30000){
              loginfo_ln("[DUCK] Destination not in table, sending new RREQ.");
              RouteJSON rreqDoc = RouteJSON(txPacket.dduid, this->duid);
              rreqDoc.addToPath(this->duid);
              sendRouteRequest(txPacket.dduid, rreqDoc);
              this->lastRreqTime = millis();
            }
        }
      }
      return err;
    }

    /**
     * @brief sendData that allows sending for reserved topic ping
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendPing(Duid targetDevice = BROADCAST_DUID){
      std::vector<uint8_t> data(1, 0);
      int err = sendReservedTopicData(targetDevice, reservedTopic::ping, data);
      if (err != DUCK_ERR_NONE){
        logerr_ln("ERR: failed to send ping");
      }
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

    void joinWifiNetwork(std::string ssid = "", std::string password = ""){
      int err = this->duckWifi.joinNetwork(ssid, password);

      // If we fail to connect to WiFi, retry a few times
      if (err == DUCK_INTERNET_ERR_CONNECT) {
        int retry=0;
        while ( err ==  DUCK_INTERNET_ERR_CONNECT && retry < 5 ) {
          Serial.printf("WiFi connection failed, retry connection: %s\n", ssid.c_str());
          delay(5000);
          err = err = this->duckWifi.joinNetwork(ssid, password);
          retry++;
        }  
      }

        if (err == DUCK_INTERNET_ERR_CONNECT) {
          logerr_ln("ERROR wifi setup failed = %d",err);
        }
    }


  protected:
    /**
     * @brief Construct a new Duck object.
     *
     */
    Duck(std::string name){ //check this for correct length
      std::copy(name.begin(), name.end(),duid.begin());
    }

    RadioType duckRadio;
    WifiCapability duckWifi;
    static constexpr int MEMORY_LOW_THRESHOLD = PACKET_LENGTH + sizeof(CdpPacket);
    std::array<uint8_t,8> duid;
    DuckRouter router;


    
    /**
     * @brief Duck-type specific handler for different packet topics
     */ 
    virtual void handleReceivedPacket() = 0;

    int broadcastPacket(CdpPacket& packet){
      bool alreadySeen = router.getFilter().bloom_check(packet.muid.data(), MUID_LENGTH);
      int err = DUCK_ERR_NONE;
      if(alreadySeen){
        logdbg_ln("broadcastPacket: Packet already seen. No relay.");
      } else{
        packet.hopCount++;
        err = sendToRadio(packet);
      }
      return err;
    }
    //this->dduid == BROADCAST_DUID || this->dduid == PAPADUCK_DUID

    int forwardPacket(CdpPacket& packet){
      //next node checks if it has the destination in its table
      //if in table, and ttl hasn't expired, forward the packet
      //if in the table and ttl has expired, send a rreq and wait for response before sending?<--- do this later?
      //if the duck can't find the destination in its routing table then it just doesn't send
      int err = DUCK_ERR_NONE;
      std::optional<Duid> nextHop = router.getBestNextHop(packet.dduid);
      if(nextHop.has_value() || packet.dduid == PAPADUCK_DUID){ //do we need to make sure this duck isn't a papa?
        err = broadcastPacket(packet);
        if (err != DUCK_ERR_NONE) {
            logerr_ln("====> ERROR forwardPacket failed. rc = %d",err);
        } else {
            loginfo_ln("forwardPacket: packet RELAY DONE");
        }
      } else{
        std::string strDuid(packet.dduid.begin(), packet.dduid.end());
        logdbg_ln("no entry for this id, skipping relay DDuid: %s", strDuid.c_str());
      }
      return err;
    }

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
     * @brief Join a visible CDP network if existing
     */ 
    void attemptNetworkJoin(){
      std::optional<CdpPacket> cdpNode = checkForNetworks();
      if(cdpNode.has_value()){
        //add an entry for the nearest neighbor, next hop is itself
        router.insertIntoRoutingTable(cdpNode->sduid, cdpNode->sduid, this->getSignalScore()); //should signal score be stored on cdp packet?
        router.setNetworkState(NetworkState::PUBLIC);
      } else {
        if((millis() - this->lastRreqTime) > NET_JOIN_DELAY){
          RouteJSON rreqDoc = RouteJSON(BROADCAST_DUID, this->duid);
          rreqDoc.addToPath(this->duid);
          sendRouteRequest(BROADCAST_DUID, rreqDoc);
          loginfo_ln("searching for networks....");
          lastRreqTime = millis();
        }
      }
    };

    /**
     * @brief sendData that allows sending for reserved topic rreq
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendRouteRequest(Duid targetDevice, RouteJSON json){
      std::string strJson = json.asString();
      std::vector<uint8_t> app_data;
      app_data.insert(app_data.end(), strJson.begin(), strJson.end());
      int err = sendReservedTopicData(targetDevice, reservedTopic::rreq, app_data);
      if (err != DUCK_ERR_NONE){
        logerr_ln("ERR: failed to send rreq");
      }
      return err;
    }

    /**
     * @brief sendData that allows sending for reserved topic rreq
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendRouteResponse(Duid targetDevice, std::string data){
      std::vector<uint8_t> app_data;
      app_data.insert(app_data.end(), data.begin(), data.end());
      int err = sendReservedTopicData(targetDevice, reservedTopic::rrep, app_data);
      if (err != DUCK_ERR_NONE){
        logerr_ln("ERR: failed to send rrep");
      }
      return err;
    }

    /**
     * @brief sendData that allows sending for reserved topic pong
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendPong(Duid targetDevice = BROADCAST_DUID){
      std::vector<uint8_t> data(1, 0);
      int err = sendReservedTopicData(targetDevice, reservedTopic::pong, data);
      if (err != DUCK_ERR_NONE){
        logerr_ln("ERR: failed to send pong");
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

    /**
     * @brief Calculate a signal score based on the current RSSI and SNR values.
     * The signal score is a value between 0 and 1, where 1 is the best possible
     * signal and 0 is the worst possible signal.
     *
     * The formula used to calculate the signal score is:
     *
     * signalScore = (normalizedRSSI + normalizedSNR) / 2
     *
     * where:
     *
     * normalizedRSSI = (RSSI - RSSI_MIN) / (RSSI_MAX - RSSI_MIN)
     * normalizedSNR = (SNR - SNR_MIN) / (SNR_MAX - SNR_MIN)
     *
     * RSSI_MIN = -131 dBm
     * RSSI_MAX = -20 dBm
     * SNR_MIN = -11.5 dB
     * SNR_MAX = 11.5 dB
     *
     * @returns SignalScore struct containing rssi, snr, and signalScore values
     */
    SignalScore getSignalScore(){
      SignalScore signalInfo;
      signalInfo.rssi = (duckRadio.getRSSI() - RSSI_MIN)/(RSSI_MAX-RSSI_MIN);
      signalInfo.snr = (duckRadio.getSNR() - SNR_MIN)/(SNR_MAX-SNR_MIN);
      signalInfo.signalScore = (signalInfo.rssi + signalInfo.snr) / 2.0f;
      return signalInfo;
    }


  private:
    Duck(Duck const&) = delete;
    Duck& operator=(Duck const&) = delete;

    /**
     * @brief Read packets from CDP nodes responding to our network join request
     * @returns Optional<CdpPacket> if network join response is found, nullopt if not 
     */
    std::optional<CdpPacket> checkForNetworks(){ 
      std::optional<CdpPacket> result;

      if (duckRadio.getReceiveFlag()){
        std::optional<std::vector<uint8_t>> rxData = this->duckRadio.readReceivedData();
        if (!rxData) {
          logerr_ln("ERROR failed to get data from DuckRadio.");
          result = std::nullopt;
        } else{
          CdpPacket rxPacket(rxData.value());
          if((rxPacket.topic == reservedTopic::rrep) && (rxPacket.dduid == this->duid)){ //should all packets without valid crc immediately be discarded at a lower level?
            result = std::optional<CdpPacket>{rxPacket}; 
          } else {
            result = std::nullopt;
          }
        }
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
    int sendReservedTopicData(Duid targetDevice, reservedTopic topic, std::vector<uint8_t> data){
      int err = DUCK_ERR_NONE;
      if((router.getNetworkState() == NetworkState::PUBLIC) || ((router.getNetworkState() == NetworkState::SEARCHING) && (topic == reservedTopic::rreq))){
        CdpPacket txPacket = CdpPacket(targetDevice, topic, data, this->duid, this->getType());
        router.getFilter().assignUniqueMessageId(txPacket);
        err = txPacket.prepareForSending();
        if (err != DUCK_ERR_NONE) {
          logerr_ln("ERROR Failed to build packet, err = " + err);
          return err;
        }
        err = duckRadio.sendData(txPacket.asBytes());
        if (err != DUCK_ERR_NONE) {
          logerr_ln("ERROR Lora sendData failed, err = %d", err);
        }
      } 
      return err;
    }

    /**
     * @brief Passes data to radiolib for rx
     *
     * @param txPacket CdpPacket to be sent 
     * @return DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendToRadio(CdpPacket& txPacket) {
      int err = txPacket.prepareForSending();
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR Failed to build ping packet, err = " + err);
        return err;
      }

      router.getFilter().bloom_add(txPacket.muid.data(), MUID_LENGTH);

      err = duckRadio.sendData(txPacket.asBytes());
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR Lora sendData failed, err = %d", err);
      }
     
      return err;
    }
};

#endif
