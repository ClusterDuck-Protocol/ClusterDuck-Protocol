# ClusterDuck Protocol examples

Example sketches for CDP 5.5. Each example is a folder with a sketch of the same name. Build any of them with PlatformIO from the root `platformio.ini`.

| Example | What it does |
|---|---|
| [Basic-Ducks/DuckLink](Basic-Ducks/DuckLink) | Leaf node: sends a text message every 10 s, doesn't relay |
| [Basic-Ducks/MamaDuck](Basic-Ducks/MamaDuck) | Mesh node: relays other ducks' messages and sends its own every 10 s |
| [Basic-Ducks/PapaDuck](Basic-Ducks/PapaDuck) | Gateway: publishes the messages it receives to an MQTT broker over WiFi |
| [Basic-Ducks/DetectorDuck](Basic-Ducks/DetectorDuck) | Signal-strength meter for placing ducks |
| [Custom-Mama-Examples/Full-MamaDuck](Custom-Mama-Examples/Full-MamaDuck) | MamaDuck with a status LED, a JSON reading and the optional 5.5 features |
| [Custom-Papa-Examples/AWS-PapaDuck](Custom-Papa-Examples/AWS-PapaDuck) | PapaDuck that publishes to AWS IoT Core |
| [Custom-Papa-Examples/DMS-Lite-WiFi-PapaDuck](Custom-Papa-Examples/DMS-Lite-WiFi-PapaDuck) | Gateway for DMS Lite Docker over local MQTT |
| [Custom-Papa-Examples/DMS-Lite-Serial-PapaDuck](Custom-Papa-Examples/DMS-Lite-Serial-PapaDuck) | Gateway for DMS Lite Docker over USB serial |
| [External-Boards-Example/DuckLink](External-Boards-Example/DuckLink) | DuckLink for a board that CDP doesn't define itself |

## Basic-Ducks

The smallest working sketch for each duck type. DuckLink, MamaDuck and PapaDuck use no LED; the DetectorDuck uses one to show signal strength. DuckLink, MamaDuck and PapaDuck share one layout: a header comment, the includes, the configuration constants, the duck and a timer, `setup()`, `loop()`, and the functions that send or publish.

- **DuckLink** sends `Hello from DUCKLINK C:<n>` on `topics::sensor` every `SEND_INTERVAL_MS`. A DuckLink doesn't relay messages from other ducks.
- **MamaDuck** sends the same message as `MAMADUCK` and relays the messages of other ducks. Relaying needs no code: `duck.run()` does it.
- **PapaDuck** joins your WiFi network (set `WIFI_SSID` and `WIFI_PASS`), connects to `test.mosquitto.org` over TLS and publishes every message it receives as JSON on `hub/event`:

  ```json
  {"from":"hub","to":"controller","RE":false,"eventType":"sensor","MessageID":"K3TQ","payload":{"hops":0,"duckType":2,"DeviceID":"MAMADUCK","Message":"Hello from MAMADUCK C:7"}}
  ```

  It also prints what arrives on `incoming/say_hello`. It checks the broker connection every 5 s and reconnects after a drop; after each failed attempt it waits twice as long, up to 60 s, because the PapaDuck doesn't receive LoRa packets while it connects. Pings and pongs aren't published. `web/` holds a small browser page that shows the published messages.
- **DetectorDuck** broadcasts a ping every 5 s. Every duck in direct range answers with a pong, and the detector logs the RSSI and SNR of each answer. On boards that define `CDPCFG_PIN_LED1` it shows the strongest answer on a WS2812 LED: green above -95 dBm, blue above -108 dBm, purple above -118 dBm, and red for a weaker signal or when two pings in a row get no answer. On boards without the pin, such as the Heltec V3, the result goes to serial only. A DetectorDuck relays nothing and sends no health messages.

## Custom-Mama-Examples

**Full-MamaDuck** is a MamaDuck with more than the basics:

- a JSON reading on `topics::sensor` every 30 s, with a counter, uptime, free heap and a UTC timestamp once the clock is valid
- a WS2812 status LED for setup, queued readings, send errors and received packets, with non-blocking flashes. It builds and runs without the LED on boards that have no `CDPCFG_PIN_LED1`.
- remote time sync, remote TX power and deep sleep, all off by default, with the build flags to turn them on

See [its README](Custom-Mama-Examples/Full-MamaDuck/README.md). A QuackPack sensor can add its readings at the hook in `sendReading()`; see [QuackPacks](#quackpacks).

## Custom-Papa-Examples

**AWS-PapaDuck** publishes every message it receives to AWS IoT Core, on `owl/device/<THINGNAME>/evt/<topic>`. While MQTT is down it keeps up to 5 messages in a queue and publishes them after it reconnects. Set the WiFi network in the sketch, and the thing name, endpoint and certificates in `secrets.h`. It needs the PubSubClient library (`libraries.txt`).

**DMS Lite PapaDucks** forward the flat JSON expected by DMS Lite Docker. The WiFi example publishes to the Docker host's local Mosquitto broker; the serial example writes newline-delimited JSON at 115200 baud. See [the setup and compatibility guide](Custom-Papa-Examples/README-DMS-Lite.md), including the CDP 5.5 serial-command limitation.

## External-Boards-Example

**DuckLink** shows how to build for a board that isn't in `src/include/boards`. With `-DCDP_EXTERNAL_BOARD`, CDP includes `cdp_external_board.h` from the sketch folder instead of its own board list, and that file picks `my-boards/<board>/board_config.h`. The folder has its own `platformio.ini` (envs `heltec_wifi_lora_paper` and `heltec_cubecell_gps`), which takes the CDP 5.5.0 release from GitHub, so build it as a separate PlatformIO project. CDP 5.5 supports ESP32 boards only, so only the Heltec Wireless Paper (ESP32-S3) configuration applies; the CubeCell one shows the file format.

## Building

### PlatformIO

From the CDP folder, set `EXAMPLE_DIR` to the example's folder under `examples/` and pick an environment. Without `EXAMPLE_DIR`, PlatformIO builds `Basic-Ducks/MamaDuck`.

```bash
EXAMPLE_DIR=Basic-Ducks/DuckLink pio run -e local_heltec_wifi_lora_32_V3              # build
EXAMPLE_DIR=Basic-Ducks/DuckLink pio run -e local_heltec_wifi_lora_32_V3 -t upload    # build and flash
EXAMPLE_DIR=Custom-Mama-Examples/Full-MamaDuck pio run -e local_lilygo_t_beam_sx1262
```

On Windows (PowerShell), set the variable first:

```powershell
$env:EXAMPLE_DIR="Basic-Ducks/DuckLink"
pio run -e local_heltec_wifi_lora_32_V3 -t upload
```

| Environment | Board | LED pin (`CDPCFG_PIN_LED1`) |
|---|---|---|
| `local_heltec_wifi_lora_32_V3` | Heltec WiFi LoRa 32 V3 | none |
| `local_heltec_wifi_lora_32_V2` | Heltec WiFi LoRa 32 V2 | 13 |
| `local_lilygo_t_beam_sx1262` | LilyGO T-Beam SX1262 | 4 |
| `local_lilygo_t_beam_sx1276` | TTGO LoRa32 V1 (SX1276) | 4 |
| `local_ttgo_lora32_v1_3` | TTGO LoRa32 V1.3 | 4 |

`local_` environments build against the CDP source in this folder; the matching `prod_` environments use the CDP 5.5.0 release from GitHub. To add a build flag, such as one of the opt-in settings below:

```bash
PLATFORMIO_BUILD_FLAGS="-DCDPCFG_REMOTE_TIME_SYNC=1" EXAMPLE_DIR=Custom-Mama-Examples/Full-MamaDuck pio run -e local_heltec_wifi_lora_32_V3
```

The Basic-Ducks, Full-MamaDuck and External-Boards sketches print with `Serial`. The CDP library's own log lines, and those of AWS-PapaDuck, use CDP's logger, which needs `-D CDP_INFO`; the PlatformIO configuration sets it.

CDP 5.5 needs C++17 or newer. `src/config/cdp_common.ini` builds with `-std=gnu++2a` and removes the default `-std=gnu++11` with `build_unflags`; do the same in your own PlatformIO project.

## Notes for writing a 5.5 sketch

- **No network join.** A duck can send as soon as `setupWithDefaults()` returns `DUCK_ERR_NONE`. Ducks in one mesh need the same radio settings; the default is 915 MHz.
- **`sendData()` queues.** It returns `DUCK_ERR_NONE` once the message is in the transmit queue, which isn't a delivery receipt. Otherwise it returns an error code: `DUCKPACKET_ERR_TOPIC_INVALID` for a reserved topic (below `0x10`), `DUCKPACKET_ERR_SIZE_INVALID` for an empty payload or one over 228 bytes (`MAX_DATA_LENGTH`), `DUCK_ERR_NOT_SUPPORTED` before setup or after `prepareForSleep()`, and `DUCK_ERR_QUEUE_FULL` when the queue (10 packets) is full. `getDuckErrorString(err)` turns a code into text. A duck transmits at most once every 3 s.
- **Some topics belong to CDP.** Topics below `0x10` are reserved for CDP's own packets, such as pings and route requests. Every duck except a DetectorDuck sends its own health message on `topics::health` every 15 minutes, MamaDucks and DuckLinks also send signal data on `topics::sig`, and `topics::cmd_tx` and `topics::cmd_time` carry commands (see the opt-in settings below). Send your data on `topics::sensor`, `topics::status` or another topic that isn't one of these.
- **The PapaDuck receives every copy.** Its `onReceiveDuckData()` callback gets each copy of a message that reaches it through a different relay, plus pings and pongs. Remove duplicates downstream by message ID, or call `muidNotReceived()` to drop them locally. The PapaDuck's own health also arrives in the callback, as a packet from itself with hop count 0, so publish it like the others. A MamaDuck or DuckLink callback also gets every packet it hears, except route requests and replies, which CDP handles itself. That includes copies of its own messages that neighbours relay back.
- **PapaDuck partitions.** WiFi, TLS and MQTT don't fit the default 1.25 MB app partition. The root `platformio.ini` sets `board_build.partitions = no_ota.csv`; do the same in your own PapaDuck project.
- **Opt-in settings, off by default.** `CDPCFG_REMOTE_TIME_SYNC=1` sets the clock from a PapaDuck's `topics::cmd_time`, `CDPCFG_REMOTE_TX_POWER=1` accepts TX power commands (`topics::cmd_tx`, `{"txPwr":20}`, 14 to 22 dBm), which take effect at the next boot, and `CDPCFG_REMOTE_COMMANDS=1` turns on both. The commands aren't authenticated. With them off, MamaDucks still relay them. A duck only stores a TX power its radio takes: 14 to 17 or 20 dBm on SX1276 boards (Heltec V2, TTGO LoRa32), all of 14 to 22 on SX1262 boards. If the stored value doesn't suit the radio at boot, the duck removes it and uses the default. `hasValidTime()` tells whether the clock holds a valid time. A PapaDuck shares a trusted time with `sendTimeSync(epoch)`; none of these examples sends one.
- **Deep sleep.** Keep calling `run()` until `txIdle()` is true (with a timeout), then call `prepareForSleep()` and `esp_deep_sleep_start()`. Waking up is a reboot. A sleeping MamaDuck doesn't relay.
- **Device IDs** are 8 characters and must be unique in the mesh.
- **Board pins.** Use `#ifdef CDPCFG_PIN_LED1` around LED code, because not every board defines it. On T-Beam boards, GPIO 16 and 17 connect to the PSRAM, so don't use them for sensors or LEDs. The T-Beam's OLED is optional; build with `-D ENABLE_DISPLAY` when one is fitted.

[Open-mesh routing, health and diagnostics](../docs/open-mesh-routing.md) describes the mesh, the health and signal messages, time sync and deep sleep in detail.

## QuackPacks

QuackPacks are libraries maintained by OWL Integrations that add a feature to a duck. Each one lives in its own repository with its own example sketch and build setup:

- [QuackPack-DuckGPS](https://github.com/Project-Owl/QuackPack-DuckGPS): GPS position and time
- [QuackPack-DuckSleep](https://github.com/Project-Owl/QuackPack-DuckSleep): scheduled deep sleep and a low-battery hold for MamaDucks
- [QuackPack-DuckDisplay](https://github.com/Project-Owl/QuackPack-DuckDisplay): OLED status display

A QuackPack has no CDP dependency of its own. Your project pins CDP and each QuackPack in `lib_deps`, as the QuackPack's README shows.
