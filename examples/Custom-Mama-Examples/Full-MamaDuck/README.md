# Full-MamaDuck

A MamaDuck for CDP 5.5 that shows more than [Basic-Ducks/MamaDuck](../../Basic-Ducks/MamaDuck). It relays the messages of other ducks like every MamaDuck, and also:

- **Sends a JSON reading** on `topics::sensor` every `SEND_INTERVAL_MS` (30 s): a counter, the uptime, the free heap and, once the clock holds a valid time, a UTC timestamp. For example `{"count":12,"uptimeS":360,"freeHeap":291340,"utc":1767225600}`.
- **Shows what it does on a WS2812 RGB LED,** on boards that define `CDPCFG_PIN_LED1`: cyan while starting, gold when running, steady red when setup failed, and short flashes for a queued reading (green), a reading that wasn't queued (red) and a packet heard (blue). The flashes don't use `delay()`. Boards without the pin, such as the Heltec WiFi LoRa 32 V3, build and run the same without the LED.
- **Logs every result** to serial at 115200 baud, with the error code when `sendData()` fails.

The device ID, send interval and LED brightness are at the top of the sketch.

## Build

From the CDP folder with PlatformIO:

```bash
EXAMPLE_DIR=Custom-Mama-Examples/Full-MamaDuck pio run -e local_heltec_wifi_lora_32_V3 -t upload
```

## Optional features (all off)

| Feature | How to turn it on | Notes |
|---|---|---|
| Remote time sync | `-DCDPCFG_REMOTE_TIME_SYNC=1` | Sets the clock from `topics::cmd_time`. Readings include `utc` once a PapaDuck has sent the time with `sendTimeSync()`; the CDP example PapaDucks don't send it. |
| Remote TX power | `-DCDPCFG_REMOTE_TX_POWER=1` | Accepts `topics::cmd_tx` commands such as `{"txPwr":20}`, 14 to 22 dBm. The duck stores the value and uses it from its next boot. It only stores a value its radio takes: 14 to 17 or 20 dBm on SX1276 boards (Heltec V2, TTGO LoRa32), all of 14 to 22 on SX1262 boards. |
| Both commands | `-DCDPCFG_REMOTE_COMMANDS=1` | |
| Deep sleep | change `#define ENABLE_DEEP_SLEEP 0` to `1` in the sketch, or build with `-DENABLE_DEEP_SLEEP=1` | Awake 60 s, asleep 300 s. See the warning below. |

The remote commands are CDP build flags. They aren't authenticated: anything in radio range can send one. With them off, the duck still relays the commands for other ducks. To build with one:

```bash
PLATFORMIO_BUILD_FLAGS="-DCDPCFG_REMOTE_TIME_SYNC=1" \
  EXAMPLE_DIR=Custom-Mama-Examples/Full-MamaDuck pio run -e local_heltec_wifi_lora_32_V3
```

In your own PlatformIO project, add the flag to `build_flags` instead.

**Deep sleep warning:** a sleeping MamaDuck receives and relays nothing, so ducks that depend on it lose their path to the PapaDuck. Use it only for a duck that no other duck needs as a relay. Before sleeping, the sketch keeps calling `run()` until `txIdle()` is true (at most 10 s), then calls `prepareForSleep()` and `esp_deep_sleep_start()`. Waking up is a reboot, so the counter and uptime start again, and a duck that is never awake for 15 minutes sends no CDP health message.

## QuackPacks

A QuackPack sensor can add its readings at the `QuackPack sensor hook` comment in `sendReading()`. Each QuackPack has its own example; see [QuackPacks](../../README.md#quackpacks).
