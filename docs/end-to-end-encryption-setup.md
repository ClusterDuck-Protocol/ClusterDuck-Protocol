# Setting Up Full End-to-End Encryption (Operator ↔ Device)

This guide walks through provisioning **all three** independent encryption
layers used by MeshBeacon so a deployment is fully encrypted, not just
partially. See [crypto-design.tex](../docs) (meshbeacon-firmware) for the
underlying design; this doc is the operational "how do I turn it on" guide.

## The three layers (all independent — you need all three for full E2E)

| Layer | Mechanism | Protects | Off-by-default? |
|---|---|---|---|
| **1. Operator commands** (`encrypted_cmd`/`dcmd` topics 8/22, plus ALERT/23 and PMSG/25) | Static-static X25519 ECDH between OpenDMS (Laravel) and the device's own identity keypair | SOS ack, GPS requests, operator/alert/private messages | Yes — falls back to plaintext if either side lacks keys; **fail-closed** once `opendmsconfig::isConfigured()` (see below) |
| **2. Uplink + MTALK** (GPS/status/alert/roger uplinks, MamaDuck↔MamaDuck chat, topic 26) | One-way seal to OpenDMS's static key (uplink) / session ECDH between two Ducks' identities (MTALK) | Live location, mesh chat | Yes — gated by `Duck::isUplinkEncryptionEnabled()`, off unless explicitly enabled; **fail-closed** for MTALK once enabled |
| **3. Group broadcast** (BEACON/BEACON_ACK discovery, and the Emergency Broadcast button, topic 24) | Pre-shared symmetric key (`MESH_GROUP_KEY`), shared by every Duck in the deployment and by Laravel | GPS in BEACON (encrypted); Emergency Broadcast text (authenticated only — message stays cleartext, forgery is prevented) | Yes — falls back to unauthenticated/plaintext if no group key is provisioned; **fail-closed** once `meshgroupconfig::isConfigured()` |

Skipping any one layer means that traffic stays plaintext even if the other
two are fully configured. **Fail-closed** means once a layer's key is
provisioned on a device, that device rejects unauthenticated traffic on the
topics it covers instead of accepting it as plaintext — see
[Fail-closed behavior](#fail-closed-behavior-once-a-layer-is-configured)
below.

---

## Step 1 — Generate OpenDMS's static keypair (Laravel side)

This is the operator app's own long-term identity. Generate it once per
deployment (not per-device):

```bash
php artisan tinker
>>> sodium_crypto_box_keypair(); // or any X25519 keypair generator
```

Or, more simply, use libsodium directly:

```bash
php -r '
$kp = sodium_crypto_box_keypair();
echo "PRIVATE (base64): " . base64_encode(sodium_crypto_box_secretkey($kp)) . PHP_EOL;
echo "PUBLIC  (hex):     " . bin2hex(sodium_crypto_box_publickey($kp)) . PHP_EOL;
'
```

Set both in the Laravel repo's `.env`:

```env
DUCK_CRYPTO_PRIVATE_KEY=<base64 private key>
DUCK_CRYPTO_PUBLIC_KEY=<hex public key>
```

- `DUCK_CRYPTO_PUBLIC_KEY` **must be hex** — it's pasted as-is into the
  firmware build flag below, no re-encoding needed.
- `DUCK_CRYPTO_PRIVATE_KEY` stays base64 — only Laravel's own PHP code ever
  reads it; it is never sent to a device or compiled into firmware.
- **Back up the private key** (e.g. in your secrets manager). This is the
  one deliberate exception to the "no key backup" rule used elsewhere —
  losing it without a backup means re-flashing every already-fielded
  device with a new OpenDMS public key.
- Leaving both empty disables this layer entirely (Laravel falls back to
  plaintext `dcmd` for everything, as already discussed).

## Step 2 — Provision the device with OpenDMS's public key

The firmware needs to trust the *same* public key you just generated. Pick
one of three methods (see [OpenDmsConfig.h](../src/security/OpenDmsConfig.h)):

**A. Build flag (recommended for fleets built from one firmware image):**

```ini
; platformio.ini, in your build environment
build_flags =
    ${env:local_wio_tracker_l1.build_flags}
    -DOPENDMS_STATIC_PUBLIC_KEY_HEX=\"<hex public key from Step 1>\"
```

**B. Compile-time edit** — replace the placeholder initializer in
[OpenDmsConfig.cpp](../src/security/OpenDmsConfig.cpp) directly before
flashing (useful if you don't want the key baked into a shared
`platformio.ini`).

**C. Field provisioning over serial** (no reflash needed, e.g. to join an
already-deployed device to a new OpenDMS instance): connect over USB
serial and send:

```
AT+OPENDMSKEY=<64 hex chars>
```

This persists to flash (LittleFS on nRF52, EEPROM elsewhere) and survives
reboots. Use `AT+OPENDMSKEY?` to check the current value, or
`AT+OPENDMSKEY+RESET` to clear it before re-provisioning. No
authentication is required for this command since the public key isn't
secret.

You can convert the Laravel-side value with the included helper:

```bash
python tools/pubkey_to_c_array.py --format hex "<DUCK_CRYPTO_PUBLIC_KEY value>"
```

## Step 3 — Generate and provision the mesh group key (device + Laravel)

This is a separate, symmetric pre-shared key used for BEACON/BEACON_ACK
discovery broadcasts **and** the Emergency Broadcast button (topic 24) (see
[MeshGroupConfig.h](../src/security/MeshGroupConfig.h)). Generate a real
key — **do not reuse the demo key already committed in `platformio.ini`**:

```bash
openssl rand -hex 32
```

Then provision it on each device the same three ways as Step 2:

- Build flag: `-DMESH_GROUP_KEY_HEX=\"<64 hex chars>\"`
- Compile-time edit in `MeshGroupConfig.cpp`
- Serial: `AT+MESHKEY=<64 hex chars>` (`AT+MESHKEY?` / reset command also
  available, mirroring `AT+OPENDMSKEY`)

Every Duck in the same deployment must share this exact key.

**Also set it in Laravel's `.env`**, so `StatusController::broadcast()` (the
Emergency Broadcast button) can produce an authenticated broadcast the
devices will actually accept. Emergency Broadcast is MAC-only, not
encrypted — `DuckCryptoService::authenticateGroupBroadcast()` sends the
message as cleartext with an authentication tag appended, so anyone in
range can still read a life-safety alert, but only a holder of the group
key can produce one that verifies:

```env
DUCK_MESH_GROUP_KEY=<the same 64 hex chars as above>
```

If `DUCK_MESH_GROUP_KEY` is left empty, `MqttService::sendGroupBroadcast()`
sends the broadcast unauthenticated instead — which any device with its
own mesh group key configured will now silently drop (see below), so the
button will appear to work in Laravel but reach no provisioned device.

## Step 4 — Enable uplink encryption

Uplink/MTALK encryption (layer 2) is a separate opt-in flag, off by
default even if Steps 1–3 are done. Enable it at compile time:

```ini
build_flags =
    ${env:local_wio_tracker_l1.build_flags}
    -DDUCK_CRYPTO_DEFAULT_ENABLED=1
```

This flips `Duck::isUplinkEncryptionEnabled()` to `true` by default, which:
- routes GPS/status/alert/roger uplinks through `sendSealedData()` instead
  of plaintext `sendData()` (requires Step 2 to actually succeed), and
- makes the device broadcast its own identity (`announceIdentity()`) once
  at boot, so OpenDMS and nearby MamaDucks learn its public key (TOFU) and
  can use session-mode `encrypted_data`/MTALK encryption.

The repo already ships a ready-made example environment combining Steps 3
and 4 for local testing — copy its pattern and replace the demo group key:

```ini
[env:local_wio_tracker_l1_encrypted]
extends = env:local_wio_tracker_l1
build_flags =
    ${env:local_wio_tracker_l1.build_flags}
    -DDUCK_CRYPTO_DEFAULT_ENABLED=1
    -DMESH_GROUP_KEY_HEX=\"<your own key here, NOT the demo one>\"
    -DOPENDMS_STATIC_PUBLIC_KEY_HEX=\"<hex public key from Step 1>\"
```

### Alternative: pass flags via the shell, no `platformio.ini` edit needed

PlatformIO's `PLATFORMIO_BUILD_FLAGS` environment variable appends extra
flags to whichever environment you build, without editing the file at all
— useful for CI or per-device secrets you don't want committed:

```bash
EXAMPLE_DIR=Basic-Ducks/Seeed/WioTrackerL1 \
PLATFORMIO_BUILD_FLAGS='-DDUCK_CRYPTO_DEFAULT_ENABLED=1 -DMESH_GROUP_KEY_HEX=\"<64 hex chars>\" -DOPENDMS_STATIC_PUBLIC_KEY_HEX=\"<64 hex chars>\"' \
pio run -e local_wio_tracker_l1 -t upload
```

Build against the plain `local_wio_tracker_l1` env (not `..._encrypted`)
since the crypto flags are supplied here instead. Keep the `\"..\"`
escaping exactly as shown (and the single quotes around the whole
`PLATFORMIO_BUILD_FLAGS=...` value) — that's what makes the shell pass a
literal `-DMESH_GROUP_KEY_HEX="<hex>"` through to the compiler as a proper
C string literal, matching the same convention used inside
`platformio.ini` itself.

## Step 5 — Device identity keypair (no action needed)

Each device generates its own long-term X25519 identity keypair
automatically on first boot ([DuckIdentity.h](../src/security/DuckIdentity.h)),
persisted to flash. Nothing to configure — but note:
- The private key never leaves the device (not logged/displayed/transmitted).
- Laravel learns each device's public key automatically via TOFU when it
  receives that device's `identity_announce` broadcast (see
  `ProcessMqttMessage.php`, `DuckIdentity::firstOrCreate(...)`) — this only
  happens once uplink encryption is enabled (Step 4), since
  `announceIdentity()` is gated behind `isUplinkEncryptionEnabled()`.
- Until Laravel has seen a device's `identity_announce`,
  `sendEncryptedCommand()` for that specific device falls back to
  plaintext (see prior discussion) — this resolves itself automatically
  once the device has booted with encryption enabled and its
  `identity_announce` has reached a Papa hub.

## Step 6 — Build, flash, and verify

Build and flash with your encrypted environment:

```bash
EXAMPLE_DIR=Basic-Ducks/Seeed/WioTrackerL1 pio run -e local_wio_tracker_l1_encrypted -t upload
```

Verify each layer is actually active:

- **Operator commands**: check Laravel logs after sending a command —
  `MqttService: sendEncryptedCommand encrypted successfully for <duckId>`
  means it worked; any `"...falling back to plaintext dcmd"` line tells you
  exactly which precondition (identity or keypair) is still missing.
- **Device-side OpenDMS key**: `AT+OPENDMSKEY?` over serial should report a
  configured (non-zero) key matching Step 1's public key.
- **Group key**: `AT+MESHKEY?` should report a configured key, and it must
  match across every device in the deployment.
- **Uplink encryption**: confirm the build was compiled with
  `-DDUCK_CRYPTO_DEFAULT_ENABLED=1` (check the PlatformIO build output /
  `platformio.ini` env used); GPS uplinks will use `sealed_uplink` on the
  wire instead of the plain application topic.
- **Emergency Broadcast**: check Laravel logs after pressing the button —
  `MqttService: sendGroupBroadcast authenticated successfully` means it
  worked; `"...mesh group key not configured, sending unauthenticated
  broadcast"` means `DUCK_MESH_GROUP_KEY` isn't set in Laravel's `.env`
  yet.

## Fail-closed behavior once a layer is configured

Provisioning a key on a device is not just "enable encryption when
available" — for the topics below, it also makes that device **reject**
anything on the same topic that doesn't successfully decrypt/authenticate,
instead of falling back to trusting the plaintext bytes. This closes the
forgery gap where anyone in LoRa range could otherwise send a raw packet on
a reserved app topic and have it treated as a genuine operator/OpenDMS
message.

- **Once `opendmsconfig::isConfigured()`** (Step 2 done): topics 22
  (`dcmd`), 23 (ALERT), and 25 (PMSG) all drop any packet that isn't a real
  `encrypted_cmd` (topic 8) — a bare/plaintext packet on 22/23/25 is logged
  and discarded, not displayed or acted on.
- **Once `Duck::isUplinkEncryptionEnabled()`** (Step 4 done): topic 26
  (MTALK) requires the packet to have arrived via `encrypted_data` and been
  successfully decrypted (`CdpPacket::wasAuthenticated`) — a bare topic-26
  packet is dropped.
- **Once `meshgroupconfig::isConfigured()`** (Step 3 done): BEACON and
  BEACON_ACK require successful group-key decryption before their GPS
  payload is trusted at all (an undecryptable BEACON's coordinates are
  discarded, not displayed); Emergency Broadcast (topic 24) is
  authenticated, not encrypted — the message itself always travels as
  cleartext, but a device with the group key configured requires the
  accompanying tag to verify (marker byte `0xE8`, distinct from BEACON's
  own marker and from the CDP framework's `reservedTopic::group_broadcast`)
  before the text is displayed/relayed, and rejects an untagged/forged
  packet outright — see `verifyBroadcastMac()` in `MamaDuck.ino` and
  `DuckCryptoService::authenticateGroupBroadcast()` on the Laravel side.

A practical consequence: partially configuring a deployment (e.g. giving
some devices a mesh group key but leaving Laravel's `DUCK_MESH_GROUP_KEY`
unset) can make a previously-working feature go silently quiet on
provisioned devices rather than degrade gracefully — always provision both
sides of a layer together.

## Known limitations (not covered by this setup)

- **No replay/freshness protection** on the `encrypted_cmd` channel — a
  captured, previously-valid ciphertext could in principle be re-sent. This
  was deliberately deferred (would require a wire-format change across both
  repos); ask if you want this implemented separately.
- Group broadcast confidentiality only protects against passive
  eavesdroppers outside the mesh — every Duck holding the shared group key
  can read all group traffic (it's a shared-secret scheme, not per-device).
- Losing the OpenDMS private key (Step 1) without a backup requires
  re-provisioning every already-fielded device's public-key pin (Step 2) to
  a newly generated keypair.
