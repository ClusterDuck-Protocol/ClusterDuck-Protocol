# Open-mesh routing, health and diagnostics

How CDP 5.5 ducks find each other, deliver data, report their own health, share the time and prepare for deep sleep. The packet header is the same 27 bytes as in 5.0.

## How a duck joins and sends

- **No join handshake.** A duck is ready to send and receive as soon as its radio starts. Ducks still need compatible radio settings: frequency, bandwidth, spreading factor and sync word.
- **Sending:** `sendData()` returns success once the packet is accepted into the bounded TX queue. That isn't a delivery receipt. A full queue returns `DUCK_ERR_QUEUE_FULL`. There's no persistent store, so a reboot or sleep discards anything still queued (see [Deep sleep](#deep-sleep)).
- **Relaying:** MamaDucks flood application packets to every neighbour, whether or not they know a route to the destination. The duplicate filter stops repeats, and a packet is relayed at most `MAX_HOPS` (6) times.
- **Pacing:** route-control and data queues take turns when both have work, with `CDPCFG_MAX_PACKET_SEND_RATE` (3 s) between transmissions. If the radio is busy, the packet stays queued.
- **Leaves and observers:** DuckLinks never advertise a transit route. DetectorDucks only listen.
- **PapaDuck delivery:** the PapaDuck's receive callback gets every packet it hears, including repeated copies of the same message arriving through different relays. Remove duplicates downstream (for example in the cloud), or call `muidNotReceived()` to filter locally.

## Neighbour discovery and routes

Routes are used for diagnostics, signal health and targeted discovery. Data delivery doesn't depend on them.

- **At startup** every non-Papa duck sends one neighbour discovery after a random 2–6 seconds.
- **Every `CDPCFG_ROUTE_KEEPALIVE_MS`** (10 min) it broadcasts a one-hop discovery, so neighbours refresh each other.
- **If there's no route to the PapaDuck,** a send or the keepalive can start a targeted discovery, at most once every 30 seconds per duck.
- **Discovery reasons** reported in diagnostics: startup (1), missing destination (2), neighbour keepalive (3), missing gateway at keepalive (4).
- **Passive refresh:** any packet heard with hop count 0 comes straight from its sender, so it refreshes that neighbour's entry at no airtime cost.
- **Expiry:** an entry expires `CDPCFG_ROUTE_TTL_MS` (30 min) after it was last refreshed. Keep the TTL well above the keepalive interval.
- **Bounded tables:** at most `CDPCFG_MAX_DIRECT_NEIGHBORS` (32) direct peers and `CDPCFG_MAX_ROUTE_DESTINATIONS` (64) destinations, with up to three next-hop candidates per destination. A route learned indirectly never replaces a link the duck measured itself.

### Route request and response format

RREQ and RREP payloads are JSON. Device IDs are written as 8-character text. The two special addresses can't be written as text, so they use aliases:

| Address | Packet header | Route JSON |
|---|---|---|
| Broadcast | `FF FF FF FF FF FF FF FF` | `"*"` |
| PapaDuck | `00 00 00 00 00 00 00 00` | `"11111111"` |

- **RREQ:** the header destination and the JSON `destination` both name the final target. `origin`, MUID and hop count survive forwarding. `path` starts with the origin and gains one entry per relay. A duck already in the path doesn't forward the request, and broadcast neighbour requests aren't relayed at all.
- **RREP:** the actual responder sends a new MUID. Its JSON `origin` is the discovered destination, `destination` is the requester, and `path` is the request's path reversed. `q` is the original request's MUID as 8 hex digits, and `via` is the duck that transmitted this copy. The header destination is the next duck on the path, not the final requester.
- **Forwarding a reply:** only the duck an RREP is addressed to forwards it. It removes itself from the path, updates `via` and increments the hop count. Overhearing a reply never causes a duck to send one. Together with the path and hop limits and a 64-entry cache of recent control packets, this prevents reply loops.

## Compatibility with 5.0 ducks

- **Header addresses are unchanged.** 5.5 ducks address the PapaDuck as `00000000` in the packet header, so 5.0 MamaDucks relay their data.
- **5.0 data reaches 5.5 PapaDucks,** and 5.5 MamaDucks relay 5.0 data, because they flood everything.
- **Phoenix 2026 firmware** addressed the PapaDuck as `11111111` in the header. 5.5 ducks treat that as the PapaDuck too.
- **Route control is only partly compatible.** 5.5 ducks accept 5.0 route requests and direct replies. 5.0 ducks can still generate extra route-reply traffic in a mixed mesh. That costs airtime but doesn't affect data delivery, which doesn't use routes. Upgrade all ducks in a mesh when you can.

## Signal health (`topics::sig`, schema v2)

Each non-Papa, non-Detector duck reports its direct neighbours every `CDPCFG_SIGNAL_INTERVAL_MS` (30 min). It also sends one snapshot with its first health heartbeat after boot, so a new installation shows up quickly.

```json
{"s":"RPNODE01","n":[["PEER0001",0.9512,0.75]],"o":1,"i":1,"tr":false,"a":42,"v":2}
```

| Field | Meaning |
|---|---|
| `s` | Sender DUID |
| `n` | `[duid, rssi, snr]` per directly heard neighbour, best signal first. RSSI is normalised over −131..−20 dBm and SNR over −30..30 dB to 0..1, rounded to 4 decimals |
| `o` | Fresh direct neighbours known |
| `i` | Neighbours included in this message |
| `tr` | `true` if neighbours were left out to fit |
| `a` | Age in seconds of the oldest included observation, `null` if none |
| `v` | Schema version, 2 |

At most `CDPCFG_SIGNAL_MAX_NEIGHBORS` (6) entries are included, and the whole payload stays within 228 bytes. A duck with no neighbours sends an empty array. RSSI and SNR are captured when each packet is received, not sampled later.

## Radio health (`topics::health`, schema v4)

Every `CDPCFG_HEALTH_INTERVAL_MS` (15 min) a duck sends one flat status message to the PapaDuck. DetectorDucks send none. The PapaDuck doesn't transmit its own: it passes it to its `onReceiveDuckData()` callback as a packet from itself to `PAPADUCK_DUID` with hop count 0, so it reaches the cloud with the health of the other ducks. Without a callback, the PapaDuck's health isn't sent anywhere.

```json
{"c":"12","fm":"291340","boot":2488545892,"uptimeS":3604,"neighbors":3,"txStarted":79,"txDone":79,"txFailed":0,"txTimeouts":0,"queueDrops":0,"v":4}
```

| Field | Meaning |
|---|---|
| `c` / `fm` | Health message counter and free heap in bytes (strings, as in earlier versions) |
| `boot` / `uptimeS` | Random ID for this boot, and seconds since boot. Compare counters only within one boot; uptime wraps after about 49.7 days |
| `neighbors` | Fresh direct neighbours |
| `txStarted` / `txDone` | Radio transmissions started and completed, including routing traffic. Completion isn't a receipt |
| `txFailed` / `txTimeouts` | Failed radio starts and transmit timeouts |
| `queueDrops` | Packets rejected by the RX, data-TX and control-TX queues combined |
| `v` | Schema version, 4 |

Counters reset at boot and wrap at 2^32. Consumers that decoded older health formats need to recognise `v`.

## Serial diagnostics

With each health heartbeat, one detail page is logged to serial as `[CDP_DIAG] health_detail={...}`. Four heartbeats cycle through all pages. Pages aren't sent over the radio; call `routingDiagnostics(page)` to get one in your own code.

Every page has `v`, page number `p`, boot ID `b` and uptime `u`.

| Page | Contents |
|---|---|
| 0 | `C` / `FM` counter and free heap, reset reason `r`, state `n` (0 searching, 1 ready, 2 disconnected), gateway next hop `gw` and its route age `ga` in seconds (`null` without a route; a PapaDuck reports `self` / 0), direct neighbours `nb`, discovery attempts `j`, last discovery reason `jr`, state transitions `nt` |
| 1 | Queue depth `q`, high-water mark `h` and rejected packets `d`, each ordered RX / data-TX / control-TX |
| 2 | RREQ `rq` and RREP `rp` counts, ordered originated / forwarded / duplicate suppressed / dropped. `dr` is malformed control packets / hop-limit drops / rejected loops |
| 3 | `tx`: radio starts / transmissions done / failed starts / busy deferrals / timeouts. `app`: data sent / relayed / duplicates suppressed. `sg`: last signal snapshot (observed / included / seconds ago). `ne`: direct-peer evictions |

A missing page or a `null` value means "not known", not zero.

Define `CDPCFG_LOG_PACKET_HEX` to log every packet a duck builds, in hex, at INFO level. That helps when tracing a message from a duck to the cloud.

## Time sync (`topics::cmd_time`)

A PapaDuck with a trusted clock, such as one set from NTP or the cellular network, can share it with the mesh:

```cpp
papa.sendTimeSync(epoch);          // every duck
papa.sendTimeSync(epoch, duid);    // one duck
```

The payload is `{"epoch":<UTC seconds>}`. `sendTimeSync()` refuses an epoch before `CDPCFG_TIME_MIN_EPOCH`, so a PapaDuck without valid time can't push 1970 to the mesh. CDP never sends the time by itself: when and how often is up to the sketch. Send it periodically, because a duck that missed it or lost power has no time until the next one.

MamaDucks and DuckLinks built with `CDPCFG_REMOTE_TIME_SYNC=1` set their clock from it, but only if all of these hold:

- **First copy only.** It's the first copy of that message the duck has handled. Relayed copies arrive later and would set the clock back by their delay.
- **From a PapaDuck.** The sender's duck type in the header is PapaDuck.
- **A plausible time.** `epoch` is an integer of at least `CDPCFG_TIME_MIN_EPOCH` (2026-01-01 by default). A missing, negative or text epoch, or broken JSON, is rejected.
- **For this duck.** It's a broadcast or addressed to this duck. A command addressed to another duck is only passed on.

MamaDucks relay every broadcast `cmd_time` under the normal duplicate and hop rules, whether or not they applied it and whether or not they were built with time sync. DuckLinks apply it but don't relay it. `hasValidTime()` tells a sketch whether the clock has reached `CDPCFG_TIME_MIN_EPOCH`, whatever set it. The clock survives deep sleep and software resets, but starts again at 1970 after a power loss.

- **Accuracy:** the epoch is read when the PapaDuck queues the packet, and relays don't correct for their own delay. With 3 s between transmissions, ducks a few hops away are typically a few seconds behind, more on a busy mesh. That's fine for schedules measured in minutes, not for sub-second timestamps.
- **Security:** these checks make spoofing harder, but the command isn't authenticated. Anyone in radio range using the same sync word can send a packet that claims to come from a PapaDuck.
- **Echoes:** the PapaDuck's receive callback also gets the relayed copies of its own `cmd_time`. Leave them out when forwarding to the cloud.

## TX power (`topics::cmd_tx`)

A duck built with `CDPCFG_REMOTE_TX_POWER=1` stores the TX power from a `cmd_tx` packet, such as `{"txPwr":20}`, and uses it from its next boot.

- **Range.** The command allows 14 to 22 dBm. A value outside that range, or broken JSON, is ignored and not relayed.
- **Only what the radio takes.** SX1276 boards (Heltec V2, TTGO LoRa32) take 14 to 17 or 20 dBm. SX1262 boards take all of 14 to 22. The duck asks its radio before it stores a value. It doesn't store one the radio refuses, but still relays it for ducks with another radio.
- **Safe boot.** At boot the duck checks the stored value again. If it is outside 14 to 22 dBm or the radio doesn't take it, the duck removes it, logs one line and uses `CDPCFG_RF_LORA_TXPOW`. If radio setup still fails with the stored value, the duck sets up the radio again with the default, and removes the stored value if that works.
- **Next boot, not now.** On an SX1276, setting the power puts the radio in standby. That would cut off a packet being sent or received, so the new value waits for the next boot.
- **For this duck.** A MamaDuck applies a broadcast or a command addressed to it. A command addressed to another duck is only passed on. A DuckLink applies only a broadcast and ignores a command addressed to it.

The command isn't authenticated, and the top of the range can exceed regional limits.

## Deep sleep

CDP doesn't decide when a duck sleeps, but it gives the sketch two hooks:

- **`txIdle()`** is true when the receive, data and control queues are empty and the radio isn't transmitting, holding an unread packet or waiting for an interrupt to be serviced. Keep calling `run()` until it's true, with a timeout: a relay on a busy mesh may never be idle.
- **`prepareForSleep()`** puts the LoRa radio to sleep and stops the duck, so `sendData()` then returns `DUCK_ERR_NOT_SUPPORTED`. Call it right before `esp_deep_sleep_start()`.

Waking from deep sleep is a reboot. Setup runs again, and `setupWithDefaults()` reinitialises the radio, queues, routes and duplicate filter. There's no resume without a reboot.

## Configuration

All of these can be overridden at build time with `-D`.

| Setting | Default | Purpose |
|---|---|---|
| `CDPCFG_HEALTH_INTERVAL_MS` | 15 min | Radio health message and serial diagnostics page |
| `CDPCFG_SIGNAL_INTERVAL_MS` | 30 min | Signal-health report |
| `CDPCFG_ROUTE_KEEPALIVE_MS` | 10 min | Neighbour keepalive discovery |
| `CDPCFG_ROUTE_TTL_MS` | 30 min | Route entry lifetime without refresh |
| `CDPCFG_SIGNAL_MAX_NEIGHBORS` | 6 | Neighbours per signal-health message |
| `CDPCFG_RF_LORA_TXPOW` | 14 | Transmit power in dBm |
| `CDPCFG_RF_LORA_SYNC_WORD` | `0x12` | LoRa sync word. Ducks only hear the same value; use different values to keep nearby deployments apart. Not security |
| `CDPCFG_REMOTE_COMMANDS` | 0 (off) | Default for the next two settings, so `-DCDPCFG_REMOTE_COMMANDS=1` turns both on |
| `CDPCFG_REMOTE_TIME_SYNC` | `CDPCFG_REMOTE_COMMANDS` | Set the clock from `cmd_time` (see [Time sync](#time-sync-topicscmd_time)). Not authenticated. When off, MamaDucks still relay it |
| `CDPCFG_REMOTE_TX_POWER` | `CDPCFG_REMOTE_COMMANDS` | Set TX power from `cmd_tx`, 14 to 22 dBm, from the next boot (see [TX power](#tx-power-topicscmd_tx)). Only values the radio takes are stored. Not authenticated, and the top of that range can exceed regional limits. When off, MamaDucks still relay it |
| `CDPCFG_TIME_MIN_EPOCH` | 1767225600 (2026-01-01) | Oldest UTC time, in seconds, that `cmd_time` and `sendTimeSync()` accept and `hasValidTime()` counts as valid |
| `CDPCFG_MAX_QUEUE_SIZE` | 10 | Packets each of the RX, data-TX and control-TX queues can hold |
| `CDPCFG_MAX_PACKET_SEND_RATE` | 3000 ms | Minimum time between two transmissions |
| `CDPCFG_LOG_PACKET_HEX` | undefined | Log each packet a duck builds, in hex |

## Payload size

A LoRa frame is at most 255 bytes, including the 27-byte header, so the data section is limited to 228 bytes (`MAX_DATA_LENGTH`). Larger payloads are rejected before they reach the radio.
