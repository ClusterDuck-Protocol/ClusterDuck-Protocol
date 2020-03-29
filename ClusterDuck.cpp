#include "ClusterDuck.h"

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

IPAddress apIP(192, 168, 1, 1);
AsyncWebServer webServer(80);

auto tymer = timer_create_default();

String ClusterDuck::_deviceId = ""; ///< holds the device id


/**
 * @brief Constructor
 */
ClusterDuck::ClusterDuck() {


}

/**
 * @brief Set device ID
 * 
 * @param deviceId used device ID. Do not leave deviceId _null_ or as an empty string.
 */
void ClusterDuck::setDeviceId(String deviceId) {
  _deviceId = deviceId;

}

/**
 * @brief Initialize baude rate for serial. Use in `setup()`.
 * 
 * @param baudRate desired baude rate
 */
void ClusterDuck::begin(int baudRate) {
  Serial.begin(baudRate);
  Serial.println("Serial start");
}

/**
 * @brief Initializes LED screen on Heltec LoRa ESP32 and configures it to show status, device ID, and the device type. Use in `setup()`.
 * 
 * @param deviceType TODO: valid values for deviceType
 */
void ClusterDuck::setupDisplay(String deviceType)  {
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);

  u8x8.setCursor(0, 1);
  u8x8.print("    ((>.<))    ");

  u8x8.setCursor(0, 2);
  u8x8.print("  Project OWL  ");
  
  u8x8.setCursor(0, 4);
  u8x8.print("Device: " + deviceType);
  
  u8x8.setCursor(0, 5);
  u8x8.print("Status: Online");

  u8x8.setCursor(0, 6);
  u8x8.print("ID:     " + _deviceId); 

  u8x8.setCursor(0, 7);
  u8x8.print(duckMac(false));
}

/**
 * @brief Initial LoRa settings
 * 
 * @param BAND TODO: Add defailt values for Heltec LoRa ESP32
 * @param SS TODO: Add defailt values for Heltec LoRa ESP32
 * @param RST TODO: Add defailt values for Heltec LoRa ESP32
 * @param DI0 TODO: Add defailt values for Heltec LoRa ESP32
 * @param TxPower TODO: Add defailt values for Heltec LoRa ESP32
 */
void ClusterDuck::setupLoRa(long BAND, int SS, int RST, int DI0, int TxPower) {
  SPI.begin(5, 19, 27, 18);
  LoRa.setPins(SS, RST, DI0);
  LoRa.setTxPower(TxPower);
  //LoRa.setSignalBandwidth(62.5E3);

  //Initialize LoRa
  if (!LoRa.begin(BAND))
  {
    u8x8.clear();
    u8x8.drawString(0, 0, "Starting LoRa failed!");
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  else
  {
    Serial.println("LoRa On");
  }

  //  LoRa.setSyncWord(0xF3);         // ranges from 0-0xFF, default 0x34
  LoRa.enableCrc();
}

//Setup Captive Portal
/**
 * @brief Setup Capative Portal
 * 
 * @param AP the value that will be displayed when accessing the wifi settings of devices such as smartphones and laptops.
 */
void ClusterDuck::setupPortal(const char *AP) {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP);
  delay(200); // wait for 200ms for the access point to start before configuring

  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  Serial.println("Created Hotspot");

  dnsServer.start(DNS_PORT, "*", apIP);

  Serial.println("Setting up Web Server");

  webServer.onNotFound([&](AsyncWebServerRequest *request) {
    request->send(200, "text/html", portal);
  });

  webServer.on("/", HTTP_GET, [&](AsyncWebServerRequest *request) {
    request->send(200, "text/html", portal);
  });

  // Captive Portal form submission
  webServer.on("/formSubmit", HTTP_POST, [&](AsyncWebServerRequest *request) {
    Serial.println("Submitting Form");

    int paramsNumber = request->params();
    String val = "";

    for (int i = 0; i < paramsNumber; i++) {
      AsyncWebParameter *p = request->getParam(i);
      Serial.printf("%s: %s", p->name().c_str(), p->value().c_str());
      Serial.println();

      val = val + p->value().c_str() + "*";
    }

    sendPayloadStandard(val);

    request->send(200, "text/html", portal);
  });

  webServer.on("/id", HTTP_GET, [&](AsyncWebServerRequest *request) {
    request->send(200, "text/html", _deviceId);
  });

  webServer.on("/restart", HTTP_GET, [&](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Restarting...");
    delay(1000);
    restartDuck();
  });

  webServer.on("/mac", HTTP_GET, [&](AsyncWebServerRequest *request) {
    String mac = duckMac(true);
    request->send(200, "text/html", mac);
  });

  // for captive portal
  webServer.addHandler(new CaptiveRequestHandler(MAIN_page)).setFilter(ON_AP_FILTER);

  // Test 👍👌😅

  webServer.begin();

  if (!MDNS.begin(DNS))
  {
    Serial.println("Error setting up MDNS responder!");
  }
  else
  {
    Serial.println("Created local DNS");
    MDNS.addService("http", "tcp", 80);
  }
}

/**
 * @brief Setup premade DuckLink with default settings
 */
void ClusterDuck::setupDuckLink() {
  setupDisplay("Duck");
  setupLoRa();
  setupPortal();

  Serial.println("Duck Online");
}

/**
 * @brief Template for running core functionality of a DuckLink.
 */
void ClusterDuck::runDuckLink() {

  processPortalRequest();

}

/**
 * @brief TODO
 */
void ClusterDuck::processPortalRequest() {

  dnsServer.processNextRequest();

}

/**
 * @brief Template for setting up a MamaDuck device.
 */
void ClusterDuck::setupMamaDuck() {
  setupDisplay("Mama");
  setupPortal();
  setupLoRa();

  //LoRa.onReceive(repeatLoRaPacket);
  LoRa.receive();

  Serial.println("MamaDuck Online");

  tymer.every(1800000, imAlive);
  tymer.every(43200000, reboot);
}

/**
 * @brief Template for running core functionality of a MamaDuck.
 */
void ClusterDuck::runMamaDuck() {
  tymer.tick();

  int packetSize = LoRa.parsePacket();
  if (packetSize != 0) {
    byte whoIsIt = LoRa.peek();
    if(whoIsIt == senderId_B) {
      String * msg = getPacketData(packetSize);
      if(!idInPath(_lastPacket.path)) {
        sendPayloadStandard(_lastPacket.payload, _lastPacket.senderId, _lastPacket.messageId, _lastPacket.path);
        LoRa.receive();
      }
      delete(msg);
    } else if(whoIsIt == ping_B) {
      LoRa.beginPacket();
      couple(iamhere_B, "1");
      LoRa.endPacket();
      Serial.println("Pong packet sent");
      LoRa.receive();
    }
  }

  processPortalRequest();

}

/**
 * @brief Packages msg into a LoRa packet and sends over LoRa.
 * Will automatically set the current device's ID as the sender ID and create a UUID for the message.
 * 
 * @param msg the message payload
 */
void ClusterDuck::sendPayloadMessage(String msg) {
  LoRa.beginPacket();
  couple(senderId_B, _deviceId);
  couple(messageId_B, uuidCreator());
  couple(payload_B, msg);
  couple(path_B, _deviceId);
  LoRa.endPacket(); 
}

/**
 * @brief Similar to and might replace sendPayloadMessage().
 * 
 * @param msg the message payload
 * @param senderId the ID of the originator of the message
 * @param messageId the UUID of the message
 * @param path the recorded pathway of the message and is used as a check to prevent the device from sending multiple of the same message.
 */
void ClusterDuck::sendPayloadStandard(String msg, String senderId, String messageId, String path) {
  if(senderId == "") senderId = _deviceId;
  if(messageId == "") messageId = uuidCreator();
  if(path == "") {
    path = _deviceId;
  } else {
    path = path + "," + _deviceId;
  }

  String total = senderId + messageId + path + msg;
  if(total.length() + 4 > 240) {
    Serial.println("Warning: message is too large!"); //TODO: do something
  }
  
  LoRa.beginPacket();
  couple(senderId_B, senderId);
  couple(messageId_B, messageId);
  couple(payload_B, msg);
  couple(path_B, path);
  LoRa.endPacket();
  Serial.println("Here7");
  
  Serial.println("Packet sent");
}

/**
 * @brief Writes data to LoRa packet.   
 * 
 * @ref setDeviceId() for byte codes. In addition it writes the outgoing length to the LoRa packet.
 * 
 * Use between a LoRa.beginPacket() and LoRa.endPacket() 
 * 
 * @note LoRa.endPacket() will send the LoRa packet
 * 
 * @param byteCode byteCode is paired with the outgoing so it can be used to identify data on an individual level.
 * @param outgoing the payload data to be sent
 */
void ClusterDuck::couple(byte byteCode, String outgoing) {
  LoRa.write(byteCode);               // add byteCode
  LoRa.write(outgoing.length());      // add payload length
  LoRa.print(outgoing);               // add payload
}

/**
 * @brief Decode LoRa message
 * Used after `LoRa.read()` to convert LoRa packet into a String.
 * 
 * @param mLength length of the incoming message
 * @return the converted string
 */
String ClusterDuck::readMessages(byte mLength)  {
  String incoming = "";

  for (int i = 0; i < mLength; i++)
  {
    incoming += (char)LoRa.read();
  }
  return incoming;
}

/**
 * @brief Checks if the path contains deviceId. Returns bool.
 * 
 * @param path TODO
 * @return true: if the path contains the deviceId
 */
bool ClusterDuck::idInPath(String path) {
  Serial.println("Checking Path");
  String temp = "";
  int len = path.length() + 1;
  char arr[len];
  path.toCharArray(arr, len);

  for (int i = 0; i < len; i++) {
    if (arr[i] == ',' || i == len - 1) {
      if (temp == _deviceId) {
        Serial.print(path);
        Serial.print("false");
        return true;
      }
      temp = "";
    } else {
      temp += arr[i];
    }
  }
  Serial.println("true");
  Serial.println(path);
  return false;
}

/**
 * @brief Called to iterate through received LoRa packet and return data as an array of Strings.
 * @note: if using standard byte codes it will store senderId, messageId, payload, and path in a Packet object. This can be accessed using getLastPacket()
 * 
 * @param pSize TODO
 * @return TODO
 */
String * ClusterDuck::getPacketData(int pSize) {
  String * packetData = new String[pSize];
  int i = 0;
  byte byteCode, mLength;

  while (LoRa.available())
  {
    byteCode = LoRa.read();
    mLength  = LoRa.read();
    if (byteCode == senderId_B)
    {
      _lastPacket.senderId  = readMessages(mLength);
      Serial.println("User ID: " + _lastPacket.senderId);
    }
    else if (byteCode == messageId_B) {
      _lastPacket.messageId = readMessages(mLength);
      Serial.println("Message ID: " + _lastPacket.messageId);
    }
    else if (byteCode == payload_B) {
      _lastPacket.payload = readMessages(mLength);
      Serial.println("Message: " + _lastPacket.payload);
    }
    else if (byteCode == path_B) {
      _lastPacket.path = readMessages(mLength);
      Serial.println("Path: " + _lastPacket.path);
    } else {
      packetData[i] = readMessages(mLength);
      _lastPacket.payload = _lastPacket.payload + packetData[i];
      _lastPacket.payload = _lastPacket.payload + "*";
      //Serial.println("Data" + i + ": " + packetData[i]);
      i++;
    }
  }
  return packetData;
}

/**
 * @brief If using the ESP32 architecture, calling this function will reboot the device.
 */
void ClusterDuck::restartDuck()
{
  Serial.println("Restarting Duck...");
  ESP.restart();
}

/**
 * @brief Used to call `restartDuck()` when using a timer
 * @details [long description]
 * 
 * @return true TODO: why this is not a void function?
 */
bool ClusterDuck::reboot(void *) {
  String reboot = "REBOOT";
  Serial.println(reboot);
  sendPayloadMessage(reboot);
  restartDuck();

  return true;
}

/**
 * @brief Used to send a '1' over LoRa on a timer to signify the device is still on and functional.
 * 
 * @return true TODO: why this is not a void function?
 */
bool ClusterDuck::imAlive(void *) {
  String alive = "1";
  sendPayloadMessage(alive);
  Serial.print("alive");

  return true;
}

/**
 * @brief Returns the MAC address of the device. 
 * @details [long description]
 * 
 * @param format Using `true` as an argument will return the MAC address formatted using ':'
 * @return MAC address
 */
String ClusterDuck::duckMac(boolean format)
{
  char id1[15];
  char id2[15];

  uint64_t chipid = ESP.getEfuseMac(); // The chip ID is essentially its MAC address(length: 6 bytes).
  uint16_t chip = (uint16_t)(chipid >> 32);

  snprintf(id1, 15, "%04X", chip);
  snprintf(id2, 15, "%08X", (uint32_t)chipid);

  String ID1 = id1;
  String ID2 = id2;

  String unformattedMac = ID1 + ID2;

  if(format == true){
    String formattedMac = "";
    for(int i = 0; i < unformattedMac.length(); i++){
      if(i % 2 == 0 && i != 0){
        formattedMac += ":";
        formattedMac += unformattedMac[i];
      } 
      else {
        formattedMac += unformattedMac[i];
      }
    }
    return formattedMac;
  } else {
    return unformattedMac;
  }
}

/**
 * @brief Create a uuid
 * @return uuid
 */
String ClusterDuck::uuidCreator() {
  byte randomValue;
  char msg[50];
  int numBytes = 0;
  int i;

  numBytes = atoi("8");
  if (numBytes > 0)
  {
    memset(msg, 0, sizeof(msg));
    for (i = 0; i < numBytes; i++) {
      randomValue = random(0, 37);
      msg[i] = randomValue + 'a';
      if (randomValue > 26) {
        msg[i] = (randomValue - 26) + '0';
      }
    }
  }

  return String(msg);
}

/**
 * @brief TODO
 */
void ClusterDuck::loRaReceive() {
  LoRa.receive();
}

/**
 * @brief Returns the device ID
 * @return device ID
 */
String ClusterDuck::getDeviceId() {
  return _deviceId;
}

/**
 * @brief Returns a packet object containing senderId, messageId, payload, and path of last packet received.
 * @note values are updated after running `getPacketData()`
 * @return packet object
 */
Packet ClusterDuck::getLastPacket() {
  Packet packet = _lastPacket;
  _lastPacket = Packet();
  return packet;
}

DNSServer ClusterDuck::dnsServer;
const char * ClusterDuck::DNS  = "duck";
const byte ClusterDuck::DNS_PORT = 53;

int ClusterDuck::_rssi = 0;
float ClusterDuck::_snr;
long ClusterDuck::_freqErr;
int ClusterDuck::_availableBytes;
int ClusterDuck::_packetSize = 0;

Packet ClusterDuck::_lastPacket;

byte ClusterDuck::ping_B       = 0xF4;
byte ClusterDuck::senderId_B   = 0xF5;
byte ClusterDuck::messageId_B  = 0xF6;
byte ClusterDuck::payload_B    = 0xF7;
byte ClusterDuck::iamhere_B    = 0xF8;
byte ClusterDuck::path_B       = 0xF3;

String ClusterDuck::portal = MAIN_page;
