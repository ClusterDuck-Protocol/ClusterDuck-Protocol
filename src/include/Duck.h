#ifndef DUCK_H
#define DUCK_H

#include <Arduino.h>

class Duck;
// Since Duck needs to know about DuckNet and DuckNet needs to know about Duck,
// this forward declaration allows a Duck pointer to be declared in DuckNet.h.
// Similarly, muidStatus needs to be declared before DuckNet.h is included.
enum muidStatus {
  invalid, // The MUID was not given in the correct format.
  unrecognized, // The MUID was not recognized. The Duck may have forgotten it.
  not_acked, // The MUID was recognized but not yet ack'd.
  acked // The MUID was recognized and has been ack'd.
};

#include "../DuckError.h"
#include "bloomfilter.h"
#include "cdpcfg.h"
#include "DuckPacket.h"
#include "DuckNet.h"
#include "DuckRadio.h"
#include "DuckTypes.h"
#include "DuckUtils.h"

class Duck {

public:
  /**
   * @brief Construct a new Duck object.
   *
   */
  Duck(std::string name="");

  virtual ~Duck();

  std::string getCDPVersion() { return duckutils::getCDPVersion(); }

  /**
   * @brief Set the Device Name object
   * 
   * @param name 
   */
  void setName(std::string name) { this->duckName = name; }
  
  /**
   * @brief Get the duck's name.
   * 
   * @returns A string representing the duck's name
   */
  std::string getName() {return duckName;}
  /**
   * @brief setup the duck unique ID
   * 
   * @param an 8 byte array
   * @return DUCK_ERR_NONE if successful, an error code otherwise 
   */
  int setDeviceId(std::array<byte,8>& id);

  /**
   * @brief setup the duck unique ID
   *
   * @param an 8 byte unique id
   * @return DUCK_ERR_NONE if successful, an error code otherwise
   */
  int setDeviceId(std::string& id);

    /**
     * @brief setup the duck unique ID
     *
     * @param an 8 byte unique id
     * @return DUCK_ERR_NONE if successful, an error code otherwise
     */
  [[deprecated("use setDeviceId(std::array<byte,8>& id) instead")]]
  int setDeviceId(byte* id);
  /**
   * @brief Setup serial connection.
   *
   * @param baudRate default: 115200
   */
  int setupSerial(int baudRate = 115200);

  /**
   * @brief Setup the radio component
   *
   * @param band      radio frequency in Mhz (default: 915.0)
   * @param ss        slave select pin (default CDPCFG_PIN_LORA_CS)
   * @param rst       reset pin  (default: CDPCFG_PIN_LORA_RST)
   * @param di0       dio0 interrupt pin (default: CDPCFG_PIN_LORA_DIO0)
   * @param di1       dio1 interrupt pin (default: CDPCFG_PIN_LORA_DIO1)
   * @param txPower   transmit power (default: CDPCFG_RF_LORA_TXPOW)
   */
  int setupRadio(float band = CDPCFG_RF_LORA_FREQ, int ss = CDPCFG_PIN_LORA_CS,
                  int rst = CDPCFG_PIN_LORA_RST, int di0 = CDPCFG_PIN_LORA_DIO0,
                  int di1 = CDPCFG_PIN_LORA_DIO1,
                  int txPower = CDPCFG_RF_LORA_TXPOW,
                  float bw = CDPCFG_RF_LORA_BW,
                  uint8_t sf = CDPCFG_RF_LORA_SF,
                  uint8_t gain = CDPCFG_RF_LORA_GAIN);

  /**
   * @brief Set sync word used to communicate between radios. 0x12 for private and 0x34 for public channels.
   * 
   * @param syncWord set byte syncWord
   */
  void setSyncWord(byte syncWord);

  /**
   * @brief Set radio channel to transmit and receive on.
   * 
   * @param channelNum set radio channel 1-5 
   */
  void setChannel(int channelNum);

  /**
   * @brief Setup WiFi access point.
   *
   * @param accessPoint a string representing the access point. Default to
   * "🆘 DUCK EMERGENCY PORTAL"
   *
   * @returns DUCK_ERROR_NONE if successful, an error code otherwise.
   */
  int setupWifi(const char* ap = "🆘 DUCK EMERGENCY PORTAL");

  /**
   * @brief Setup DNS.
   *
   * @returns DUCK_ERROR_NONE if successful, an error code otherwise.
   */
  int setupDns();

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
  int setupWebServer(bool createCaptivePortal = false, std::string html = "");

  /**
   * @brief Setup internet access.
   *
   * @param ssid        the ssid of the WiFi network
   * @param password    password to join the network
   */
  int setupInternet(std::string ssid, std::string password);

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
    const std::array<byte,8> targetDevice = ZERO_DUID, std::array<byte,8> * outgoingMuid = NULL);

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
    const std::array<byte,8> targetDevice = ZERO_DUID, std::array<byte,8> * outgoingMuid = NULL);
    
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
    const std::array<byte,8> targetDevice = ZERO_DUID, std::array<byte,8> * outgoingMuid = NULL);

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
    const std::array<byte,8> targetDevice, const std::array<byte,8> &muid);

  /**
   * @brief Get the status of an MUID
   */
  muidStatus getMuidStatus(const std::array<byte,4> & muid) const;

  /**
   * @brief Check wifi connection status
   * 
   * @returns true if device wifi is connected, false otherwise. 
   */
  bool isWifiConnected();

  /**
   * @brief Get the access point ssid
   * 
   * @returns the wifi access point as a string
   */
  std::string getSsid();
  /**
   * @brief Get the wifi access point password.
   * 
   * @returns the wifi access point password as a string. 
   */
  std::string getPassword();

  /**
   * @brief Get an error code description.
   * 
   * @param error an error code
   * @returns a string describing the error. 
   */
  std::string getErrorString(int error);

  /**
   * @brief Turn on or off encryption.
   * 
   * @param state true for on, false for off
   */
  void setEncrypt(bool state);

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
  void setAESKey(uint8_t newKEY[32]);

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

protected:
  Duck(Duck const&) = delete;
  Duck& operator=(Duck const&) = delete;

  std::string duckName="";

  std::string deviceId;
  std::array<byte,8> duid;

  DuckRadio& duckRadio = DuckRadio::getInstance();


  DuckNet * const duckNet;

  DuckPacket* txPacket = NULL;
  DuckPacket* rxPacket = NULL;
  std::array<byte,4> lastMessageMuid;

  bool lastMessageAck = true;
  // Since this may be used to throttle outgoing packets, start out in a state
  // that indicates we're not waiting for a ack

  BloomFilter filter;

  /**
   * @brief sends a pong message
   * 
   * @return DUCK_ERR_NONE if successfull. An error code otherwise 
   */
  int sendPong();
  
  /**
   * @brief sends a ping message
   *
   * @return DUCK_ERR_NONE if successfull. An error code otherwise
   */
  int sendPing();

  /**
   * @brief Tell the duck radio to start receiving packets from the mesh network
   *
   * @return DUCK_ERR_NONE if successful, an error code otherwise
   */
  int startReceive();

  /**
   * @brief Implement the duck's specific behavior.
   * 
   * This method must be implemented by the Duck's concrete classes such as DuckLink, MamaDuck,...
   */
  virtual void run() = 0;

  /**
   * @brief Setup a duck with default settings
   *
   * The default implementation simply initializes the serial interface.
   * It can be overriden by each concrete Duck class implementation.
   */
  virtual int setupWithDefaults(std::array<byte,8> deviceId, std::string ssid, std::string password) {
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
  static void logIfLowMemory();

  static bool imAlive(void*);
  static bool reboot(void*);
};

#endif
