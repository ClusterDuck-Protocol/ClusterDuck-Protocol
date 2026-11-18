# Routing host tests

Runs the real CDP routing code on your computer, with a fake radio and clock in place of the hardware. The script builds three times, each with the address and undefined-behaviour sanitizers: remote commands off (the default), time sync only (`CDPCFG_REMOTE_TIME_SYNC=1`), and all remote commands on (`CDPCFG_REMOTE_COMMANDS=1`).

## Run

The tests reuse the ArduinoJson and CRC32 sources that PlatformIO installs, so build any example once first:

```sh
EXAMPLE_DIR=Basic-Ducks/MamaDuck pio run -e local_lilygo_t_beam_sx1262
bash tests/routing-host/run.sh
```

To use other copies of those libraries, set `ARDUINOJSON_SRC` and `CRC32_SRC` to their `src` folders. Set `CXX` to use a compiler other than `clang++`. Each of the three runs ends with `PASS`.

## What's covered

- **Route table:** a measured link isn't replaced by an indirect one, routes expire after `CDPCFG_ROUTE_TTL_MS`, and expiry survives the `millis()` rollover.
- **Signal health:** 0, 1, 2, 3, 6, 7 and 40 neighbours. Each payload fits in 228 bytes and reports truncation.
- **Malformed input:** bad route JSON, duplicate path entries, payloads over 228 bytes and hop counts over the limit are rejected.
- **Four-duck complete mesh:** 10 quiet minutes stay within 6 route requests and 18 replies. Every diagnostics page fits in 228 bytes even at maximum counter values. The PapaDuck receives repeated copies, and a full TX queue reports failure.
- **Three-link chain:** discovery reaches the PapaDuck, and replies keep their identity across hops. The duck recovers after losing the gateway, and after a relay reboots.
- **Startup pacing:** sends are queued and paced, then delivered.
- **5.0 compatibility:** PapaDuck data uses the `00000000` header address, route JSON uses `"11111111"`, and packets addressed to `11111111` are accepted as PapaDuck traffic.
- **Time sync (`cmd_time`):** a PapaDuck's `sendTimeSync()` sets each MamaDuck's clock once, although every MamaDuck hears three copies, and a repeated copy never moves the clock back. Packets from a MamaDuck, with an epoch before `CDPCFG_TIME_MIN_EPOCH`, without an integer epoch or with broken JSON are rejected but still relayed. A command addressed to one duck is applied there and not relayed; other ducks pass it on. DuckLinks apply it too. With time sync off nothing is applied, and MamaDucks still relay every copy. The clock is a fake, so the tests never touch the computer's clock.
- **TX power (`cmd_tx`):** stored only when `CDPCFG_REMOTE_TX_POWER` is on, relayed either way, and a command addressed to another duck is passed on rather than applied. The fake radio takes 2 to 17 or 20 dBm, like an SX1276, or -9 to 22 dBm when set to SX1262, and its setup fails on any other value. With the setting on, MamaDucks and DuckLinks refuse 18, 19 and 21 dBm on an SX1276 and store 17, 20 and 22 dBm where the radio takes them. A MamaDuck also applies a command addressed to it; a DuckLink ignores one. A broadcast the radio refuses is still relayed; 13 and 23 dBm, outside 14 to 22, are not stored, and a MamaDuck doesn't relay them. The power never changes before the next boot. At boot, a MamaDuck, DuckLink or PapaDuck removes a stored value outside 14 to 22 dBm (10, -5 or 30) or one the radio refuses (18 or 22 on an SX1276), uses the default and sets up. If the check passes a value but setup still fails, setup runs once more with the default and the value is removed. A radio fault leaves the value alone. With the setting off, stored values are ignored and left alone.
- **Duplicate filter:** each swap clears every word of the half it reuses, and a MUID is still seen after the swap that follows it. The two hash bits are independent and collisions depend on the seeds. Over 80,000 new MUIDs (800 swaps) fewer than 0.1% are reported as already seen; the test prints the count.
- **Sleep hooks:** `txIdle()` is false while a packet is queued, the radio is transmitting or a received packet is unread, and true once the queues drain. After `prepareForSleep()` the radio is asleep and `sendData()` fails.
- **`toString()` for logs:** arrays, vectors, strings and byte pointers (the `Rx packet` line) keep printable ASCII and write every other byte as two hex digits, so `0x0A` becomes `0A`. DUIDs in the routing table and route JSON stay the raw eight bytes.

These tests don't model RF loss, collisions, power or real timing. Hardware testing is still needed before a release.
