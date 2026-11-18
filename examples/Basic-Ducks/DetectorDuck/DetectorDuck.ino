/**
 * @file DetectorDuck.ino
 * @brief A DetectorDuck: a signal-strength meter for placing ducks.
 *
 * Every PING_INTERVAL_MS the detector broadcasts a ping, and every duck in
 * direct range (MamaDuck, DuckLink or PapaDuck) answers with a pong. Pings and
 * pongs are never relayed, so only direct neighbours answer. For each pong,
 * CDP calls handlePong() with the signal measured on it as JSON, for example
 * {"rssi":-87.5,"snr":9.25}: RSSI in dBm and SNR in dB, both with decimals.
 *
 * Each answer is logged to serial. The strongest answer to the latest ping is
 * shown on a WS2812 RGB LED, on boards that define CDPCFG_PIN_LED1:
 *   green   RSSI above -95 dBm    good signal
 *   blue    above -108 dBm        okay signal
 *   purple  above -118 dBm        low signal
 *   red     weaker, or no answer to NO_SIGNAL_PINGS pings in a row
 * On boards without CDPCFG_PIN_LED1, such as the Heltec WiFi LoRa 32 V3, the
 * result goes to serial only.
 *
 * Apart from its pings, a DetectorDuck sends nothing: no health messages, and
 * it relays nothing.
 */

#include <CDP.h>

#ifdef CDPCFG_PIN_LED1
#include <FastLED.h>
#endif

// --- Configuration ---
const char DEVICE_ID[] = "DETECTOR";            // exactly 8 characters, unique in the mesh
const unsigned long PING_INTERVAL_MS = 5000;     // time between two pings
// Pings in a row with no answer before the detector reports no signal. More
// than one, because a pong can wait up to 3 s or more in the answering duck's
// transmit queue (CDP sends at most one packet every 3 s).
const uint32_t NO_SIGNAL_PINGS = 2;
const uint8_t LED_BRIGHTNESS = 128;              // 0 to 255

// Signal levels in dBm, and their LED colours as 0xRRGGBB
const float RSSI_GOOD = -95.0f;
const float RSSI_OKAY = -108.0f;
const float RSSI_LOW = -118.0f;
const uint32_t COLOR_READY = 0xFFD700;  // gold: set up, no answer yet
const uint32_t COLOR_GOOD = 0x008000;   // green
const uint32_t COLOR_OKAY = 0x0000FF;   // blue
const uint32_t COLOR_LOW = 0x800080;    // purple
const uint32_t COLOR_NONE = 0xFF0000;   // red: no signal, or setup failed

// --- Globals ---
DetectorDuck duck(DEVICE_ID);
auto timer = timer_create_default();
uint32_t pingsSinceAnswer = 0;     // pings sent since the latest answer
bool answered = false;             // whether any duck answered the latest ping
float bestRssi = 0.0f;             // strongest RSSI heard since the latest ping
bool setupOK = false;

#ifdef CDPCFG_PIN_LED1
CRGB led[1];
#endif

bool pingNeighbours(void*);
void handlePong(CdpPacket packet);

/**
 * @brief Show a colour on the LED. Does nothing on boards without one.
 */
void setLed(uint32_t color) {
#ifdef CDPCFG_PIN_LED1
  led[0] = CRGB(color);
  FastLED.show();
#else
  (void)color;
#endif
}

void setup() {
#ifdef CDPCFG_PIN_LED1
  FastLED.addLeds<WS2812, CDPCFG_PIN_LED1, GRB>(led, 1).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(LED_BRIGHTNESS);
#endif

  // Starts the serial port at 115200 baud and the LoRa radio.
  int err = duck.setupWithDefaults();
  if (err != DUCK_ERR_NONE) {
    Serial.printf("[DETECTOR] Setup failed, rc = %d (%s)\n", err, getDuckErrorString(err));
    setLed(COLOR_NONE);
    return;
  }

  duck.onReceiveDuckData(handlePong);
  timer.every(PING_INTERVAL_MS, pingNeighbours);
  setupOK = true;
  setLed(COLOR_READY);
  Serial.println("[DETECTOR] Setup OK");
}

void loop() {
  if (!setupOK) {
    return;
  }
  timer.tick();
  duck.run();   // sends the pings and calls handlePong() for each answer
}

/**
 * @brief Report no signal if the last NO_SIGNAL_PINGS pings got no answer, then
 * broadcast a ping to the direct neighbours. Called by the timer.
 * @returns true to keep the timer running
 */
bool pingNeighbours(void*) {
  if (pingsSinceAnswer >= NO_SIGNAL_PINGS) {
    Serial.printf("[DETECTOR] No signal: no duck answered the last %u pings\n",
      unsigned(pingsSinceAnswer));
    setLed(COLOR_NONE);
  }

  answered = false;
  int err = duck.sendPing();
  if (err == DUCK_ERR_NONE) {
    pingsSinceAnswer++;
  } else {
    Serial.printf("[DETECTOR] sendPing failed, rc = %d (%s)\n", err, getDuckErrorString(err));
  }
  return true;
}

const char* signalLabel(float rssi) {
  if (rssi > RSSI_GOOD) return "GOOD";
  if (rssi > RSSI_OKAY) return "OKAY";
  if (rssi > RSSI_LOW) return "LOW";
  return "NO SIGNAL";
}

uint32_t signalColor(float rssi) {
  if (rssi > RSSI_GOOD) return COLOR_GOOD;
  if (rssi > RSSI_OKAY) return COLOR_OKAY;
  if (rssi > RSSI_LOW) return COLOR_LOW;
  return COLOR_NONE;
}

/**
 * @brief Called by duck.run() for each pong, with {"rssi":<dBm>,"snr":<dB>}.
 */
void handlePong(CdpPacket packet) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc,
    reinterpret_cast<const char*>(packet.data.data()), packet.data.size());
  // RSSI and SNR are floats: read them with as<float>(). A default such as
  // doc["rssi"] | -200 only works for integers and would turn -87.5 into -200.
  if (error || !doc["rssi"].is<float>()) {
    Serial.println("[DETECTOR] Answer without signal data");
    return;
  }
  float rssi = doc["rssi"].as<float>();
  float snr = doc["snr"].as<float>();
  pingsSinceAnswer = 0;

  Serial.printf("[DETECTOR] %s answered: RSSI %.1f dBm, SNR %.1f dB - %s\n",
    duckutils::toString(packet.sduid).c_str(), rssi, snr, signalLabel(rssi));

  if (!answered || rssi > bestRssi) {
    answered = true;
    bestRssi = rssi;
    setLed(signalColor(rssi));
  }
}
