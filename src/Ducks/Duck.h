#ifndef DUCK_H
#define DUCK_H

#include <Arduino.h>

#include "../utils/DuckError.h"
#include "../bloomfilter.h"
#include "../include/cdpcfg.h"
#include "../DuckPacket.h"
#include "../Radio/DuckLoRa.h"
#include "DuckTypes.h"
#include "../utils/DuckUtils.h"
#include <cassert>
#include "../CdpPacket.h"
#include "../DuckCrypto.h"
#include "../DuckEsp.h"

enum class NetworkState {SEARCHING, PUBLIC};

//templated class to require some radio capability
template <typename RadioType = DuckLoRa>
class Duck {

public:
  /**
   * @brief Construct a new Duck object.
   *
   */
  Duck(std::string name){
    std::copy(name.begin(), name.end(),duid.begin());
  }

  virtual ~Duck();

  std::string getCDPVersion() { return duckutils::getCDPVersion(); }

private:
  const int MEMORY_LOW_THRESHOLD = PACKET_LENGTH + sizeof(CdpPacket);
  /**
   * @brief Get the duck's unique ID.
   * 
   * @returns A std::string representing the duck's unique ID
   */ 
  std::string getDuckId() {return std::string(duid.begin(), duid.end());}


  int setupLoRaRadio(const LoRaConfigParams& config){
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
  
    txPacket = new DuckPacket(duid);
    rxPacket = new DuckPacket();
    loginfo_ln("setupRadio rc = %d",DUCK_ERR_NONE);
  
    return DUCK_ERR_NONE;
  }

  /**
   * @brief Setup WiFi access point.
   *
   * @param accessPoint a string representing the access point. Default to
   * "🆘 DUCK EMERGENCY PORTAL"
   *
   * @returns DUCK_ERROR_NONE if successful, an error code otherwise.
   */
  int setupWifi(const char* ap = "🆘 DUCK EMERGENCY PORTAL"){
    int err = duckNet->setupWifiAp(ap);
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR setupWifi rc = %d",DUCK_ERR_NONE);
    } else {
      loginfo_ln("setupWifi OK");
    }
    return err;
  }; //this should be a separate module too

  /**
   * @brief Setup DNS.
   *
   * @returns DUCK_ERROR_NONE if successful, an error code otherwise.
   */
  int setupDns(){
    int err = duckNet->setupDns();
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR setupDns rc = %d",DUCK_ERR_NONE);
    } else {
      loginfo_ln("setupDns OK");
    }
    return err;
  };

  /**
   * @brief Setup web server.
   *
   * The WebServer is used to communicate with the Duck over ad-hoc WiFi
   * connection.
   *
   * @param createCaptivePortal set to true if Captive WiFi connection is
   * needed. Defaults to false
   * @param html A string representing custom HTML code used for the portal.
   * Default is an empty string Default portal web page is used if the string is
   * empty
   */
  int setupWebServer(bool createCaptivePortal = false, std::string html = ""){
    int err = DUCK_ERR_NONE;
    duckNet->setDeviceId(duid);
    err = duckNet->setupWebServer(createCaptivePortal, html);
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR setupWebServer %d",err);
    } else {
      loginfo_ln("setupWebServer OK");
    }
    return err;
  }

  /**
   * @brief Setup internet access.
   *
   * @param ssid        the ssid of the WiFi network
   * @param password    password to join the network
   */
  int setupInternet(std::string ssid, std::string password){
    int err = duckNet->setupInternet(ssid, password);
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR setupInternet rc = %d",DUCK_ERR_NONE);
    } else {
      loginfo_ln("setupInternet OK");
    }
    return err;
  };

  #ifdef CDPCFG_WIFI_NONE //move to wifi module
  void Duck::processPortalRequest() {}
  #else
  void Duck::processPortalRequest() { duckNet->dnsServer.processNextRequest(); }
  #endif

  /**
   * @brief Sends data into the mesh network.
   *
   * @param topic the message topic
   * @param data a string representing the data
   * @param targetDevice the device UID to receive the message (default is no target device)
   * @param outgoingMuid Output parameter that returns the MUID of the sent packet. NULL is ignored.
   * @return DUCK_ERR_NONE if the data was send successfully, an error code otherwise. 
   */
  int sendData(byte topic, const std::string data,
    const std::array<byte,8> targetDevice = PAPADUCK_DUID, std::array<byte,8> * outgoingMuid = NULL){
      std::vector<byte> app_data;
      app_data.insert(app_data.end(), data.begin(), data.end());
      int err = sendData(topic, app_data, targetDevice, outgoingMuid);
      return err;
  }
  /**
   * @brief Sends data into the mesh network.
   *
   * @param topic the message topic
   * @param data a vector of bytes representing the data to send
   * @param targetDevice the device UID to receive the message (default is no target device)
   * @param outgoingMuid Output parameter that returns the MUID of the sent packet. NULL is ignored.
   * @return DUCK_ERR_NONE if the data was send successfully, an error code
   otherwise.
   */
  int sendData(byte topic, std::vector<byte>& bytes,
    const std::array<byte,8> targetDevice = PAPADUCK_DUID, std::array<byte,8> * outgoingMuid = NULL){
      std::vector<byte> app_data(length);
      app_data.insert(app_data.end(), &data[0], &data[length]);
      int err = sendData(topic, app_data, targetDevice, outgoingMuid);
      return err;
  }
    
  /**
   * @brief Sends data into the mesh network.
   *
   * @param topic the message topic
   * @param data a byte buffer representing the data to send
   * @param length the length of the byte buffer
   * @param targetDevice the device UID to receive the message (default is no target device)
   * @param outgoingMuid Output parameter that returns the MUID of the sent packet. NULL is ignored.
   * @return DUCK_ERR_NONE if the data was send successfully, an error code
   * otherwise.
   */
  int sendData(byte topic, const byte* data, int length,
    const std::array<byte,8> targetDevice = PAPADUCK_DUID, std::array<byte,8> * outgoingMuid = NULL){
      if (topic < reservedTopic::max_reserved) {
        logerr_ln("ERROR send data failed, topic is reserved.");
        return DUCKPACKET_ERR_TOPIC_INVALID;
      }
     if (data.size() > MAX_DATA_LENGTH) {
       logerr_ln("ERROR send data failed, message too large: %d bytes",data.size());
       return DUCKPACKET_ERR_SIZE_INVALID;
     }
     int err = txPacket->prepareForSending(&filter, targetDevice, this->getType(), topic, data);
   
     if (err != DUCK_ERR_NONE) {
       return err;
     }
   
     err = duckRadio.sendData(txPacket->getBuffer());
   
     CdpPacket packet = CdpPacket(txPacket->getBuffer());
   
     if (err == DUCK_ERR_NONE) {
       filter.bloom_add(packet.muid.data(), MUID_LENGTH);
     }
       std::copy(packet.muid.begin(),packet.muid.end(),lastMessageMuid.begin());
     assert(lastMessageMuid.size() == MUID_LENGTH);
     if (outgoingMuid != NULL) {
         std::copy(packet.muid.cbegin(),packet.muid.cend(),outgoingMuid->begin());
       assert(outgoingMuid->size() == MUID_LENGTH);
     }
     txPacket->reset();
   
     return err;
   }

  /**
   * @brief Builds a CdpPacket with a specified muid.
   *
   * @param topic the message topic
   * @param data a byte buffer representing the data to send
   * @param length the length of the byte buffer
   * @param targetDevice the device UID to receive the message
   * @param muid the muid that should be associated with this packet
   * @return a new CdpPacket
   * */
  CdpPacket buildCdpPacket(byte topic, const std::vector<byte> data,
    const std::array<byte,8> targetDevice, const std::array<byte,4> &muid){
      if (data.size() > MAX_DATA_LENGTH) {
        logerr_ln("ERROR send data failed, message too large: %d bytes", data.size());
      }
      int err = txPacket->prepareForSending(&filter, targetDevice, this->getType(), topic, data);
      //txPacket unintended side effects?
      if (err != DUCK_ERR_NONE) {
        logerr_ln("prepare for sending failed. " + err);
      }
      //todo error not handled properly
      CdpPacket packet = CdpPacket(txPacket->getBuffer());
      packet.muid = muid;
    
      return packet;
  } //this doesn't even belong in this class

  // /**
  //  * @brief Check wifi connection status
  //  * 
  //  * @returns true if device wifi is connected, false otherwise. 
  //  */
  // bool isWifiConnected();

  // /**
  //  * @brief Get the access point ssid
  //  * 
  //  * @returns the wifi access point as a string
  //  */
  // std::string getSsid();
  // /**
  //  * @brief Get the wifi access point password.
  //  * 
  //  * @returns the wifi access point password as a string. 
  //  */
  // std::string getPassword();

  /**
   * @brief Get an error code description.
   * 
   * @param error an error code
   * @returns a string describing the error. 
   */
  std::string getErrorString(int error) {//seems more like a duck logger or utils type of thing
    std::string errorStr = std::to_string(error) + ": ";
  
    switch (error) {
      case DUCK_ERR_NONE:
        return errorStr + "No error";
      case DUCK_ERR_NOT_SUPPORTED:
        return errorStr + "Feature not supported";
      case DUCK_ERR_SETUP:
        return errorStr + "Setup failure";
      case DUCK_ERR_ID_TOO_LONG:
        return errorStr + "Id length is invalid";
      case DUCKLORA_ERR_BEGIN:
        return errorStr + "Lora module initialization failed";
      case DUCKLORA_ERR_SETUP:
        return errorStr + "Lora module configuration failed";
      case DUCKLORA_ERR_RECEIVE:
        return errorStr + "Lora module failed to read data";
      case DUCKLORA_ERR_TIMEOUT:
        return errorStr + "Lora module timed out";
      case DUCKLORA_ERR_TRANSMIT:
        return errorStr + "Lora moduled failed to send data";
      case DUCKLORA_ERR_HANDLE_PACKET:
        return errorStr + "Lora moduled failed to handle RX data";
      case DUCKLORA_ERR_MSG_TOO_LARGE:
        return errorStr + "Attempted to send a message larger than 256 bytes";
  
      case DUCKWIFI_ERR_NOT_AVAILABLE:
        return errorStr + "Wifi network is not availble";
      case DUCKWIFI_ERR_DISCONNECTED:
        return errorStr + "Wifi is disconnected";
      case DUCKWIFI_ERR_AP_CONFIG:
        return errorStr + "Wifi configuration failed";
  
      case DUCKDNS_ERR_STARTING:
        return errorStr + "DNS initialization failed";
  
      case DUCKPACKET_ERR_SIZE_INVALID:
        return errorStr + "Duck packet size is invalid";
      case DUCKPACKET_ERR_TOPIC_INVALID:
        return errorStr + "Duck packet topic field is invalid";
      case DUCKPACKET_ERR_MAX_HOPS:
        return errorStr + "Duck packet reached maximum allowed hops";
  
      case DUCK_INTERNET_ERR_SETUP:
        return errorStr + "Internet setup failed";
      case DUCK_INTERNET_ERR_SSID:
        return errorStr + "Internet SSID is not valid";
      case DUCK_INTERNET_ERR_CONNECT:
        return errorStr + "Internet connection failed";
    }
    
    return "Unknown error";
  }

  /**
   * @brief Turn on or off encryption.
   * 
   * @param state true for on, false for off
   */
  void setEncrypt(bool state); //why

  /**
   * @brief get encryption state.
   * 
   * @return true for on, false for off
   */
  bool getEncrypt();

  /**
   * @brief Turn on or off decryption. Used with MamaDuck
   * 
   * @param state true for on, false for off
   */
  void setDecrypt(bool state);

  /**
   * @brief get decryption state.
   * 
   * @return true for on, false for off
   */
  bool getDecrypt();

  /**
   * @brief Set new AES key for encryption.
   * 
   * @param newKEY byte array, must be 32 bytes
   */
  void setAESKey(uint8_t newKEY[32]); //different module

  /**
   * @brief Set new AES initialization vector.
   * 
   * @param newIV byte array, must be 16 bytes
   */
  void setAESIv(uint8_t newIV[16]);

  /**
   * @brief Encrypt data using AES-256 CTR.
   * 
   * @param text pointer to byte array of plaintext
   * @param encryptedData pointer to byte array to store encrypted message
   * @param inc size of text to be encrypted
   */
  void encrypt(uint8_t* text, uint8_t* encryptedData, size_t inc);

  /**
   * @brief Decrypt data using AES-256 CTR.
   * 
   * @param encryptedData pointer to byte array to be decrypted
   * @param text pointer to byte array to store decrypted plaintext
   * @param inc size of text to be decrypted
   */
  void decrypt(uint8_t* encryptedData, uint8_t* text, size_t inc);

  virtual void handleReceivedPacket() = 0;
    /**
   * @brief Run sketch-loop specific code
   * 
   * This method must be implemented by the Duck's concrete classes such as DuckLink, MamaDuck,...
   */
  void run(){
    Duck::logIfLowMemory();

    duckRadio.serviceInterruptFlags();
  
    if(networkState == NetworkState::PUBLIC) {
      handleReceivedPacket();
      processPortalRequest();
    } else {
      attemptNetworkJoin();
      rxPacket->reset();
    }
  }

private: 
  RadioType duckRadio;
  NetworkState networkState = NetworkState::SEARCHING;
  void setNetworkState(NetworkState newState){
    if (networkState != newState) {
        NetworkState oldState = networkState;
        networkState = newState;
        networkTransition(oldState, newState);
    }
  }
  void networkTransition(NetworkState oldState, NetworkState newState){
    if (oldState == NetworkState::SEARCHING && newState == NetworkState::PUBLIC) {
      logdbg_ln("public network joined");
    }
  }

  std::optional<CdpPacket> checkForNetworks(){ 
  std::optional<CdpPacket> result;

  if (duckRadio::getReceiveFlag()){
    std::vector<uint8_t> data;
    int err = duckRadio.readReceivedData(&data);
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR failed to get data from DuckRadio. rc = %d",err);
      result = std::nullopt;
    }
    logdbg_ln("Got data from radio, prepare for relay. size: %d",data.size());

    CdpPacket packet = CdpPacket(rxPacket->getBuffer());
    result = (packet.topic == reservedTopic::rrep) ? std::optional<CdpPacket>{packet} : std::nullopt;

    rxPacket->reset();
  } else {
    result = std::nullopt;
  }
  return result;
} ;


// //put this on Router
// void updateRoutingTable(){
//   Serial.println("routing table creation")
// }


  void attemptNetworkJoin(){
  std::optional<CdpPacket> cdpNode = checkForNetworks();

  if(cdpNode.has_value()){
    // updateRoutingTable(cdpNode);
    networkTransition(networkState, NetworkState::PUBLIC);
  } else {
    if((millis() - this->lastRreqTime) > 5000L){
      sendRouteData(reservedTopic::rreq, getDuckId(), BROADCAST_DUID);
      lastRreqTime = millis();
    }
  }
};

  unsigned long lastRreqTime = 0L;
  int sendRouteData(reservedTopic topic, std::string data, Duid targetDevice){
    std::vector<byte> app_data;
    app_data.insert(app_data.end(), data.begin(), data.end());
    // int err = sendData(topic, app_data, targetDevice);
    txPacket->prepareForSending(&filter, BROADCAST_DUID, DuckType::UNKNOWN, topic, app_data );
    int err = duckRadio.sendData(txPacket->getBuffer());
    return err;
  }
  

protected:
  Duck(Duck const&) = delete;
  Duck& operator=(Duck const&) = delete;

  std::string deviceId; //just make this a print function
  std::array<uint8_t,8> duid;

  class DuckRecord {
    public:
      //DuckRecord() : routingScore(0), lastSeen(0), snr(0), rssi(0) {}
      DuckRecord(std::string devId, long routingScore, long lastSeen, float snr, float rssi) :
        DeviceId(std::move(devId)), routingScore(routingScore), lastSeen(lastSeen), snr(snr), rssi(rssi) {}

      std::string getDeviceId() { return DeviceId; }
      long getRoutingScore() const { return routingScore; }
      long getLastSeen() { return lastSeen; }
      long getSnr() { return snr; }
      long getRssi() { return rssi; }
  private:
      std::string DeviceId;
      long routingScore, lastSeen;
      float snr, rssi;
  };
  
  std::multimap<long,DuckRecord,std::greater<long>> routingTable;

  // DuckNet * const duckNet;

  DuckPacket* txPacket = NULL;
  DuckPacket* rxPacket = NULL;
  std::array<uint8_t,4> lastMessageMuid;

  BloomFilter filter;
  /**
   * @brief Sort the routing table using the customGreater comparator
   */
   std::list <DuckRecord> getRoutingTable() {
    std::list<DuckRecord> sortedList;
    for (const auto& pair : routingTable) {
      sortedList.push_back(pair.second);
    }
    return sortedList;
    }
//  void sortRoutingTable() {
//    getRoutingTable().sort([](const DuckRecord& lhs, const DuckRecord& rhs){
//        return lhs.getRoutingScore() > rhs.getRoutingScore();
//    });
//  }
    /**
     * @brief Insert a new record into the routing table
     *
     * @param deviceID the device ID
     * @param routingScore the routing score
     * @param lastSeen the last seen timestamp
     * @param snr the signal to noise ratio
     * @param rssi the received signal strength indicator
     */
  void insertIntoRoutingTable(std::string deviceID, long routingScore, long lastSeen, float snr, float rssi) {
    DuckRecord record(std::move(deviceID), routingScore, lastSeen, snr, rssi);
    routingTable.insert(std::make_pair(routingScore, record));
  }

  /**
   * @brief sends a pong message
   * 
   * @return DUCK_ERR_NONE if successfull. An error code otherwise 
   */
  int sendPong(){ //these should not be special
    int err = DUCK_ERR_NONE;
    std::vector<byte> data(1, 0);
    err = txPacket->prepareForSending(&filter, PAPADUCK_DUID, this->getType(), reservedTopic::pong, data);
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR Oops! failed to build pong packet, err = %d", err);
      return err;
    }
    err = duckRadio.sendData(txPacket->getBuffer());
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR Oops! Lora sendData failed, err = %d", err);
      return err;
    }
    return err;
  }
  
  /**
   * @brief sends a ping message
   *
   * @return DUCK_ERR_NONE if successfull. An error code otherwise
   */
  int sendPing(){
    int err = DUCK_ERR_NONE;
    std::vector<byte> data(1, 0);
    err = txPacket->prepareForSending(&filter, PAPADUCK_DUID, this->getType(), reservedTopic::ping, data);
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR Failed to build ping packet, err = " + err);
      return err;
    }
    err = duckRadio.sendData(txPacket->getBuffer());
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR Lora sendData failed, err = %d", err);
    }
    return err;
  }
  

  /**
   * @brief Tell the duck radio to start receiving packets from the mesh network
   *
   * @return DUCK_ERR_NONE if successful, an error code otherwise
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
   * @brief Setup a duck with default settings
   *
   * The default implementation simply initializes the serial interface.
   * It can be overriden by each concrete Duck class implementation.
   */
  virtual int setupWithDefaults(std::array<uint8_t,8> deviceId, std::string ssid, std::string password) {
    int err = setupSerial();
    if (err != DUCK_ERR_NONE) {
      return err;
    }
    err = setDeviceId(deviceId);
    if (err != DUCK_ERR_NONE) {
      return err;
    }
    return DUCK_ERR_NONE;
  }

  /**
   * @brief Get the duck type.
   * 
   * @returns A value representing a DuckType
   */
  virtual int getType() = 0;

  /**
   * @brief reconnect the duck to the given wifi access point
   * 
   * @param ssid the access point ssid to connect to 
   * @param password the access point password
   * @return DUCK_ERR_NONE if the duck reconnected to the AP sucessfully. An error code otherwise. 
   */
  virtual int reconnectWifi(std::string ssid, std::string password) {
    return DUCK_ERR_NONE;
  }

  /**
   * @brief Handle request from emergency portal.
   *
   */
  void processPortalRequest();

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

};

#endif
