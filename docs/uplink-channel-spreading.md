# Uplink Channel Spreading

## Overview

When a MamaDuck sends a packet it *originated* to PapaDuck, it picks a random
channel from an 8-channel pool (921.4–922.8 MHz, 200 kHz spacing) before
transmitting. This spreads uplink traffic across all SX1302 demodulators at the
gateway, reducing last-hop collisions.

Packets that are being **relayed** — including those relayed toward PapaDuck —
are always sent on the shared mesh channel (922.8 MHz) regardless of their
destination.

---

## Channels

| Define | Frequency | Role |
|---|---|---|
| `CDPCFG_RADIO_CHANNEL_1` | 922.8 MHz | Shared mesh channel (default) |
| `CDPCFG_RADIO_CHANNEL_2` | 922.6 MHz | Uplink pool |
| `CDPCFG_RADIO_CHANNEL_3` | 922.4 MHz | Uplink pool |
| `CDPCFG_RADIO_CHANNEL_4` | 922.2 MHz | Uplink pool |
| `CDPCFG_RADIO_CHANNEL_5` | 922.0 MHz | Uplink pool |
| `CDPCFG_RADIO_CHANNEL_6` | 921.8 MHz | Uplink pool |
| `CDPCFG_RADIO_CHANNEL_7` | 921.6 MHz | Uplink pool |
| `CDPCFG_RADIO_CHANNEL_8` | 921.4 MHz | Uplink pool |

All 8 channels are AS923 channels and are monitored simultaneously by the
SX1302 multi-SF demodulators at PapaDuck.

---

## Rules

### Channel switching applies when ALL of the following are true

1. `txPacket.dduid == PAPADUCK_DUID` — the packet is addressed to PapaDuck.
2. `txPacket.sduid == this->duid` — this duck is the **original sender**, not
   a relay.

### Channel switching does NOT apply when

- The packet is being **relayed** from another duck (`sduid != this->duid`),
  even if the destination is PapaDuck.
- The packet is addressed to any destination other than PapaDuck.

---

## Packet flow examples

### Originating duck → PapaDuck (channel switching ON)

```
DuckLink ──(922.8)──► MamaDuck A ──(random channel)──► PapaDuck
```

MamaDuck A originated the packet (`sduid == A`), so it picks a random uplink
channel before transmitting to PapaDuck.

### DuckLink → MamaDuck A → MamaDuck B → PapaDuck (channel switching OFF for relay)

```
DuckLink ──(922.8)──► MamaDuck A ──(922.8)──► MamaDuck B ──(922.8)──► PapaDuck
```

MamaDuck A and B are relaying a packet whose `sduid` is the DuckLink's ID.
Neither relay switches channels; both stay on 922.8 MHz so the next hop can
hear the packet. PapaDuck receives it on 922.8 MHz via its SX1302.

### MamaDuck originates and relays are present

```
MamaDuck A (originator) ──(random channel)──► PapaDuck
                        ──(922.8)──► MamaDuck B ──(922.8)──► PapaDuck
```

If MamaDuck A is close enough to PapaDuck, the random-channel packet reaches
it directly. If the packet also needs to be relayed through MamaDuck B, B
relays it on 922.8 MHz.

---

## Implementation

| File | Symbol | Description |
|---|---|---|
| `src/Ducks/Duck.h` | `sendToRadio()` | Applies the channel-switch condition before calling `duckRadio.sendData()` |
| `src/radio/DuckLoRa.h` / `.cpp` | `setUplinkFrequency()` | Calls RadioLib `lora.setFrequency()` to switch channel |
| `src/radio/DuckLoRa.h` / `.cpp` | `getRandomUplinkChannel()` | Picks uniformly from `CDPCFG_UPLINK_CHANNEL_POOL` using a Mersenne-Twister RNG |
| `src/include/cdpcfg.h` | `CDPCFG_UPLINK_CHANNEL_POOL` | Defines the 8-channel pool |

The mesh channel (922.8 MHz) is automatically restored after each transmission
in the TX_DONE interrupt handler inside `DuckLoRa`.
