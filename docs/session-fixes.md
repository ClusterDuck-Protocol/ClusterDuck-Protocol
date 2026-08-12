# ClusterDuck Protocol — Bug Fixes & Changes

Documented issues diagnosed and resolved across firmware (`MamaDuck.ino`),
mobile app (`clusterduck-mobile`), and dashboard (`opendms`).

---

## 1. Bloom Filter Self-MUID Bug (Relay Loop)

**Symptom**  
Packets from the sending duck would be re-relayed after another duck echoed
them back, causing the radio to be busy and missing incoming ACKs (e.g.
BEACON_ACK never arrived, GPS didn't appear on the map).

**Root Cause**  
`BloomFilter::assignUniqueMessageId()` generated a unique MUID and assigned
it to the packet but did **not** add it to the sender's own bloom filter.
When a relay duck echoed the packet back, the sender's bloom check returned
"not seen" → it relayed its own packet again → flood loop.

**Fix** (`src/routing/bloomfilter.cpp`)  
Added `bloom_add` immediately after the MUID is assigned:
```cpp
void BloomFilter::assignUniqueMessageId(CdpPacket& packet) {
    while (bloom_check(packet.muid.data(), MUID_LENGTH)) {
        duckutils::getRandomBytes(MUID_LENGTH, packet.muid.data());
    }
    bloom_add(packet.muid.data(), MUID_LENGTH); // ← added
}
```

---

## 2. BEACON / BEACON_ACK GPS Not Appearing on Map

**Symptom**  
After sending a BEACON (topic 27) and receiving a BEACON_ACK (topic 28),
the peer duck's GPS position was not appearing on the map in the mobile app.

**Root Cause**  
Multiple compounding factors:
- Bloom filter self-MUID bug (see §1) caused re-relay loops that kept the
  radio in TX state, dropping the BEACON_ACK.
- BEACON and BEACON_ACK were sent with the duck's own DUID as destination
  instead of `BROADCAST_DUID`, so intermediate ducks didn't relay them.

**Fix** (`examples/Basic-Ducks/Heltec/MamaDuck.ino`)  
- Bloom filter fix (§1) eliminated the relay loop.
- BEACON/BEACON_ACK are now sent with `BROADCAST_DUID` as the 3rd argument
  to `sendData` so they propagate through the mesh.
- Added 350 ms deferred TX timer for BEACON_ACK to avoid TX collision with
  the sender.
- Added `goPublic()` call in `setup()` to skip the 80-second SEARCHING state.

---

## 3. Emergency OLED Display Auto-Clearing Too Fast

**Symptom**  
SOS sent confirmation, operator messages, and SOS ACK screens would
disappear after a few seconds, before the operator had read them.

**Root Cause**  
The home-screen refresh timer and the GPS display timer both called
`display.displayOff()` / `displayHome()` unconditionally, overwriting
emergency screens.

**Fix** (`examples/Basic-Ducks/Heltec/MamaDuck.ino`)  
Added `static bool emergencyDisplayPending` flag:
- Set to `true` by SOS send confirmation, SOS ACK, operator messages
  (topic 22), and emergency broadcasts.
- All display refresh timers check `!emergencyDisplayPending` before
  updating the screen.
- Cleared only when the user **single-clicks** the program button.

---

## 4. GPS Not Sent to OpenDMS When Phone GPS Cache Is Hot

**Symptom**  
When the device received a GPSREQ (topic 234) from the mesh while the phone
GPS cache was already populated, the cached GPS was never forwarded upstream.

**Root Cause**  
The GPSREQ handler detected the cache was hot and returned early (correctly
avoiding a duplicate network request), but it never constructed and sent the
deferred GPS payload (`gpsTxPending`).

**Fix** (`examples/Basic-Ducks/Heltec/MamaDuck.ino`)  
When the cache is hot and GPSREQ is skipped, the handler now builds the GPS
payload from cached values and sets `gpsTxPending = true` so the main loop
transmits it.

---

## 5. BLE Android 13 Immediate Disconnect (< 1 s)

**Symptom**  
Android 13 phones would connect to the duck's BLE but disconnect within
~700 ms, before the app could discover services.

**Root Cause**  
Eight compounding issues were identified:

| # | Cause | Fix |
|---|-------|-----|
| 1 | `onConnect` sent BLE notifications before Android finished MTU negotiation and CCCD subscription | Moved ID + battery announce to a 700 ms deferred timer |
| 2 | `delay(2000)` in BLE connect splash blocked `duck.run()` | Replaced with non-blocking `bleSplashClearMs` timer |
| 3 | USB discovery loop flooded BLE with battery every 3 s | Gated with `if (!bleConnected)` |
| 4 | `handleFrame` sent `CDK:ID` on every frame | Rate-limited to once per 10 s |
| 5 | Stale iPhone LTK bond in NVS caused Android security rejection | Added `deleteAllBonds()` (later removed when bonding was stripped) |
| 6 | Static public BLE address rejected by Android 13 privacy mode | Set `BLE_OWN_ADDR_RANDOM` |
| 7 | MTU mismatch caused GATT fragmentation errors | Set `NimBLEDevice::setMTU(512)` |
| 8 | NUS service UUID missing from BLE scan response | Added `scanRsp.addServiceUUID(NUS_SERVICE)` |

---

## 6. BLE Pairing / Security

**History**  
Several pairing approaches were tried and evaluated:

- **Automatic MITM passkey on connect**: Implemented with
  `setSecurityAuth(true, true, false)` + `BLE_HS_IO_DISPLAY_ONLY` +
  `startSecurity()`. Worked but complicated UX.
- **Double-click triggered pairing**: Passkey shown on OLED only when
  operator double-clicks the program button. Allowed unencrypted use
  otherwise.
- **Final decision — no bonding**: For a disaster-response mesh device,
  maximum accessibility is the correct design. Any phone must be able to
  send an SOS without pairing. Bonding was removed entirely:
  - `setSecurityAuth(false, false, false)`
  - `BLE_HS_IO_NO_INPUT_OUTPUT`
  - No passkey generation or storage

---

## 7. SOS ACK Received Multiple Times (Duplicate Notifications)

**Symptom**  
OpenDMS sends 3 SOS ACKs (immediate + 2 retries at 10 s intervals). The
device was triggering 5 or more notifications.

**Root Cause**  
`recvDataCallback` is called in `MamaDuck.h` not only when a packet is
directly addressed to the duck but also when the duck **relays** a packet
(see `ifBroadcast` and `ifNotBroadcast` default cases). This is intentional —
it enables `CDK:SEEN` peer-discovery for all overheard ducks. The side effect
is that every relay hop re-runs `handleDuckData`, matching topic 22 +
"SOS DITERIMA" again. `ifBroadcast`/`ifNotBroadcast` were **not** changed to
avoid breaking peer discovery.

**Fix** (`examples/Basic-Ducks/Heltec/MamaDuck.ino`)  
Added a 5-second debounce on the SOS ACK case:
```cpp
static unsigned long lastSosAckMs = 0;
if (millis() - lastSosAckMs < 5000UL) { break; } // suppress relay copies
lastSosAckMs = millis();
```
5 s is long enough to suppress relay duplicates (which arrive within ~1–2 s)
but short enough to allow the 10 s OpenDMS retransmissions through.

---

## 8. PapaDuck Appearing in Nearby Nodes List

**Symptom**  
The PapaDuck (internet gateway) was showing in the mobile app's "Nearby
Nodes" list and map, alongside user-facing MamaDucks and DuckLinks.

**Root Cause**  
`addSeen()` in `use-nearby-ducks.ts` accepted all duck types without
filtering infrastructure nodes.

**Fix** (`clusterduck-mobile/hooks/use-nearby-ducks.ts`)  
```typescript
if (duckType === "PAPA") return; // gateway infrastructure — exclude from list
```
Existing PAPA entries expire naturally within the 5-minute TTL.

---

## 9. Phone-App SOS Categorised as "SOS HW" in OpenDMS Active Incidents

**Symptom**  
SOS sent from the mobile app was appearing in the Active Incidents dashboard
panel labelled "SOS HW" (hardware button) instead of "SOS" (mobile app).

**Root Cause**  
Two issues:

1. `getActiveIncidents()` only queried `WHERE topic = 'alert'`. Hardware SOS
   uses `topics::alert` (0x14); phone SOS uses `topics::status` (0x10) which
   the gateway stores as `topic='status'`. Phone SOS never created a new
   incident entry — the stale hardware SOS record remained.

2. As a result, the incident shown was always the last **hardware** SOS,
   hence "SOS HW".

**Fix** (`opendms/app/Repositories/ClusterDataRepository.php`)  
Extended `getActiveIncidents` to include `topic='status'` records whose
payload begins with `SOS,`:
```php
->where(function ($q) {
    $q->where('topic', 'alert')
      ->orWhere(function ($q2) {
          $q2->where('topic', 'status')
             ->where('payload', 'like', 'SOS,%');
      });
})
```
The existing `sos_from_device` / `sos_from_mobile` model attributes (which
check for `SRC:DEVICE` in the payload) then correctly badge each incident.

---

## 10. Redundant `ID:DUCKNAME` in LoRa Payload

**Symptom** / **Finding**  
The SOS LoRa payload included `ID:<DUCKNAME>` (e.g. `SOS,SRC:DEVICE,ID:IBRAHIM1,...`).
Neither OpenDMS nor the mobile app parsed this field — OpenDMS uses
`DeviceID` from the MQTT envelope (stored as `duck_id`), and the app uses
`CDK:ID,VALUE:` announce frames.

**Fix**  
- Removed `,ID:<DUCKNAME>` from the LoRa payload in `sendEmergency()`
  (`examples/Basic-Ducks/Heltec/MamaDuck.ino`).
- Removed `,ID:<DUCKNAME>` from the `CDK:SOS` BLE notification frame.
- Updated `use-esp32-data.ts` to fall back to `state.deviceId` (populated
  by `CDK:ID,VALUE:`) when the SOS frame's `deviceId` field is empty, so
  push notification bodies still show the correct duck name.

---

## 11. Battery Update Frequency Too Slow Over BLE

**Symptom**  
Battery level in the mobile app was slow to update when connected via BLE.

**Root Cause**  
A previous fix to prevent Android 13 BLE disconnect (§5) gated the USB
discovery loop's 3 s battery send with `if (!bleConnected)`, and the
periodic BLE battery timer was set to 30 s.

**Fix** (`examples/Basic-Ducks/Heltec/MamaDuck.ino`)  
Reduced BLE-connected periodic battery interval from 30 s to 10 s:
```cpp
if (millis() - lastBattMs >= (bleConnected ? 10000UL : 60000UL))
```

---

## 12. Node Map Missing Connection Status Banner

**Symptom**  
All app screens except the Node Map showed a connection status banner when
the device was disconnected. The map gave no feedback.

**Fix**  
- Added `disconnectedOnly` prop to `SerialStatusBanner`
  (`clusterduck-mobile/components/serial-status-banner.tsx`): when `true`,
  the banner renders nothing when status is `"connected"` (Option 3 — silent
  when all is well, unmissable red bar when disconnected).
- Added `<SerialStatusBanner disconnectedOnly />` between the header and map
  in `app/(tabs)/map.tsx`.

---

## 13. Bloom Filter Reset Bug (Filter Saturation After Prolonged Uptime)

**Symptom**  
A duck could send/relay packets fine for hours, then stop being able to
send or forward anything at all. Restarting the affected device (or, on the
gateway, restarting the PapaDuck) temporarily fixed it. Since the two-phase
bloom filter is randomly seeded per boot, the exact time-to-failure and
which device appeared "stuck" varied — busier/longer-running senders hit it
sooner, and on the gateway it was worse because a saturated filter caused
`PapaDuck.h::handleReceivedPacket()` to drop **all** incoming packets, not
just relay decisions.

**Root Cause**  
`BloomFilter::bloom_add()` clears the inactive filter array before switching
to it, so it starts each new window empty. The clear loop used the wrong
bound:
```cpp
for (int i = 0; i < (this->numSectors)/(this->bitsPerSector); i++) {
    this->filter2[i] = 0;
}
```
`filter1`/`filter2` are each allocated as `numSectors` words
(`new unsigned int[this->numSectors]`), so a full clear requires looping
`numSectors` times. Dividing by `bitsPerSector` (312/32 = 9 with the
defaults) cleared only the first 9 of 312 words per switch. The remaining
~303 words kept stale bits from earlier cycles, so after a handful of
filter switches nearly every word in both filters had bits set. From then
on `bloom_check()` returned "already seen" for almost any input — including
brand-new, never-sent MUIDs — because it's declared a match as soon as
every hashed bit position happens to already be set somewhere in the
filter.

**Fix** (`src/routing/bloomfilter.cpp`)  
Loop the full array length on both switch branches:
```cpp
for (int i = 0; i < this->numSectors; i++) { this->filter2[i] = 0; }
...
for (int i = 0; i < this->numSectors; i++) { this->filter1[i] = 0; }
```
Also bounded the MUID-generation retry loop in `assignUniqueMessageId()`
to `MAX_MUID_ATTEMPTS = 50` (with a `logerr_ln` warning on saturation)
so a still-saturated filter can no longer hang the send path indefinitely
instead of eventually giving up and sending anyway.

**Important — separate vendored copy on the gateway**  
The PapaDuck gateway (`clusterduckd`) vendors its own independent copy of
`src/routing/bloomfilter.cpp` (not a symlink/submodule of this repo), so
this fix had to be applied there separately. That copy was also missing
the `bloom_add()` self-MUID fix from §1 above, which was restored at the
same time. Any future bloom filter change here should be checked against
that copy too, since the two can silently drift out of sync.
