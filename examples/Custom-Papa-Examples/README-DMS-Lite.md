# DMS Lite Docker PapaDucks

These two standalone sketches connect CDP 5.5 to
[DMS Lite Docker](https://github.com/Project-Owl/dms-lite-docker):

| Sketch | Connection to DMS Lite |
|---|---|
| [DMS-Lite-WiFi-PapaDuck](DMS-Lite-WiFi-PapaDuck) | WiFi and MQTT to the Docker host's Mosquitto broker |
| [DMS-Lite-Serial-PapaDuck](DMS-Lite-Serial-PapaDuck) | USB serial to the Docker host at 115200 baud |

Use one sketch per PapaDuck. Give every duck a unique eight-character
`DEVICE_ID`; the defaults are `PAPAMQTT` and `PAPASER1`. All ducks in the mesh
must use compatible CDP firmware and the same radio settings. These sketches use
the selected board's pins and CDP's default radio settings, including 915 MHz.

## MQTT / WiFi

1. In the WiFi sketch, set `WIFI_SSID`, `WIFI_PASS` and `MQTT_SERVER`. The server
   is the **LAN IP of the machine running Docker**, for example
   `IPAddress(192, 168, 1, 100)`. Container names such as `mosquitto`, container
   IPs and `localhost` are not the PapaDuck's broker address.
2. Set `MQTT_PORT` to the Docker host's exposed broker port, normally `1883`
   (`MQTT_HOST_PORT` in DMS Lite's `.env`). The bundled Mosquitto configuration
   accepts anonymous, unencrypted MQTT on the local network. This example uses
   that configuration; it has no public broker or cloud dependency.
3. Build and flash from this CDP checkout's root, selecting your board:

   ```sh
   EXAMPLE_DIR=Custom-Papa-Examples/DMS-Lite-WiFi-PapaDuck pio run -e local_heltec_wifi_lora_32_V3
   # Add -t upload when the correct PapaDuck is connected.
   ```

4. In the DMS Lite checkout, configure `.env` as its README describes, then run:

   ```sh
   docker compose -f docker-compose-base.yml -f docker-compose-wifi.yml up -d --build
   ```

The PapaDuck publishes each JSON message on the packet's CDP topic name, such as
`sensor`, `health`, `gps` or `signal health`. DMS Lite's default `MQTT_TOPIC='#'`
subscription accepts them all. A narrower subscription must match these names.
`MQTT_HOST=mosquitto` in Docker's `.env` is the **writer's internal broker
address**; keep that default when using the bundled broker.

WiFi starts without waiting for an access point and retries after a failed
initial connection. MQTT retries use delays from 5 to 60 seconds and two-second
TCP/MQTT socket timeouts. An MQTT connection attempt can still pause mesh
processing and lose LoRa packets. While disconnected, up to five messages wait
in RAM; the oldest is dropped when full. The queue drains one message per loop
after reconnecting. MQTT uses QoS 0 without retained messages: a successful
publish is not a database acknowledgement. Queued messages are lost on reboot.

## USB serial

1. Build and flash the serial sketch from the CDP checkout's root:

   ```sh
   EXAMPLE_DIR=Custom-Papa-Examples/DMS-Lite-Serial-PapaDuck pio run -e local_heltec_wifi_lora_32_V3
   # Add -t upload when the correct PapaDuck is connected.
   ```

2. Connect it by USB to a native Linux Docker host. Close other serial monitors
   so DMS Lite can own the port. In DMS Lite's `.env`, set
   `SERIAL_HOST_DEVICE` to the correct device and set `SERIAL_GID` as described
   in its README. Keep `SERIAL_BAUD=115200`.
3. Start the serial stack in the DMS Lite checkout:

   ```sh
   docker compose -f docker-compose-base.yml -f docker-compose-serial.yml up -d --build
   ```

The sketch prints each packet as a compact JSON object followed by a newline.
It leaves WiFi unused. CDP startup and diagnostic lines can also appear; the
modern serial adapter logs and skips those lines and continues with the next
JSON packet. Blank lines are ignored. A newline before each JSON message keeps
an unfinished diagnostic from becoming part of that message.

For Heltec V2 or T-Beam, use the corresponding `local_` environment from
[the examples guide](../README.md#platformio). Build against the CDP source in
this checkout while 5.5 is in testing; the `prod_` environments fetch a release
archive instead. The root configuration already selects the larger `no_ota.csv`
app partition. For Arduino IDE, open the matching `.ino` and install CDP 5.5 and
its dependencies; the WiFi sketch also uses PubSubClient. The board toolchain
must compile with C++17 or newer, as CDP 5.5 requires. The root PlatformIO
configuration sets the language standard for you.

## JSON format

Both transports emit the same flat object. These seven fields have the names
and types expected by both modern DMS Lite ingestion adapters:

```json
{"DeviceID":"MAMADUCK","MessageID":"4B335451","topic":"sensor","Payload":"23.5","path":"","hops":1,"duckType":2}
```

- `DeviceID` is the originating duck's eight-character ID. IDs containing
  spaces or nonprintable/non-ASCII bytes use full uppercase hex instead, with
  an additional `DeviceIDEncoding: "hex"` field.
- `MessageID` is the full four-byte MUID in uppercase hex. This avoids truncated
  IDs and collisions from converting binary IDs to printable log text.
- `topic` uses CDP 5.5's `topicToString()`, including its current sensor names.
- `Payload` stays a **string**, including when it contains a number or JSON.
  UTF-8 text, quotes, newlines and embedded NULs are preserved by JSON escaping.
  Non-UTF-8 binary payloads become `hex:<uppercase hex>` and include
  `PayloadEncoding: "hex"`. The prefix keeps binary data out of numeric charts.
  DMS Lite currently stores the string, but not the additional encoding fields.
- `path` is an empty string because CDP 5.5 no longer carries the legacy relay
  path in each packet. It remains present for DMS Lite schema compatibility.
- `hops` and `duckType` are JSON integers copied from the received packet.

Reserved routing/ping/pong packets are omitted. The PapaDuck's own periodic
health packet is included. Each relay copy is forwarded with the same
`MessageID`; these examples and DMS Lite's writers do not deduplicate copies.

## Serial command limitation

These sketches provide **data ingestion**. CDP 5.5 marks the old `dcmd` protocol
as deprecated and does not implement the legacy WiFi, health or channel
commands. The old command-enabled serial example's `sendCommand()` API is also
gone. The new serial sketch drains incoming bytes in bounded batches and logs
one warning instead of executing those commands.

## Historical reference

These replace the old examples from
[`569503f3`](https://github.com/Project-Owl/ClusterDuck-Protocol/tree/569503f39dc5244128819c14c4970d94a4d42006/examples/5.Custom-Papa-Examples).
The sketches were removed in
[`327db03a`](https://github.com/Project-Owl/ClusterDuck-Protocol/commit/327db03ae60c57605b7cbccce66020ea805381a7),
which reached master with the v4.2 release. The new versions use the CDP 5.5
packet callback, board configuration and ArduinoJson 7 APIs.
