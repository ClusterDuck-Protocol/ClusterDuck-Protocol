#include "ClusterDuck.h"

#include <ArduinoJson.h>

// static variable definition
byte DuckLink::m_messageId_B = 0xF6;
byte DuckLink::m_senderId_B  = 0xF5;
byte DuckLink::m_payload_B   = 0xF7;
byte DuckLink::m_path_B      = 0xF3;
String DuckLink::m_deviceId = "";

/**
 * @brief Construct a new Duck Link:: Duck Link object
 *
 */
DuckLink::DuckLink(/* args */)
    : m_rssi(0), m_snr(0), m_freqErr(0), m_availableBytes(0), m_packetSize(0), m_dnsServer(), m_dnsPort(53), m_dns("duck"), m_ap(0),
      m_portal(MAIN_page), m_runTime(""), m_u8x8(/* clock=*/15, /* data=*/4, /* reset=*/16), m_apIp(192, 168, 1, 1),
      m_webServer(80), m_ping_B(0xF4), m_iamhere_B(0xF8), m_lastPacket()
{
}

/**
 * @brief Destroy the Duck Link:: Duck Link object
 *
 */
DuckLink::~DuckLink() {}

/**
 * @brief Set device ID
 *
 * @param deviceId used device ID. Do not leave deviceId _null_ or as an empty string.
 */
void DuckLink::setDeviceId(String deviceId)
{
    m_deviceId = deviceId;
}

/**
 * @brief Initialize baude rate for serial. Use in `setup()`.
 *
 * @param baudRate desired baude rate
 */
void DuckLink::begin(int baudRate)
{
    Serial.begin(baudRate);
    Serial.println("Serial start");
}

/**
 * @brief Initializes LED screen on Heltec LoRa ESP32 and configures it to show status, device ID, and the device type.
 * Use in `setup()`.
 *
 * @param deviceType TODO: valid values for deviceType
 */
void DuckLink::setupDisplay(String deviceType)
{
    m_u8x8.begin();
    m_u8x8.setFont(u8x8_font_chroma48medium8_r);

    m_u8x8.setCursor(0, 1);
    m_u8x8.print("    ((>.<))    ");

    m_u8x8.setCursor(0, 2);
    m_u8x8.print("  Project OWL  ");

    m_u8x8.setCursor(0, 4);
    m_u8x8.print("Device: " + deviceType);

    m_u8x8.setCursor(0, 5);
    m_u8x8.print("Status: Online");

    m_u8x8.setCursor(0, 6);
    m_u8x8.print("ID:     " + m_deviceId);

    m_u8x8.setCursor(0, 7);
    m_u8x8.print(duckMac(false));
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
void DuckLink::setupLoRa(long BAND, int SS, int RST, int DI0, int TxPower)
{
    SPI.begin(5, 19, 27, 18);
    LoRa.setPins(SS, RST, DI0);
    LoRa.setTxPower(TxPower);
    // LoRa.setSignalBandwidth(62.5E3);

    // Initialize LoRa
    if (!LoRa.begin(BAND)) {
        m_u8x8.clear();
        m_u8x8.drawString(0, 0, "Starting LoRa failed!");
        Serial.println("Starting LoRa failed!");
        while (1)
            ;
    } else {
        Serial.println("LoRa On");
    }

    //  LoRa.setSyncWord(0xF3);         // ranges from 0-0xFF, default 0x34
    LoRa.enableCrc();
}

/**
 * @brief Setup Capative Portal
 *
 * @param AP the value that will be displayed when accessing the wifi settings of devices such as smartphones and
 * laptops.
 */
void DuckLink::setupPortal(const char *AP)
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP);
    delay(200); // wait for 200ms for the access point to start before configuring

    WiFi.softAPConfig(m_apIp, m_apIp, IPAddress(255, 255, 255, 0));

    Serial.println("Created Hotspot");

    m_dnsServer.start(m_dnsPort, "*", m_apIp);

    Serial.println("Setting up Web Server");

    m_webServer.onNotFound([&](AsyncWebServerRequest *request) { request->send(200, "text/html", m_portal); });

    m_webServer.on("/", HTTP_GET, [&](AsyncWebServerRequest *request) { request->send(200, "text/html", m_portal); });

    // Captive Portal form submission
    m_webServer.on("/formSubmit", HTTP_POST, [&](AsyncWebServerRequest *request) {
        Serial.println("Submitting Form");

        int paramsNumber = request->params();
        String val       = "";

        for (int i = 0; i < paramsNumber; i++) {
            AsyncWebParameter *p = request->getParam(i);
            Serial.printf("%s: %s", p->name().c_str(), p->value().c_str());
            Serial.println();

            val = val + p->value().c_str() + "*";
        }

        sendPayloadStandard(val);

        request->send(200, "text/html", m_portal);
    });

    m_webServer.on("/id", HTTP_GET, [&](AsyncWebServerRequest *request) { request->send(200, "text/html", m_deviceId); });

    m_webServer.on("/restart", HTTP_GET, [&](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Restarting...");
        delay(1000);
        restart();
    });

    m_webServer.on("/mac", HTTP_GET, [&](AsyncWebServerRequest *request) {
        String mac = duckMac(true);
        request->send(200, "text/html", mac);
    });

    // for captive portal
    m_webServer.addHandler(new CaptiveRequestHandler(MAIN_page)).setFilter(ON_AP_FILTER);

    // Test 👍👌😅

    m_webServer.begin();

    if (!MDNS.begin(m_dns)) {
        Serial.println("Error setting up MDNS responder!");
    } else {
        Serial.println("Created local DNS");
        MDNS.addService("http", "tcp", 80);
    }
}

/**
 * @brief Setup premade DuckLink with default settings
 */
void DuckLink::setup()
{
    setupDisplay("Duck");
    setupLoRa();
    setupPortal();

    Serial.println("Duck Online");
}

/**
 * @brief Template for running core functionality of a DuckLink.
 */
void DuckLink::run()
{

    processPortalRequest();
}

/**
 * @brief TODO
 */
void DuckLink::processPortalRequest()
{

    m_dnsServer.processNextRequest();
}

/**
 * @brief Packages msg into a LoRa packet and sends over LoRa.
 * Will automatically set the current device's ID as the sender ID and create a UUID for the message.
 *
 * @param msg the message payload
 */
void DuckLink::sendPayloadMessage(String msg)
{
    LoRa.beginPacket();
    couple(m_senderId_B, m_deviceId);
    couple(m_messageId_B, uuidCreator());
    couple(m_payload_B, msg);
    couple(m_path_B, m_deviceId);
    LoRa.endPacket();
}

/**
 * @brief Similar to and might replace sendPayloadMessage().
 *
 * @param msg the message payload
 * @param senderId the ID of the originator of the message
 * @param messageId the UUID of the message
 * @param path the recorded pathway of the message and is used as a check to prevent the device from sending multiple of
 * the same message.
 */
void DuckLink::sendPayloadStandard(String msg, String senderId, String messageId, String path)
{
    if (senderId == "")
        senderId = m_deviceId;
    if (messageId == "")
        messageId = uuidCreator();
    if (path == "") {
        path = m_deviceId;
    } else {
        path = path + "," + m_deviceId;
    }

    String total = senderId + messageId + path + msg;
    if (total.length() + 4 > 240) {
        Serial.println("Warning: message is too large!"); // TODO: do something
    }

    LoRa.beginPacket();
    couple(m_senderId_B, senderId);
    couple(m_messageId_B, messageId);
    couple(m_payload_B, msg);
    couple(m_path_B, path);
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
void DuckLink::couple(byte byteCode, String outgoing)
{
    LoRa.write(byteCode);          // add byteCode
    LoRa.write(outgoing.length()); // add payload length
    LoRa.print(outgoing);          // add payload
}

/**
 * @brief Decode LoRa message
 * Used after `LoRa.read()` to convert LoRa packet into a String.
 *
 * @param mLength length of the incoming message
 * @return the converted string
 */
String DuckLink::readMessages(byte mLength)
{
    String incoming = "";

    for (int i = 0; i < mLength; i++) {
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
bool MamaDuck::idInPath(String path)
{
    Serial.println("Checking Path");
    String temp = "";
    int len     = path.length() + 1;
    char arr[len];
    path.toCharArray(arr, len);

    for (int i = 0; i < len; i++) {
        if (arr[i] == ',' || i == len - 1) {
            if (temp == m_deviceId) {
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
 * @note: if using standard byte codes it will store senderId, messageId, payload, and path in a Packet object. This can
 * be accessed using getLastPacket()
 *
 * @param pSize TODO
 * @return TODO
 */
String *DuckLink::getPacketData(int pSize)
{
    String *packetData = new String[pSize];
    int i              = 0;
    byte byteCode, mLength;

    while (LoRa.available()) {
        byteCode = LoRa.read();
        mLength  = LoRa.read();
        if (byteCode == m_senderId_B) {
            m_lastPacket.senderId = readMessages(mLength);
            Serial.println("User ID: " + m_lastPacket.senderId);
        } else if (byteCode == m_messageId_B) {
            m_lastPacket.messageId = readMessages(mLength);
            Serial.println("Message ID: " + m_lastPacket.messageId);
        } else if (byteCode == m_payload_B) {
            m_lastPacket.payload = readMessages(mLength);
            Serial.println("Message: " + m_lastPacket.payload);
        } else if (byteCode == m_path_B) {
            m_lastPacket.path = readMessages(mLength);
            Serial.println("Path: " + m_lastPacket.path);
        } else {
            packetData[i]       = readMessages(mLength);
            m_lastPacket.payload = m_lastPacket.payload + packetData[i];
            m_lastPacket.payload = m_lastPacket.payload + "*";
            // Serial.println("Data" + i + ": " + packetData[i]);
            i++;
        }
    }
    return packetData;
}

/**
 * @brief If using the ESP32 architecture, calling this function will reboot the device.
 */
void DuckLink::restart()
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
bool DuckLink::reboot(void *)
{
    String reboot = "REBOOT";
    Serial.println(reboot);
    sendPayloadMessage(reboot);
    restart();

    return true;
}

/**
 * @brief Used to send a '1' over LoRa on a timer to signify the device is still on and functional.
 *
 * @return true TODO: why this is not a void function?
 */
bool DuckLink::imAlive(void *)
{
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
String DuckLink::duckMac(boolean format)
{
    char id1[15];
    char id2[15];

    uint64_t chipid = ESP.getEfuseMac(); // The chip ID is essentially its MAC address(length: 6 bytes).
    uint16_t chip   = (uint16_t)(chipid >> 32);

    snprintf(id1, 15, "%04X", chip);
    snprintf(id2, 15, "%08X", (uint32_t)chipid);

    String ID1 = id1;
    String ID2 = id2;

    String unformattedMac = ID1 + ID2;

    if (format == true) {
        String formattedMac = "";
        for (int i = 0; i < unformattedMac.length(); i++) {
            if (i % 2 == 0 && i != 0) {
                formattedMac += ":";
                formattedMac += unformattedMac[i];
            } else {
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
String DuckLink::uuidCreator()
{
    byte randomValue;
    char msg[50];
    int numBytes = 0;
    int i;

    numBytes = atoi("8");
    if (numBytes > 0) {
        memset(msg, 0, sizeof(msg));
        for (i = 0; i < numBytes; i++) {
            randomValue = random(0, 37);
            msg[i]      = randomValue + 'a';
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
void DuckLink::loRaReceive()
{
    LoRa.receive();
}

/**
 * @brief Returns the device ID
 * @return device ID
 */
String DuckLink::getDeviceId()
{
    return m_deviceId;
}

/**
 * @brief Returns a packet object containing senderId, messageId, payload, and path of last packet received.
 * @note values are updated after running `getPacketData()`
 * @return packet object
 */
Packet DuckLink::getLastPacket()
{
    Packet packet = m_lastPacket;
    m_lastPacket   = Packet();
    return packet;
}

/**
 * @brief Construct a new Mama Duck:: Mama Duck object
 *
 */
MamaDuck::MamaDuck(/* args */) : m_timer(timer_create_default()) {}

/**
 * @brief Destroy the Mama Duck:: Mama Duck object
 *
 */
MamaDuck::~MamaDuck() {}

/**
 * @brief Template for setting up a MamaDuck device.
 *
 */
void MamaDuck::setup()
{
    setupDisplay("Mama");
    setupPortal();
    setupLoRa();

    // LoRa.onReceive(repeatLoRaPacket);
    LoRa.receive();

    Serial.println("MamaDuck Online");

    m_timer.every(1800000, imAlive);
    m_timer.every(43200000, reboot);
}

/**
 * @brief Template for running core functionality of a MamaDuck.
 *
 */
void MamaDuck::run()
{
    m_timer.tick();

    int packetSize = LoRa.parsePacket();
    if (packetSize != 0) {
        byte whoIsIt = LoRa.peek();
        if (whoIsIt == m_senderId_B) {
            String *msg = getPacketData(packetSize);
            if (!idInPath(m_lastPacket.path)) {
                sendPayloadStandard(m_lastPacket.payload, m_lastPacket.senderId, m_lastPacket.messageId, m_lastPacket.path);
                LoRa.receive();
            }
            delete (msg);
        } else if (whoIsIt == m_ping_B) {
            LoRa.beginPacket();
            couple(m_iamhere_B, "1");
            LoRa.endPacket();
            Serial.println("Pong packet sent");
            LoRa.receive();
        }
    }

    processPortalRequest();
}

/**
 * @brief Construct a new Papa Duck:: Papa Duck object
 *
 * @param ssid        SSID of the wifi network to connect. Needs internet access
 * @param password    password used to connect to the wifi network
 * @param org         name of the organisation
 * @param deviceId    unique identifier of the duck
 * @param deviceType  device type: duck, mama, papa (TODO: Should this be a enum - and anyway isn't this always papa
 * here?)
 * @param token       access token for MQTT server
 * @param server      MQTT broker
 * @param topic       used MQTT token
 * @param authMethod  authentification method for the MQTT server
 * @param clientId    MQTT client ID
 */
PapaDuck::PapaDuck(String ssid, String password, String org, String deviceId, String deviceType, String token,
                   String server, String topic, String authMethod, String clientId)
    : m_ssid(ssid), m_password(password), m_org(org), m_deviceId(deviceId), m_deviceType(deviceType), m_token(token),
      m_server(server), m_topic(topic), m_authMethod(authMethod), m_clientId(clientId),
      m_pubSubClient(m_server.c_str(), 8883, m_wifiClient), m_ping(0xF4)
{
}

/**
 * @brief Destroy the Papa Duck:: Papa Duck object
 *
 */
PapaDuck::~PapaDuck() {}

/**
 * @brief Template for setting up a PapaDuck device.
 *
 */
void PapaDuck::setup()
{
    setupDisplay("Papa");
    setupLoRa();
    LoRa.receive();

    setupWiFi();

    Serial.println("PAPA Online");
}

/**
 * @brief Template for running core functionality of a PapaDuck.
 *
 */
void PapaDuck::run()
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.print("WiFi disconnected, reconnecting to local network: ");
        Serial.print(m_ssid);
        setupWiFi();
    }
    setupMQTT();

    int packetSize = LoRa.parsePacket();
    if (packetSize != 0) {
        byte whoIsIt = LoRa.peek();
        if (whoIsIt != m_ping) {
            Serial.println(packetSize);
            quackJson();
        }
    }

    m_timer.tick();
}

/**
 * @brief Setting up the wifi connection
 *
 */
void PapaDuck::setupWiFi()
{
    Serial.println();
    Serial.print("Connecting to ");
    Serial.print(m_ssid);

    // Connect to Access Poink
    WiFi.begin(m_ssid.c_str(), m_password.c_str());

    while (WiFi.status() != WL_CONNECTED) {
        m_timer.tick(); // Advance timer to reboot after awhile
        // delay(500);
        Serial.print(".");
    }

    // Connected to Access Point
    Serial.println("");
    Serial.println("WiFi connected - PAPA ONLINE");
}

/**
 * @brief connect to a MQTT broker
 *
 */
void PapaDuck::setupMQTT()
{
    if (!!!m_pubSubClient.connected()) {
        Serial.print("Reconnecting client to ");
        Serial.println(m_server);
        while (!(m_org == "quickstart"
                     ? m_pubSubClient.connect(m_clientId.c_str())
                     : m_pubSubClient.connect(m_clientId.c_str(), m_authMethod.c_str(), m_token.c_str()))) {
            m_timer.tick(); // Advance timer to reboot after awhile
            Serial.print("i");
            delay(500);
        }
    }
}

/**
 * @brief TODO
 *
 */
void PapaDuck::quackJson()
{
    const int bufferSize = 4 * JSON_OBJECT_SIZE(4);
    StaticJsonDocument<bufferSize> doc;

    JsonObject root = doc.as<JsonObject>();

    Packet lastPacket = getLastPacket();

    doc["DeviceID"]  = lastPacket.senderId;
    doc["MessageID"] = lastPacket.messageId;
    doc["Payload"].set(lastPacket.payload);
    doc["path"].set(lastPacket.path + "," + getDeviceId());

    String jsonstat;
    serializeJson(doc, jsonstat);

    if (m_pubSubClient.publish(m_topic.c_str(), jsonstat.c_str())) {

        serializeJsonPretty(doc, Serial);
        Serial.println("");
        Serial.println("Publish ok");

    } else {
        Serial.println("Publish failed");
    }
}