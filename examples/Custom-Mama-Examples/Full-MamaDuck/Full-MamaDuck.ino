/**
 * @file Full-MamaDuck.ino
 * @brief A full-featured MamaDuck for the ClusterDuck Protocol (CDP) 5.5.
 *
 * Like every MamaDuck, it relays the messages it hears from other ducks. On
 * top of Basic-Ducks/MamaDuck it:
 * - sends a JSON reading on topics::sensor every SEND_INTERVAL_MS, such as
 *   {"count":12,"uptimeS":360,"freeHeap":291340,"utc":1767225600}
 *   "utc" (UTC seconds since 1970) is only included once the clock holds a
 *   valid time; see hasValidTime() and option 1 below.
 * - shows what it does on a WS2812 RGB LED, on boards that define
 *   CDPCFG_PIN_LED1:
 *     cyan          starting
 *     gold          running
 *     red           setup failed
 *     green flash   reading queued
 *     red flash     reading not queued (the serial log has the error)
 *     blue flash    packet heard (see handleDuckData())
 *   On boards without CDPCFG_PIN_LED1, such as the Heltec WiFi LoRa 32 V3, it
 *   builds and runs the same, without the LED.
 * - logs every result to serial, at 115200 baud.
 *
 * sendData() returns DUCK_ERR_NONE once a reading is queued for the radio,
 * which is not a delivery receipt. CDP also sends the duck's own health on
 * topics::health every 15 minutes, so readings use another topic.
 *
 * Optional features, all off by default:
 *
 * 1. Remote time sync and TX power. These are CDP build flags, not code in
 *    this sketch:
 *      -DCDPCFG_REMOTE_TIME_SYNC=1  set the clock from topics::cmd_time. Readings
 *                                   get "utc" once a PapaDuck has sent the time
 *                                   with sendTimeSync(); the CDP example
 *                                   PapaDucks don't send it.
 *      -DCDPCFG_REMOTE_TX_POWER=1   accept topics::cmd_tx TX power commands, such
 *                                   as {"txPwr":20} (14 to 22 dBm). The duck
 *                                   stores the value and uses it from its next
 *                                   boot. It only stores a value its radio
 *                                   takes: SX1276 boards (Heltec V2, TTGO
 *                                   LoRa32) take 14 to 17 or 20 dBm.
 *      -DCDPCFG_REMOTE_COMMANDS=1   both
 *    Neither command is authenticated: anything in radio range can send one.
 *    With them off, the duck still relays them for other ducks. To build with
 *    PlatformIO from the CDP folder:
 *      PLATFORMIO_BUILD_FLAGS="-DCDPCFG_REMOTE_TIME_SYNC=1" \
 *        EXAMPLE_DIR=Custom-Mama-Examples/Full-MamaDuck \
 *        pio run -e local_heltec_wifi_lora_32_V3
 *    or add the flag to build_flags in your own platformio.ini.
 *
 * 2. Deep sleep: change `#define ENABLE_DEEP_SLEEP 0` below to 1, or build with
 *    -DENABLE_DEEP_SLEEP=1. WARNING: a sleeping MamaDuck receives and relays
 *    nothing, so ducks that depend on it lose their path to the PapaDuck. Use
 *    it only for a duck that no other duck needs as a relay.
 */

#include <string>
#include <time.h>
#include <esp_timer.h>
#include <CDP.h>

#ifdef CDPCFG_PIN_LED1
#include <FastLED.h>
#endif

// --- Configuration ---
const char DEVICE_ID[] = "MAMA0001";            // exactly 8 characters, unique in the mesh
const unsigned long SEND_INTERVAL_MS = 30000;    // time between two readings
const uint8_t LED_BRIGHTNESS = 64;               // 0 to 255
const unsigned long LED_FLASH_MS = 150;          // length of an event flash

// LED colours, as 0xRRGGBB
const uint32_t COLOR_OFF = 0x000000;
const uint32_t COLOR_STARTING = 0x00FFFF;   // cyan
const uint32_t COLOR_READY = 0xFFD700;      // gold
const uint32_t COLOR_FAILED = 0xFF0000;     // red
const uint32_t COLOR_QUEUED = 0x00FF00;     // green
const uint32_t COLOR_SEND_ERROR = 0xFF0000; // red
const uint32_t COLOR_RECEIVED = 0x0000FF;   // blue

// Optional deep sleep (see the top of this file). Off by default.
#ifndef ENABLE_DEEP_SLEEP
#define ENABLE_DEEP_SLEEP 0
#endif

#if ENABLE_DEEP_SLEEP
#include <esp_sleep.h>
const unsigned long AWAKE_TIME_MS = 60000;    // time awake after each boot or wake-up
const unsigned long IDLE_TIMEOUT_MS = 10000;  // longest wait for queued packets to go out
const uint64_t SLEEP_TIME_S = 300;            // time asleep
static_assert(AWAKE_TIME_MS > SEND_INTERVAL_MS, "stay awake long enough to send a reading");
#endif

// --- Globals ---
MamaDuck duck(DEVICE_ID);
auto timer = timer_create_default();
uint32_t counter = 1;   // number of the next reading
bool setupOK = false;

#ifdef CDPCFG_PIN_LED1
CRGB led[1];
#endif
uint32_t ledColor = COLOR_OFF;   // steady colour, shown between flashes
unsigned long flashStart = 0;    // millis() when the current flash started
bool flashing = false;

bool sendReading(void*);
void handleDuckData(CdpPacket packet);

// --- LED: steady colours and short flashes, without delay() ---

/**
 * @brief Write a colour to the LED. Does nothing on boards without one.
 */
void showColor(uint32_t color) {
#ifdef CDPCFG_PIN_LED1
  led[0] = CRGB(color);
  FastLED.show();
#else
  (void)color;
#endif
}

/**
 * @brief Set the steady colour, shown whenever no flash is running.
 */
void setLed(uint32_t color) {
  ledColor = color;
  if (!flashing) {
    showColor(color);
  }
}

/**
 * @brief Flash a colour for LED_FLASH_MS. updateLed() ends the flash.
 */
void flashLed(uint32_t color) {
  flashing = true;
  flashStart = millis();
  showColor(color);
}

/**
 * @brief End the current flash once LED_FLASH_MS has passed. Called from loop().
 */
void updateLed() {
  if (flashing && millis() - flashStart >= LED_FLASH_MS) {
    flashing = false;
    showColor(ledColor);
  }
}

void setup() {
#ifdef CDPCFG_PIN_LED1
  FastLED.addLeds<WS2812, CDPCFG_PIN_LED1, GRB>(led, 1).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(LED_BRIGHTNESS);
#endif
  setLed(COLOR_STARTING);

  // Starts the serial port at 115200 baud and the LoRa radio. There is no
  // network to join: the duck can send and relay as soon as this returns.
  int err = duck.setupWithDefaults();
  if (err != DUCK_ERR_NONE) {
    Serial.printf("[MAMA] Setup failed, rc = %d (%s)\n", err, getDuckErrorString(err));
    setLed(COLOR_FAILED);
    return;
  }

  duck.onReceiveDuckData(handleDuckData);
  timer.every(SEND_INTERVAL_MS, sendReading);
  setupOK = true;
  setLed(COLOR_READY);

  Serial.printf("[MAMA] Setup OK. Remote time sync: %s, remote TX power: %s, deep sleep: %s\n",
    CDPCFG_REMOTE_TIME_SYNC ? "on" : "off",
    CDPCFG_REMOTE_TX_POWER ? "on" : "off",
    ENABLE_DEEP_SLEEP ? "on" : "off");
}

#if ENABLE_DEEP_SLEEP
void sleepWhenDone();
#endif

void loop() {
  if (!setupOK) {
    return;
  }
  timer.tick();
  duck.run();     // relays received packets, sends queued ones, calls handleDuckData()
  updateLed();
#if ENABLE_DEEP_SLEEP
  sleepWhenDone();
#endif
}

/**
 * @brief Called by duck.run() for every packet the duck hears, before it is
 * relayed, except route requests and replies (CDP handles those itself) and
 * packets over the hop limit. That includes pings, pongs and commands. A
 * message that several ducks relay arrives once per copy, so this also gets
 * copies of this duck's own messages that neighbours relay back.
 */
void handleDuckData(CdpPacket packet) {
  Serial.printf("[MAMA] Heard %s from %s, hop count %u\n", packet.topicToString().c_str(),
    duckutils::toString(packet.sduid).c_str(), unsigned(packet.hopCount));
  flashLed(COLOR_RECEIVED);
}

/**
 * @brief Build a JSON reading and queue it on topics::sensor. Called by the timer.
 * @returns true to keep the timer running
 */
bool sendReading(void*) {
  JsonDocument doc;
  doc["count"] = counter;
  // esp_timer_get_time() counts microseconds in 64 bits; millis() wraps
  // after about 49.7 days.
  doc["uptimeS"] = static_cast<uint32_t>(esp_timer_get_time() / 1000000ULL);
  doc["freeHeap"] = ESP.getFreeHeap();
  if (duck.hasValidTime()) {
    doc["utc"] = static_cast<uint32_t>(time(nullptr));
  }

  // --- QuackPack sensor hook ---
  // A QuackPack sensor will add its readings to doc here, for example
  // doc["temp"] = 21.5. Each QuackPack has its own example; see the
  // QuackPacks section of examples/README.md.
  // Keep the whole payload within MAX_DATA_LENGTH (228 bytes): sendData()
  // rejects a larger one with DUCKPACKET_ERR_SIZE_INVALID.

  std::string payload;
  serializeJson(doc, payload);

  int err = duck.sendData(topics::sensor, payload);
  if (err == DUCK_ERR_NONE) {
    Serial.printf("[MAMA] Queued %u bytes: %s\n", unsigned(payload.size()), payload.c_str());
    counter++;
    flashLed(COLOR_QUEUED);
  } else {
    Serial.printf("[MAMA] sendData failed for %u bytes, rc = %d (%s)\n",
      unsigned(payload.size()), err, getDuckErrorString(err));
    flashLed(COLOR_SEND_ERROR);
  }
  return true;
}

#if ENABLE_DEEP_SLEEP
// millis() at which the duck tries to sleep. Waking from deep sleep is a
// reboot, so millis() and every variable in this sketch, the counter
// included, start again. CDP's own health message is sent every 15 minutes
// of awake time, so a duck that is never awake that long sends none.
unsigned long sleepAt = AWAKE_TIME_MS;

/**
 * @brief Go to deep sleep once the duck has been awake AWAKE_TIME_MS and has
 * sent what it queued. Called from loop(), so duck.run() keeps running while
 * it waits. The order matters:
 * 1. Wait until duck.txIdle() is true, or queued packets are lost. Give up
 *    after IDLE_TIMEOUT_MS: a relay on a busy mesh may never be idle.
 * 2. duck.prepareForSleep() puts the LoRa radio to sleep and stops the duck.
 * 3. esp_deep_sleep_start(). Waking up is a reboot: setup() runs again.
 */
void sleepWhenDone() {
  const unsigned long now = millis();
  if (now < sleepAt) {
    return;
  }
  const bool idle = duck.txIdle();
  if (!idle && now - sleepAt < IDLE_TIMEOUT_MS) {
    return;   // keep calling duck.run() until the queues are empty
  }
  if (!idle) {
    Serial.println("[MAMA] Still busy after IDLE_TIMEOUT_MS, dropping what is queued");
  }

  int err = duck.prepareForSleep();
  if (err != DUCK_ERR_NONE) {
    Serial.printf("[MAMA] prepareForSleep failed, rc = %d (%s), staying awake\n",
      err, getDuckErrorString(err));
    sleepAt = now + AWAKE_TIME_MS;   // the duck keeps running; try again later
    return;
  }

  Serial.printf("[MAMA] Sleeping for %lu s\n", (unsigned long)SLEEP_TIME_S);
  Serial.flush();
  showColor(COLOR_OFF);
  esp_sleep_enable_timer_wakeup(SLEEP_TIME_S * 1000000ULL);
  esp_deep_sleep_start();
}
#endif
