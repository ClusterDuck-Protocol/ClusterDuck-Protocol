#include "ClusterDuck.h"

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

IPAddress apIP(192, 168, 1, 1);
AsyncWebServer webServer(80);

SX1276 lora = new Module(18, 26, 14, 25);

auto tymer = timer_create_default();

byte transmission[250];
int packetIndex = 0;

String ClusterDuck::_deviceId = "";

ClusterDuck::ClusterDuck() {


}

void ClusterDuck::setDeviceId(String deviceId) {
  _deviceId = deviceId;

}

void ClusterDuck::begin(int baudRate) {
  Serial.begin(baudRate);
  Serial.println("Serial start");
}

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

// Initial LoRa settings
void ClusterDuck::setupLoRa(long BAND, int SS, int RST, int DI0, int DI1, int TxPower) {
  //LoRa.setSignalBandwidth(62.5E3);

  lora = new Module(SS, DI0, RST, DI1);

  Serial.println("Starting LoRa......");

  int state = lora.begin(BAND,125.0,7,7,TxPower,1); //TODO: Make more modular -> Bandwidth, Spreading factor

  //Initialize LoRa
  if (state == ERR_NONE) {
    Serial.println("LoRa online, Quack!");
  } else {
    u8x8.clear();
    u8x8.drawString(0, 0, "Starting LoRa failed!");
    Serial.print("Starting LoRa Failed!!!");
    Serial.println(state);
    while (true);
  }

  lora.setDio0Action(setFlag);

  state = lora.startReceive();
  
  if (state == ERR_NONE) {
    Serial.println("Listening for quacks");
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
}

//============= Used for receiving LoRa packets ==========
volatile bool receivedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

void ClusterDuck::setFlag(void) {
  // check if the interrupt is enabled
  if(!enableInterrupt) {
    return;
  }
  // we got a packet, set the flag
  receivedFlag = true;
}

//=========================================================

//Setup Captive Portal
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

//Setup premade DuckLink with default settings
void ClusterDuck::setupDuckLink() {
  setupDisplay("Duck");
  setupLoRa();
  setupPortal();

  Serial.println("Duck Online");
}

void ClusterDuck::runDuckLink() {

  processPortalRequest();

}

void ClusterDuck::processPortalRequest() {

  dnsServer.processNextRequest();

}

void ClusterDuck::setupMamaDuck() {
  setupDisplay("Mama");
  setupPortal();
  setupLoRa();

  Serial.println("MamaDuck Online");

  tymer.every(1800000, imAlive);
  tymer.every(43200000, reboot);
}

int ClusterDuck::handlePacket() {
  int pSize = lora.getPacketLength();
  int state = lora.readData(transmission, pSize);

  if (state == ERR_NONE) {
    // packet was successfully received
    Serial.println("Packet Received!");

    return pSize;
  } else {
    // some other error occurred
    Serial.print("Failed, code ");
    Serial.println(state);

    return -1;
  }
}

void ClusterDuck::runMamaDuck() {
  tymer.tick();

  if(receivedFlag) {  //If LoRa packet received
    int pSize = handlePacket();
    if(pSize > 0) {
      byte whoIsIt = transmission[0];
      if(whoIsIt == senderId_B) {
        String * msg = getPacketData(pSize);
        if(!idInPath(_lastPacket.path)) {
          sendPayloadStandard(_lastPacket.payload, _lastPacket.senderId, _lastPacket.messageId, _lastPacket.path);
          
          lora.startReceive();
          enableInterrupt = true;
        }
        delete(msg);
      } else if(whoIsIt == ping_B) {
        memset(transmission, 0x00, packetIndex);
        packetIndex = 0;
        couple(iamhere_B, "1");
        int state = lora.transmit(transmission, packetIndex);
        
        lora.startReceive();
        enableInterrupt = true;
      }
      
    } else {
      Serial.println("Byte code not recognized!"); 
      memset(transmission, 0x00, pSize); //Reset transmission

    }
    
  }

  processPortalRequest();

}

void ClusterDuck::sendPayloadMessage(String msg) {
  couple(senderId_B, _deviceId);
  couple(messageId_B, uuidCreator());
  couple(payload_B, msg);
  couple(path_B, _deviceId);

  int state = lora.transmit(transmission, packetIndex);

  memset(transmission, 0x00, packetIndex); //Reset transmission
  packetIndex = 0; //Reset packetIndex

  if (state == ERR_NONE) {
    // the packet was successfully transmitted
    Serial.println("Packet sent");
  } else if (state == ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 256 bytes
    Serial.println(F(" too long!"));
  } else if (state == ERR_TX_TIMEOUT) {
    // timeout occured while transmitting packet
    Serial.println(F(" timeout!"));
  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);
  }
}

void ClusterDuck::sendPayloadStandard(String msg, String senderId, String messageId, String path) {
  if(senderId == "") senderId = _deviceId;
  if(messageId == "") messageId = uuidCreator();
  if(path == "") {
    path = _deviceId;
  } else {
    path = path + "," + _deviceId;
  }

  String total = senderId + messageId + path + msg;
  if(total.length() + 4 > 250) {
    Serial.println("Warning: message is too large!"); //TODO: do something
  }

  if(packetIndex < 1) {
    packetIndex = total.length();
  }
  
  couple(senderId_B, senderId);
  couple(messageId_B, messageId);
  couple(payload_B, msg);
  couple(path_B, path);

  int state = lora.transmit(transmission, packetIndex);

  memset(transmission, 0x00, packetIndex); //Reset transmission
  packetIndex = 0; //Reset packetIndex

  if (state == ERR_NONE) {
    // the packet was successfully transmitted
    Serial.println("Packet sent");
  } else if (state == ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 256 bytes
    Serial.println(F(" too long!"));
  } else if (state == ERR_TX_TIMEOUT) {
    // timeout occured while transmitting packet
    Serial.println(F(" timeout!"));
  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

}

void ClusterDuck::couple(byte byteCode, String outgoing) {
  int outgoingLen = outgoing.length() + 1;
  byte byteBuffer[outgoingLen];

  outgoing.getBytes(byteBuffer, outgoingLen);

  transmission[packetIndex] = byteCode; //add byte code
  packetIndex++;
  transmission[packetIndex + 1] = (byte)outgoingLen; // add payload length
  packetIndex++;

  for(int i=0; i < outgoingLen; i++) {  // add payload
    transmission[i+2] = byteBuffer[i];
  }

  packetIndex = packetIndex + outgoingLen;

}

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

String * ClusterDuck::getPacketData(int pSize) {
  String * packetData = new String[pSize];
  packetIndex = 0;
  int len = 0;
  byte byteCode;
  bool sId, mId, pLoad, pth;
  String msg = "";
  bool gotLen = false;

  for(int i=0; i < pSize; i++) {
    if(i > 0 && len == 0) {
      gotLen = false;
      if(sId) {
        _lastPacket.senderId  = msg;
        Serial.println("User ID: " + _lastPacket.senderId);
        msg = "";
        sId = false;

      } else if(mId) {
        _lastPacket.messageId = msg;
        Serial.println("Message ID: " + _lastPacket.messageId);
        msg = "";
        mId = false;
      } else if(pLoad) {
        _lastPacket.payload = msg;
        Serial.println("Message: " + _lastPacket.payload);
        msg = "";
        pLoad = false;
      } else if(pth) {
        _lastPacket.path = msg;
        Serial.println("Path: " + _lastPacket.path);
        msg = "";
        pth = false;
      }
    }
    if(transmission[i] == senderId_B){
      Serial.println(transmission[1+i]);
      sId = true;
      len = transmission[i+1];
      Serial.println("Len = " + String(len));

    } else if(transmission[i] == messageId_B) {
      mId = true;
      len = transmission[i+1];
      Serial.println("Len = " + String(len));

    } else if(transmission[i] == payload_B) {
      pLoad = true;
      len = transmission[i+1];
      Serial.println("Len = " + String(len));

    } else if(transmission[i] == path_B) {
      pth = true;
      len = transmission[i+1];
      Serial.println("Len = " + String(len));

    } else if(len > 0 && gotLen) {
      msg = msg + String((char)transmission[i]);
      len--;
    }
    else {
      gotLen = true;
    }
    packetIndex++;
  }

  if(len == 0) {
    if(sId) {
      _lastPacket.senderId  = msg;
      Serial.println("User ID: " + _lastPacket.senderId);
      msg = "";

    } else if(mId) {
      _lastPacket.messageId = msg;
      Serial.println("Message ID: " + _lastPacket.messageId);
      msg = "";

    } else if(pLoad) {
      _lastPacket.payload = msg;
      Serial.println("Message: " + _lastPacket.payload);
      msg = "";

    } else if(pth) {
      _lastPacket.path = msg;
      Serial.println("Path: " + _lastPacket.path);
      msg = "";
    }
  }
  
  return packetData;
}

/**
  restart
  Only restarts ESP
*/
void ClusterDuck::restartDuck()
{
  Serial.println("Restarting Duck...");
  ESP.restart();
}

//Timer reboot
bool ClusterDuck::reboot(void *) {
  String reboot = "REBOOT";
  Serial.println(reboot);
  sendPayloadMessage(reboot);
  restartDuck();

  return true;
}

bool ClusterDuck::imAlive(void *) {
  String alive = "1";
  sendPayloadMessage(alive);
  Serial.print("alive");

  return true;
}

//Get Duck MAC address
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

//Create a uuid
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

//Getters

String ClusterDuck::getDeviceId() {
  return _deviceId;
}

Packet ClusterDuck::getLastPacket() {
  Packet packet = _lastPacket;
  _lastPacket = Packet();
  return packet;
}

volatile bool ClusterDuck::getFlag() {
  return receivedFlag;
}

volatile bool ClusterDuck::getInterrupt() {
  return enableInterrupt;
}

//Setter
void ClusterDuck::flipFlag() {
  !receivedFlag;
}

void ClusterDuck::flipInterrupt() {
  !enableInterrupt;
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