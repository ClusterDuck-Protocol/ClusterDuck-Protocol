# CDK Frame Protocol

Defines the line-oriented text protocol used between the **MamaDuck firmware** and the
**companion phone app** (or any USB serial host).

## Transport

| Channel | Details |
|---------|---------|
| BLE | Nordic UART Service (NUS). Service `6E400001-…`, RX char `6E400002-…`, TX char `6E400003-…`. Frames are UTF-8 strings terminated with `\n`. |
| USB Serial | 115200 baud, 8N1. Same `\n`-terminated frame format. |

All frames have the prefix `CDK:` followed by a type token and optional comma-separated
key-value fields.

```
CDK:<TYPE>[,<KEY>:<VALUE>[,<KEY>:<VALUE>…]]\n
```

---

## Frames sent **by the device → phone**

### `CDK:ID`
Sent on BLE connect, USB connect, and in response to every inbound frame (for reconnect
recovery).

```
CDK:ID,VALUE:<duck_id>
```

| Field | Description |
|-------|-------------|
| `VALUE` | 8-character duck identifier (e.g. `MNZAIHAN`) |

---

### `CDK:BATT`
Sent on connect and every 60 seconds.

```
CDK:BATT,LEVEL:<percent>
```

| Field | Description |
|-------|-------------|
| `LEVEL` | Battery percentage `0`–`100` |

---

### `CDK:MSG`
Relays a LoRa text message (topics 22 or 23) received from the mesh.

```
CDK:MSG,TEXT:<message>
```

---

### `CDK:BCAST`
Relays an emergency broadcast (topic 24) from the mesh operator.

```
CDK:BCAST,TEXT:<message>
```

---

### `CDK:PMSG`
Relays a personal message (topic 25) from the mesh.

```
CDK:PMSG,TEXT:<message>
```

---

### `CDK:SOS`
Notifies the phone that the **hardware button** SOS was sent successfully.

```
CDK:SOS,SRC:DEVICE,ID:<duck_id>,LAT:<lat>,LNG:<lng>
```

| Field | Description |
|-------|-------------|
| `SRC` | Always `DEVICE` (hardware button origin) |
| `ID` | 8-character duck identifier |
| `LAT` | Latitude string, or `none` if no GPS fix |
| `LNG` | Longitude string, or `none` if no GPS fix |

---

### `CDK:GPSREQ`
Requests the phone to reply with its current GPS coordinates (see `CDK:GPS` below).
Sent when a LoRa GPS request (topic `0xEA`) arrives and no hardware GPS fix is
available, **and** a phone is connected.

```
CDK:GPSREQ
```

No fields. The phone must reply with `CDK:GPS,LAT:<lat>,LNG:<lng>` (or
`CDK:GPS,LAT:none,LNG:none` if unavailable).

---

### `CDK:MTALK`
Delivers a MamaDuck-to-MamaDuck (MTALK, topic 26) message to the phone.

```
CDK:MTALK,TEXT:<text>,FROM:<sender_duck_id>[,MID:<4-char-id>]
```

| Field | Description |
|-------|-------------|
| `TEXT` | Message body |
| `FROM` | 8-character sender duck ID |
| `MID` | Optional 4-character message ID for delivery receipts |

---

### `CDK:MACK`
Delivery receipt — the target duck has received an MTALK message.

```
CDK:MACK,ID:<mid>,FROM:<sender_duck_id>
```

| Field | Description |
|-------|-------------|
| `ID` | Message ID that was acknowledged |
| `FROM` | Duck ID that sent the acknowledgement |

---

### `CDK:ACK`
Generic acknowledgement sent after the device has forwarded a frame onto LoRa.

```
CDK:ACK,ID:<frame_type>[,TARGET:<duck_id>]
```

| `ID` value | Triggered by |
|------------|--------------|
| `SOS` | `CDK:SOS` from phone processed and sent |
| `MSG` | `CDK:MSG` from phone processed and sent |
| `MTALK` | `CDK:MTALK` from phone processed and sent |

---

### `CDK:RADIOREGION`
Reply to a phone `CDK:RADIOREGION` query, or an ack after a successful region write.
Also sent (error form) after a rejected write.

```
CDK:RADIOREGION,VALUE:<region_code>[,STATUS:ok,REBOOT_REQUIRED:1]
CDK:RADIOREGION,ERROR:<unknown_region|write_failed>
```

| Field | Description |
|-------|-------------|
| `VALUE` | Active/newly-set region code: one of `MY`, `SG`, `PH`, `ID`, `US`, `UK` |
| `STATUS` | `ok`, present only after a successful write |
| `REBOOT_REQUIRED` | `1` after a successful write -- the new region only takes effect after the device reboots; the running radio is not retuned live |
| `ERROR` | `unknown_region` (VALUE didn't match a known code) or `write_failed` (flash write failed) |

---

## Frames sent **by the phone → device**

### `CDK:PING`
Sent by the app after subscribing to the BLE TX characteristic. The device replies with
`CDK:ID`.

```
CDK:PING
```

---

### `CDK:SOS`
Instructs the device to send an SOS alert (topic `topics::status`) onto LoRa.

```
CDK:SOS,LAT:<lat>,LNG:<lng>
```

| Field | Description |
|-------|-------------|
| `LAT` | Latitude string from phone GPS |
| `LNG` | Longitude string from phone GPS |

---

### `CDK:MSG`
Instructs the device to send a text message (topic `topics::status`) onto LoRa.

```
CDK:MSG,URGENCY:<level>,LAT:<lat>,LNG:<lng>,TEXT:<text>
```

| Field | Description |
|-------|-------------|
| `URGENCY` | Urgency level string |
| `LAT` | Latitude (may be empty) |
| `LNG` | Longitude (may be empty) |
| `TEXT` | Message body |

---

### `CDK:GPS`
Phone reply to `CDK:GPSREQ`. Provides the phone's current GPS coordinates.

```
CDK:GPS,LAT:<lat>,LNG:<lng>
```

| Field | Description |
|-------|-------------|
| `LAT` | Latitude string, or `none` if phone has no fix |
| `LNG` | Longitude string, or `none` if phone has no fix |

The device forwards this onto LoRa as `GPS,SRC:PHONE,LAT:…,LNG:…` (topic `0xEA`).
If both values are `none`, the device sends `GPS,FIX:0,SRC:PHONE`.

---

### `CDK:MTALK`
Instructs the device to send a targeted duck-to-duck message (topic 26) to another
MamaDuck.

```
CDK:MTALK,TARGET:<duck_id>,TEXT:<text>[,MID:<4-char-id>]
```

| Field | Description |
|-------|-------------|
| `TARGET` | Exactly 8-character destination duck ID |
| `TEXT` | Message body |
| `MID` | Optional 4-character message ID; echoed back in `CDK:MACK` |

---

### `CDK:RADIOREGION`
Sets or queries the device's LoRa region preset (mesh channel + uplink channel pool).
Send with no `VALUE` field to query the currently active region; the device replies
with `CDK:RADIOREGION,VALUE:<region_code>`.

```
CDK:RADIOREGION[,VALUE:<region_code>]
```

| Field | Description |
|-------|-------------|
| `VALUE` | Optional. One of `MY`, `SG`, `PH`, `ID`, `US`, `UK`. Omit to query the current region instead of changing it. |

Changing the region persists it to flash and updates the mesh channel/uplink pool used
on the *next* boot; the device must be rebooted (see `REBOOT_REQUIRED:1` in the device's
ack) for the change to actually take effect on air.

---

## LoRa GPS payload reference

These payloads are sent over LoRa (topic `0xEA = 234`) and stored by OpenDMS.

| Payload | Meaning |
|---------|---------|
| `GPS,LAT:<lat>,LNG:<lng>,SATS:<n>` | Hardware GPS fix (V4 board) |
| `GPS,SRC:PHONE,LAT:<lat>,LNG:<lng>` | GPS sourced from phone |
| `GPS,FIX:0,SRC:PHONE` | Phone connected but has no fix |
| `GPS,FIX:0,SRC:NONE,REASON:NO_PHONE` | No hardware GPS and no phone connected |

---

## GPS request flow

```
OpenDMS                  MamaDuck (no hw GPS fix)        Phone App
   |                             |                           |
   |-- LoRa topic 0xEA --------->|                           |
   |   (GPS location request)    |                           |
   |                             |-- CDK:GPSREQ ------------>|
   |                             |                           | (reads GPS)
   |                             |<-- CDK:GPS,LAT:x,LNG:y --|
   |<-- LoRa GPS,SRC:PHONE ------|                           |
```

If no phone is connected when the GPS request arrives, the device immediately responds
over LoRa with `GPS,FIX:0,SRC:NONE,REASON:NO_PHONE` so OpenDMS knows the device is
alive but GPS is unavailable.
