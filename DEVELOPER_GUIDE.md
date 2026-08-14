# CDP Project Developer Guide

## Table of Contents
- [Introduction](#introduction)
- [Prerequisites](#prerequisites)
- [Project Setup And Build](#setup)
- [Testing](#testing)
- [How to run the tests](#how-to-run-the-tests)
- [How to run the examples](#how-to-run-the-examples)
- [End-to-End Encryption Setup (DuckCrypto)](#end-to-end-encryption-setup-duckcrypto)
  - [1. Generate OpenDMS's Static Keypair (meshbeacon server)](#1-generate-opendmss-static-keypair-meshbeacon-server)
  - [2. Gateway / Uplink Pass-Through (meshbeacon-uplink)](#2-gateway--uplink-pass-through-meshbeacon-uplink)
  - [3. Field Provisioning: Setting the OpenDMS Public Key](#3-field-provisioning-setting-the-opendms-public-key)
  - [4. Duck-to-Duck Session Encryption (opt-in)](#4-duck-to-duck-session-encryption-opt-in)
  - [5. Duck-to-OpenDMS Sealed Uplink (opt-in)](#5-duck-to-opendms-sealed-uplink-opt-in)
  - [6. OpenDMS-to-Duck Encrypted Downlink Commands](#6-opendms-to-duck-encrypted-downlink-commands)
  - [Security Notes and Caveats](#security-notes-and-caveats)

## Introduction 
This guide will help you install the ClusterDuck Protocol (CDP) on your development machine. The CDP is a set of libraries and tools that enable the development of mesh networks for IoT devices. The CDP is designed to be used with the PlatformIO development environment and is compatible with the Arduino framework and IDE as well.

This guide will help you set up the CDP on your development machine and build the firmware for your development board. It is  primarily intended for developers who want to contribute to the CDP or use it in their projects.

## Prerequisites
- [PlatformIO](https://platformio.org/install/ide?install=vscode) installed on your development machine.
- [VSCode](https://code.visualstudio.com/download) or any other IDE compatible with PlatformIO. 
- A development board compatible with the CDP. For example, the Heltec LoRa v3 Arduino board.
- C/C++ IntelliSense, debugging, and code browsing capabilities for your IDE. For example, the C/C++ extension for VSCode. (C/C++, C/C++ Themes and C/C++ Extension Pack from Microsoft)

## Project Setup And Build
1. Clone the CDP repository to your development machine.
    ```bash
    $ git clone https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol.git
    ```

2. Open the project in VSCode or your preferred IDE. In VSCode by clicking on the "Open Folder" icon in the left sidebar and selecting the `ClusterDuck-Protocol` folder.

3. Open the `platformio.ini` file and select the environment for your development board. For example, the Heltec LoRa v3 Arduino board environment is defined as follows:
    ```ini
    [platformio]

       default_envs = local_heltec_wifi_lora_32_V3
    ;   default_envs = local_heltec_wifi_lora_32_V2
    ;   default_envs = local_lilygo_t_beam_sx1276
    ;   default_envs = local_lilygo_t_beam_sx1262

    ;   default_envs = prod_heltec_wifi_lora_32_V3
    ;   default_envs = prod_heltec_wifi_lora_32_V2
    ;   default_envs = prod_lilygo_t_beam_sx1276
    ;   default_envs = prod_lilygo_t_beam_sx1262

    ;  default_envs = test_heltec_wifi_lora_32_V3
    ;  default_envs = test_lilygo_t_beam_sx1262
    ;  default_envs = test_ttgo_lora32_v1

    ```
This will set the environment for the Heltec LoRa v3 Arduino board. You can select the environment for your development board by uncommenting the corresponding line.

4. Build the project by clicking on the PlatformIO icon in the VSCode sidebar and selecting the `Build` option. This will compile the project and generate the firmware for your development board.

The CDP project build a library which by itself does not do much. It is intended to be used as a dependency in other projects. However, the project also includes a few examples that demonstrate how to use the CDP library. You can find these examples in the `examples` folder. Additionally the project includes unit tests that validate the CDP publicly accessible APIs. These tests are located in the `test` folder.

## Testing
Starting with release 3.7.0 we have unit tests available with the PlatformIO test framework `unity`

Tests are located in the `ClusterDuckProtocol/test` folder. These tests are unit tests as they validate the CDP publicly accessible APIs. However they must be run on a device. This means you have to connect a device to your development machine and build the tests to run on the device. Platform IO `test` command will build, deploy and run the tests and report back the results on your terminal console.

Before building and uploading tests and examples to the device you need to install the platformio CLI (command line interface) on your system. You can find the installation instructions [here](https://platformio.org/install/cli).

### How to run the tests
Here are the steps to run the tests (on Linux or Mac OS). This assumes you have platformIO installed on your system.

1. Open a terminal

2. Go to the project root folder (where the platformio.ini is located)
    ```bash
    $ cd ClusterDuckProtocol
    ```

3. Run the tests
    ```bash
    $ platformio test -e test_heltec_wifi_lora_32_V3
    ```

    To run a specific test suite, you can use the `--filter` of `-f` option. For example, to run the `test_DuckUtils` test suite, you can use the following command:
    ```bash
    $ platformio test -e test_heltec_wifi_lora_32_V3 --filter test_DuckUtils
    ```

This will build, deploy and run the tests and report back the results on your terminal console.

### How to run the examples
Here are the steps to run the examples (on Linux or Mac OS). This assumes you have platformIO installed on your system.

1. Open a terminal

2. Go to the project root folder (where the platformio.ini is located)
    ```bash
    cd ClusterDuckProtocol
    ```

3. Run the examples for the Lilygo Tbeam (with SX1276 LoRa chip) using your local CDP library (indicated by the `local_` prefix of board environment), supplying the relative path to the example folder you want to use. If no EXAMPLE_DIR is supplied, it defaults to Basic-Ducks/MamaDuck
    ```bash
    EXAMPLE_DIR=Basic-Ducks/DuckLink platformio run -e local_lilygo_t_beam_sx1276 -t upload
    ```

4. Run the examples for the Lilygo Tbeam (with SX1276 LoRa chip) using the CDP library from the PlatformIO library registry (indicated by the `prod_` prefix of board environment).
    ```bash
    EXAMPLE_DIR=Basic-Ducks/DuckLink platformio run -e prod_lilygo_t_beam_sx1276 -t upload
    ```
   If you are flashing from **Windows**, you may need to run the commands separately as below instead:
   ```
   $env:EXAMPLE_DIR="Basic-Ducks/DuckLink"
   platformio run -e prod_lilygo_t_beam_sx1276 -t upload
   ```

## End-to-End Encryption Setup (DuckCrypto)

DuckCrypto is the mesh's end-to-end encryption layer, spanning three repos:

| Repo | Role |
|---|---|
| `meshbeacon-firmware` (this repo) | Each Duck's own X25519 identity (`DuckIdentity`), the AEAD primitives (`DuckCrypto`), and OpenDMS's pinned public key (`OpenDmsConfig`). |
| `meshbeacon-uplink` (`clusterduckd`) | The LoRa gateway. It only relays encrypted payloads as opaque base64 blobs between the mesh and MQTT -- it never decrypts anything itself. |
| `meshbeacon` (Laravel/OpenDMS) | Holds OpenDMS's static X25519 keypair (`DuckCryptoService`) and encrypts/decrypts traffic to/from Ducks over MQTT. |

Each Duck generates its own X25519 identity keypair on first boot (see `DuckIdentity`) -- nothing to configure there. Setting up encryption for a deployment means: (a) giving OpenDMS a static keypair, and (b) pinning its public half into every Duck's firmware. The steps below do that, then cover the three encryption modes available once it's configured.

### 1. Generate OpenDMS's Static Keypair (meshbeacon server)

OpenDMS needs one fixed X25519 keypair for the whole deployment (not per-Duck). Generate it with `php artisan tinker` (or `php -r`) using libsodium, which is what `DuckCryptoService` uses at runtime:

```php
$kp = sodium_crypto_box_keypair();
echo "DUCK_CRYPTO_PRIVATE_KEY=" . base64_encode(sodium_crypto_box_secretkey($kp)) . "\n";
echo "DUCK_CRYPTO_PUBLIC_KEY="  . base64_encode(sodium_crypto_box_publickey($kp)) . "\n";
```

Paste the two output lines as literal `KEY=value` lines into the **`meshbeacon` (Laravel/OpenDMS) repo's root `.env` file** -- not this firmware repo, which has no `.env` of its own. The names must match exactly as printed (`DUCK_CRYPTO_PRIVATE_KEY`, `DUCK_CRYPTO_PUBLIC_KEY`), since `config/services.php`'s `duck_crypto` entry reads them by those exact names via `env(...)`. Treat `DUCK_CRYPTO_PRIVATE_KEY` like any other secret (back it up in your usual `.env`/secrets manager) -- this is the one deliberate exception to the "never back up a key" rule used elsewhere in DuckCrypto, since losing it without a backup means re-flashing every already-fielded device with a new public key. `DuckCryptoService::isConfigured()` returns `false` (and callers fall back to unencrypted behavior) until both values are set.

There's no firmware-style "build time" option for *this* keypair -- Laravel has no compile step; `.env` is read at runtime (or baked into a cached file via `php artisan config:cache`, which still needs the values in `.env` first). The compile-time option in step 3 below is a separate thing: baking OpenDMS's **public** key (`DUCK_CRYPTO_PUBLIC_KEY`, generated here) into a *Duck's* firmware -- it doesn't apply to generating or storing this server-side keypair itself.

### 2. Gateway / Uplink Pass-Through (meshbeacon-uplink)

`clusterduckd` requires no encryption-specific configuration -- see `MQTT_CONFIG.md` for its normal MQTT setup (broker, topics, optional TLS). The four DuckCrypto reserved topics (`encrypted_cmd`, `sealed_uplink`, `identity_announce`, `encrypted_data`) are carried as base64-encoded opaque blobs in the standard PapaDuck MQTT message format; the gateway forwards them without attempting to decrypt anything. The one thing to confirm is that operator downlink commands sent to the `topics.subscribe` (`hub/command`) topic use `"topic":8` for `encrypted_cmd`, with `target`/`message` base64-encoded (see `mqtt_message_arrived()` in `clusterduckd.c`, which validates the base64 charset before decoding).

### 3. Field Provisioning: Setting the OpenDMS Public Key
Each Duck decrypts operator-initiated downlink commands using this deployment's OpenDMS instance's static X25519 public key, `OPENDMS_STATIC_PUBLIC_KEY` (see `src/security/OpenDmsConfig.h`). This key is **not secret** -- only the matching private key held by OpenDMS must stay confidential -- so it can be set either at compile time or, without reflashing, over the USB serial console. Either way, it must be the *same* public key generated in step 1 (`DUCK_CRYPTO_PUBLIC_KEY`).

**Option A -- compile time:** edit **`src/security/OpenDmsConfig.cpp`** (in this `meshbeacon-firmware` repo), not the header. Find this line (currently ~line 207, inside `namespace opendmsconfig { ... }`), which ships with an all-zero placeholder:
```cpp
uint8_t OPENDMS_STATIC_PUBLIC_KEY[duckcrypto::PUBLIC_KEY_LENGTH] = {0};
```
Convert `DUCK_CRYPTO_PUBLIC_KEY` (base64, from step 1) to a C byte array:
```bash
python3 -c "import base64; print(', '.join(f'0x{b:02X}' for b in base64.b64decode('<DUCK_CRYPTO_PUBLIC_KEY value>')))"
```
and replace only the `{0}` with that comma-separated list of 32 `0x..` bytes (keep the rest of the declaration as-is), e.g. `= {0x1A, 0x2B, ..., 0xFF};`. Then rebuild and reflash -- this is a compile-time constant, so it only takes effect in a fresh flash, and it's shared source, so it applies to whichever `platformio.ini` environment/board you build (there's no per-board override).

**Note:** if a device has already been field-provisioned via serial (Option B below), `begin()` loads that stored key from flash over this compile-time value every boot (see `loadFromStorage()`), so editing this array alone won't change the key on an already-provisioned device -- send `AT+OPENDMSKEY+RESET` first, or erase/reflash flash entirely.

**Option B -- field provisioning over serial (no reflash):** Connect to the device's serial console (e.g. `platformio device monitor`, or the Arduino Serial Monitor) at the baud rate configured by `setupSerial()` (115200 by default), then send one of the following plaintext commands, terminated with a newline. These take **hex**, not base64, so convert first:
```bash
python3 -c "import base64; print(base64.b64decode('<DUCK_CRYPTO_PUBLIC_KEY value>').hex())"
```

- `AT+OPENDMSKEY=<64 hex chars>` -- provisions the device with a new OpenDMS public key (32 bytes, hex-encoded). Only succeeds if the device does not already have a key configured; if it does, send `AT+OPENDMSKEY+RESET` first.
- `AT+OPENDMSKEY+RESET` -- erases the currently configured key, allowing a new `AT+OPENDMSKEY=` write to be accepted.
- `AT+OPENDMSKEY?` -- prints whether a key is currently configured, and its value in hex if so.

No authentication is required to use these commands -- this is intentional, since the value being provisioned is public. The device persists the key to flash (LittleFS on nRF52, EEPROM on ESP32) so it survives reboots without needing to be re-sent.

### 4. Duck-to-Duck Session Encryption (opt-in)

Once a Duck knows a peer's public key, it can send it end-to-end encrypted traffic. This is entirely opt-in -- existing `sendData()` calls are unaffected unless you explicitly call these:

1. `announceIdentity(targetDevice = BROADCAST_DUID)` -- broadcasts this Duck's own public key so peers can learn it (`identity_announce` topic). TOFU: the receiving Duck trusts the first announcement it sees for a given SDUID, with no signature/verification.
2. Once a peer's identity has been learned (via `announceIdentity` or `sendEncryptedData`/`getPeerIdentity`), call `sendEncryptedData(topic, data, targetDevice)` to send AEAD-encrypted application data to it. Returns `DUCK_ERR_CRYPTO_ECDH_FAILED` if the peer's public key isn't known yet -- call `announceIdentity()` first.

### 5. Duck-to-OpenDMS Sealed Uplink (opt-in)

For one-way uplink traffic to OpenDMS (e.g. SOS/alert payloads), call `sendSealedData(topic, data, targetDevice = PAPADUCK_DUID)`. This seals `data` against OpenDMS's pinned static public key using a fresh, one-time ephemeral keypair per call (see `DuckCrypto::sealToStatic`). Returns `DUCK_ERR_CRYPTO_ECDH_FAILED` if `OpenDmsConfig::isConfigured()` is false (step 3 not done yet).

### 6. OpenDMS-to-Duck Encrypted Downlink Commands

OpenDMS sends operator commands end-to-end encrypted using the Duck's public key (learned server-side via whatever mesh discovery/registration flow your OpenDMS deployment uses) and its own static private key -- ECDH is symmetric, so the Duck decrypts with `decryptFromPeer(OPENDMS_STATIC_PUBLIC_KEY, ...)`. On the wire this travels as an MQTT `hub/command` message with `"topic":8` (`encrypted_cmd`) and base64-encoded `target`/`message` fields, exactly like any other gateway command (see step 2).

### Security Notes and Caveats

- **TOFU, not PKI.** Peer identities (Duck-to-Duck) and OpenDMS's public key (compile-time/serial) are both trust-on-first-use with no signature chain. Anyone able to inject a competing announcement/key before the real one arrives can impersonate a peer.
- **No forward secrecy** for session mode (`encryptWithPeer`/`decryptFromPeer`) or downlink (`encrypted_cmd`) -- both use static-static ECDH, so compromise of either long-term private key retroactively exposes all past traffic between that pair. Sealed uplink (`sealToStatic`) has one-sided forward secrecy (fresh ephemeral key per message) but only protects against compromise of the Duck's key, not OpenDMS's.
- **Key rotation is manual and not yet built out** -- there is no rotation opcode or automated re-keying flow; rotating OpenDMS's static keypair currently means re-provisioning every fielded device (step 3) with the new public key.
- A design-rationale document (`crypto-design.tex`) is referenced throughout the DuckCrypto code comments. It is now maintained outside this repo (not checked in under `docs/`) -- check with your team for the current shared location before relying on an absolute path from any one contributor's machine.
