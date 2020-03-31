#include "ClusterDuck.h"

#include <ArduinoJson.h>

  //static variable definition
  byte DuckLink::messageId_B = 0xF6;
  byte DuckLink::senderId_B = 0xF5; 
  byte DuckLink::payload_B=0xF7; 
  byte DuckLink::path_B=0xF3;
  String DuckLink::_deviceId = "";

DuckLink::DuckLink(/* args */) :
  _rssi(0),
  _snr(0),
  _freqErr(0),
  _availableBytes(0),
  _packetSize(0),
  dnsServer(),
  DNS_PORT(53),
  DNS("duck"),
  AP(0),
  portal(MAIN_page),
  runTime(""),
    u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16),
  apIP(192, 168, 1, 1),
  webServer(80),
  ping_B(0xF4), 
  iamhere_B(0xF8), 
  _lastPacket(),
  tymer(timer_create_default())
{

}

DuckLink::~DuckLink()
{
}

void DuckLink::setDeviceId(String deviceId) {
  _deviceId = deviceId;

}

void DuckLink::begin(int baudRate) {
  Serial.begin(baudRate);
  Serial.println("Serial start");
}

void DuckLink::setupDisplay(String deviceType)  {
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
void DuckLink::setupLoRa(long BAND, int SS, int RST, int DI0, int TxPower) {
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
void DuckLink::setupPortal(const char *AP) {
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
    restart();
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
void DuckLink::setup() {
  setupDisplay("Duck");
  setupLoRa();
  setupPortal();

  Serial.println("Duck Online");
}

void DuckLink::run() {

  processPortalRequest();

}

void DuckLink::processPortalRequest() {

  dnsServer.processNextRequest();

}

void DuckLink::sendPayloadMessage(String msg) {
  LoRa.beginPacket();
  couple(senderId_B, _deviceId);
  couple(messageId_B, uuidCreator());
  couple(payload_B, msg);
  couple(path_B, _deviceId);
  LoRa.endPacket(); 
}

void DuckLink::sendPayloadStandard(String msg, String senderId, String messageId, String path) {
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

void DuckLink::couple(byte byteCode, String outgoing) {
  LoRa.write(byteCode);               // add byteCode
  LoRa.write(outgoing.length());      // add payload length
  LoRa.print(outgoing);               // add payload
}

//Decode LoRa message
String DuckLink::readMessages(byte mLength)  {
  String incoming = "";

  for (int i = 0; i < mLength; i++)
  {
    incoming += (char)LoRa.read();
  }
  return incoming;
}

bool MamaDuck::idInPath(String path) {
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

String * DuckLink::getPacketData(int pSize) {
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
  restart
  Only restarts ESP
*/
void DuckLink::restart()
{
  Serial.println("Restarting Duck...");
  ESP.restart();
}

//Timer reboot
bool DuckLink::reboot(void *) {
  String reboot = "REBOOT";
  Serial.println(reboot);
  sendPayloadMessage(reboot);
  restart();

  return true;
}

bool DuckLink::imAlive(void *) {
  String alive = "1";
  sendPayloadMessage(alive);
  Serial.print("alive");

  return true;
}

//Get Duck MAC address
String DuckLink::duckMac(boolean format)
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
String DuckLink::uuidCreator() {
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

void DuckLink::loRaReceive() {
  LoRa.receive();
}

//Getters

String DuckLink::getDeviceId() {
  return _deviceId;
}

Packet DuckLink::getLastPacket() {
  Packet packet = _lastPacket;
  _lastPacket = Packet();
  return packet;
}

MamaDuck::MamaDuck(/* args */)
{
}

MamaDuck::~MamaDuck()
{
}

void MamaDuck::setup() {
  setupDisplay("Mama");
  setupPortal();
  setupLoRa();

  //LoRa.onReceive(repeatLoRaPacket);
  LoRa.receive();

  Serial.println("MamaDuck Online");

  tymer.every(1800000, imAlive);
  tymer.every(43200000, reboot);
}

void MamaDuck::run() {
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

PapaDuck::PapaDuck(String ssid, String password, String org, String deviceId, String deviceType, String token, String server, String topic, String authMethod, String clientId) :
  m_ssid(ssid), 
  m_password(password), 
  m_org(org), 
  m_deviceId(deviceId), 
  m_deviceType(deviceType), 
  m_token(token), 
  m_server(server), 
  m_topic(topic), 
  m_authMethod(authMethod), 
  m_clientId(clientId), 
  m_timer(timer_create_default()),
  m_pubSubClient(m_server.c_str(), 8883, m_wifiClient),
  m_ping(0xF4)
{
}

PapaDuck::~PapaDuck()
{
}

void PapaDuck::setup()
{
  setupDisplay("Papa");
  setupLoRa();
  LoRa.receive();
  
  setupWiFi();
  
  Serial.println("PAPA Online");
}
void PapaDuck::run()
{
  if(WiFi.status() != WL_CONNECTED)
  {
    Serial.print("WiFi disconnected, reconnecting to local network: ");
    Serial.print(m_ssid);
    setupWiFi();

  }
  setupMQTT();

  int packetSize = LoRa.parsePacket();
  if (packetSize != 0) {
    byte whoIsIt = LoRa.peek();
    if(whoIsIt != m_ping) {
      Serial.println(packetSize);
      String * val = getPacketData(packetSize);
      quackJson();
    }
  }

  
  m_timer.tick();
}

void PapaDuck::setupWiFi()
{
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(m_ssid);

  // Connect to Access Poink
  WiFi.begin(m_ssid.c_str(), m_password.c_str());

  while (WiFi.status() != WL_CONNECTED)
  {
    m_timer.tick(); //Advance timer to reboot after awhile
    //delay(500);
    Serial.print(".");
  }

  // Connected to Access Point
  Serial.println("");
  Serial.println("WiFi connected - PAPA ONLINE");
}

void PapaDuck::setupMQTT()
{
  if (!!!m_pubSubClient.connected()) {
    Serial.print("Reconnecting client to "); Serial.println(m_server);
    while ( ! (m_org == "quickstart" ? m_pubSubClient.connect(m_clientId.c_str()) : m_pubSubClient.connect(m_clientId.c_str(), m_authMethod.c_str(), m_token.c_str())))
    {
      m_timer.tick(); //Advance timer to reboot after awhile
      Serial.print("i");
      delay(500);
    }
  }
}

void PapaDuck::quackJson() {
  const int bufferSize = 4*  JSON_OBJECT_SIZE(4);
  StaticJsonDocument<bufferSize> doc;

  JsonObject root = doc.as<JsonObject>();

  Packet lastPacket = getLastPacket();

  doc["DeviceID"]        = lastPacket.senderId;
  doc["MessageID"]       = lastPacket.messageId;
  doc["Payload"]     .set(lastPacket.payload);
  doc["path"]         .set(lastPacket.path + "," + getDeviceId());


  String jsonstat;
  serializeJson(doc, jsonstat);

  if (m_pubSubClient.publish(m_topic.c_str(), jsonstat.c_str())) {
    
    serializeJsonPretty(doc, Serial);
     Serial.println("");
    Serial.println("Publish ok");
   
  }
  else {
    Serial.println("Publish failed");
  }

}