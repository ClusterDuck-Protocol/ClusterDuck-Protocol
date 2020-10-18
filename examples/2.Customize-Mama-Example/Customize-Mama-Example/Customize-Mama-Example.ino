/**
 * @file Customize-Mama-Example.ino
 * @brief Uses the built in Mama Duck with some customatizations.
 * 
 * This example is a Mama Duck, but it is also periodically sending a message in the Mesh
 * It is setup to provide a custom Emergency portal, instead of using the one provided by the SDK.
 * Notice the background color of the captive portal is Black instead of the default Red.
 * 
 */
#include <string>
#include <arduino-timer.h>
#include <MamaDuck.h>
#include <DuckDisplay.h>


auto timer = timer_create_default();
const int INTERVAL_MS = 30000;
int counter = 1;

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

const char* DUCK_WIFI_AP = "MAMA DUCK PORTAL";

// Use pre-built papa duck
MamaDuck duck = MamaDuck();
DuckDisplay* display = NULL;
// LORA RF CONFIG
#define LORA_FREQ 915.0 // Frequency Range. Set for US Region 915.0Mhz
#define LORA_TXPOWER 20 // Transmit Power
// LORA HELTEC PIN CONFIG
#define LORA_CS_PIN 18
#define LORA_DIO0_PIN 26
#define LORA_DIO1_PIN -1 // unused
#define LORA_RST_PIN 14
void setup() {

  // We are using a hardcoded device id here, but it should be retrieved or given during the device provisioning
  // then converted to a byte vector to setup the duck
  // NOTE: The Device ID must be exactly 8 bytes otherwise it will get rejected
  std::string deviceId("MAMA0001");
  std::vector<byte> devId;
  devId.insert(devId.end(), deviceId.begin(), deviceId.end());

  // Assign unique device id
  duck.setDeviceId(devId);

  // initialize the serial component with the hardware supported baudrate
  duck.setupSerial(115200);
  // initialize the LoRa radio with specific settings. This will overwrites settings defined in the CDP config file cdpcfg.h
  duck.setupRadio(LORA_FREQ, LORA_CS_PIN, LORA_RST_PIN, LORA_DIO0_PIN, LORA_DIO1_PIN, LORA_TXPOWER);
  // initialize the wifi access point with a custom AP name
  duck.setupWifi(DUCK_WIFI_AP);
  // initialize DNS
  duck.setupDns();
  // initialize web server, enabling the captive portal with a custom HTML page
  //  duck.setupWebServer(true, HTML);
  // initialize Over The Air firmware upgrade
  duck.setupOTA();
  // This duck has an OLED display and we want to use it.
  // Get an instance and initialize it, so we can use in our application
  display = DuckDisplay::getInstance();
  display->setupDisplay();
  // we are done
  display->drawString(true, 20, 20, "DUCK READY");
  Serial.println("MAMA-DUCK...READY!");

  timer.every(INTERVAL_MS, runSensor);
}

void loop() {
  timer.tick();
  // Use the default run(). The Mama duck is designed to also forward data it receives
  // from other ducks, across the network. It has a basic routing mechanism built-in
  // to prevent messages from hoping endlessly.
  duck.run();
}

bool runSensor(void *) {
  bool result;
  const byte* buffer;

  String message = String("mama0001:") + String(counter);
  int length = message.length();
  Serial.print("[MAMA] sensor data: ");
  Serial.println(message);
  buffer = (byte*) message.c_str();

  result = sendData(buffer, length);
  if (result) {
    Serial.println("[MAMA] runSensor ok.");
  } else {
    Serial.println("[MAMA] runSensor failed.");
  }
  return result;
}

bool sendData(const byte* buffer, int length) {
  bool sentOk = false;

  // Send Data can either take a byte buffer (unsigned char) or a vector
  int err = duck.sendData(topics::status, buffer, length);
  if (err == DUCK_ERR_NONE) {
    counter++;
    sentOk = true;
  }
  if (!sentOk) {
    Serial.println("[MAMA] Failed to send data. error = " + String(err));
  }
  return sentOk;
}
