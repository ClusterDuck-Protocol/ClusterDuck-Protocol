/**
 * @file cdpcfg.h
 * @brief CDP central compile-time configuration file.
 *
 * @version
 * @date 2020-09-16
 *
 * @copyright
 *
 */

#ifndef CDPCFG_H
#define CDPCFG_H

#ifdef CDP_EXTERNAL_BOARD
#include "cdp_external_board.h"
#else
#include "boards/heltec_wifi_lora_32_V3.h"
#include "boards/heltec_wifi_lora_32_V2.h"
#include "boards/lilygo_t_beam_sx1262.h"
#include "boards/ttgo_t_beam_v1_sx1276.h"
#include "boards/lilygo_t_beam_supreme_sx1262.h"
#include "boards/lilygo_sim7000g_lora.h"
#endif

// version definitions
#define CDP_VERSION_MAJOR  5
#define CDP_VERSION_MINOR  5
#define CDP_VERSION_PATCH  0
#ifndef CDPCFG_BUILD_REVISION
#define CDPCFG_BUILD_REVISION "unversioned"
#endif

#define CDP_VERSION ((((CDP_VERSION_MAJOR) << 16) | ((CDP_VERSION_MINOR) << 8) | (CDP_VERSION_PATCH)))

#ifdef CDPCFG_RADIO_SX1262
  #define CDPCFG_LORA_CLASS SX1262
#else
  #define CDPCFG_LORA_CLASS SX1276
#endif

/**
 * @brief Non board specific configuration
 */

//Default Username and Password
#define CDPCFG_UPDATE_USERNAME "user"
#define CDPCFG_UPDATE_PASSWORD "pass"

// Remote duck commands. Commands are not authenticated, so any node in radio
// range can send them; they are off by default. With a command off, ducks still
// relay its packets for other ducks but ignore them themselves.
//   CDPCFG_REMOTE_TIME_SYNC: act on topics::cmd_time (set the clock).
//   CDPCFG_REMOTE_TX_POWER:  act on topics::cmd_tx (store a TX power for the
//                            next boot, if the radio takes it).
// Both default to CDPCFG_REMOTE_COMMANDS, so -DCDPCFG_REMOTE_COMMANDS=1 still
// turns both on. Turn on time sync alone with -DCDPCFG_REMOTE_TIME_SYNC=1.
#ifndef CDPCFG_REMOTE_COMMANDS
#define CDPCFG_REMOTE_COMMANDS 0
#endif
#ifndef CDPCFG_REMOTE_TIME_SYNC
#define CDPCFG_REMOTE_TIME_SYNC CDPCFG_REMOTE_COMMANDS
#endif
#ifndef CDPCFG_REMOTE_TX_POWER
#define CDPCFG_REMOTE_TX_POWER CDPCFG_REMOTE_COMMANDS
#endif

// Oldest UTC time, in seconds since 1970, that a duck treats as valid
// (default 2026-01-01). A topics::cmd_time with an older epoch is rejected,
// and Duck::hasValidTime() stays false until the clock reaches it.
#ifndef CDPCFG_TIME_MIN_EPOCH
#define CDPCFG_TIME_MIN_EPOCH 1767225600UL
#endif

/// Serial Console Baud Rate
#define CDPCFG_SERIAL_BAUD 115200

// Packets each of the RX, data-TX and control-TX queues can hold.
#ifndef CDPCFG_MAX_QUEUE_SIZE
#define CDPCFG_MAX_QUEUE_SIZE 10
#endif
// Minimum time between two transmissions, in milliseconds.
#ifndef CDPCFG_MAX_PACKET_SEND_RATE
#define CDPCFG_MAX_PACKET_SEND_RATE 3000
#endif

// Bounded RAM for directly observed peers and destination alternatives.
#define CDPCFG_MAX_DIRECT_NEIGHBORS 32
#define CDPCFG_MAX_ROUTE_DESTINATIONS 64

// How often a duck sends its health summary (topics::health) and logs one
// detailed diagnostics page to serial. The first heartbeat also sends one
// signal-health snapshot, so a new installation shows up without waiting a
// full signal interval.
#ifndef CDPCFG_HEALTH_INTERVAL_MS
#define CDPCFG_HEALTH_INTERVAL_MS (1000UL * 60UL * 15UL)   // 15 minutes
#endif

// How often a duck reports its direct neighbours' signal (topics::sig).
#ifndef CDPCFG_SIGNAL_INTERVAL_MS
#define CDPCFG_SIGNAL_INTERVAL_MS (1000UL * 60UL * 30UL)   // 30 minutes
#endif

// How long a routing table entry stays valid without being refreshed.
// Entries are refreshed when a packet is heard directly from that node
// (hop count 0) and by the periodic route keepalive below, so this must stay
// comfortably above CDPCFG_ROUTE_KEEPALIVE_MS or routes age out between
// keepalives and every send falls back to an RREQ.
#ifndef CDPCFG_ROUTE_TTL_MS
#define CDPCFG_ROUTE_TTL_MS (1000UL * 60UL * 30UL)   // 30 minutes
#endif

// Most neighbours reported in one signal-health payload. The data section is
// MAX_DATA_LENGTH (228) bytes and each entry costs roughly 26, so 6 leaves margin.
// Entries are sorted best-signal-first and any truncation is logged.
#ifndef CDPCFG_SIGNAL_MAX_NEIGHBORS
#define CDPCFG_SIGNAL_MAX_NEIGHBORS 6
#endif

// How often a duck broadcasts a one-hop neighbour keepalive. Missing gateway
// routes are separately discovered by a bounded, targeted RREQ.
// Neighbours that hear it refresh our entry in their table and answer with an
// RREP, which refreshes theirs in ours. Keep it well under CDPCFG_ROUTE_TTL_MS.
#ifndef CDPCFG_ROUTE_KEEPALIVE_MS
#define CDPCFG_ROUTE_KEEPALIVE_MS (1000UL * 60UL * 10UL)   // 10 minutes
#endif

// Access point IP adress

#define CDPCFG_AP_IP1 192
#define CDPCFG_AP_IP2 168
#define CDPCFG_AP_IP3 1
#define CDPCFG_AP_IP4 1

/** @brief Asyncwebserver Port */
#define CDPCFG_WEB_PORT 80

/// Frequency Range. Set for US Region 915.0Mhz
#define CDPCFG_RF_LORA_FREQ 915.0f
#define CDPCFG_RF_LORA_FREQ_HZ 915000000
/// Bandwidth. Default is 125Mhz
#define CDPCFG_RF_LORA_BW 125.0f
/// Spread Factor
#define CDPCFG_RF_LORA_SF 7
/// Transmit Power (dBm). Deployments that need more can override it at build time.
#ifndef CDPCFG_RF_LORA_TXPOW
#define CDPCFG_RF_LORA_TXPOW 14
#endif
/// Antenna Gain correction
#define CDPCFG_RF_LORA_GAIN 0
/// LoRa sync word. Ducks only hear radios using the same value, so separate
/// deployments in range can pick different values to keep their traffic apart.
/// This is not security: anyone who sets the same value hears and joins the mesh.
/// 0x12 is the standard private (non-LoRaWAN) value; avoid 0x34, LoRaWAN's public one.
#ifndef CDPCFG_RF_LORA_SYNC_WORD
#define CDPCFG_RF_LORA_SYNC_WORD 0x12
#endif

/// CDP message buffer max length
#define CDPCFG_CDP_BUFSIZE 256
/// CDP UUID generator max length
#define CDPCFG_UUID_LEN 8

/// CDP chat circular buffer size
#define CDPCFG_CDP_CHATBUF_SIZE 15

/// CDP ALIVE timer duration in milliseconds
#define CDPCFG_MILLIS_ALIVE 1800000
/// CDP REBOOT timer duration in milliseconds
#define CDPCFG_MILLIS_REBOOT 43200000

/// CDP RGB Led RED Pin default value
#define CDPCFG_PIN_RGBLED_R 25
/// CDP RGB Led GREEN Pin default value
#define CDPCFG_PIN_RGBLED_G 4
/// CDP RGB Led BLUE Pin default value
#define CDPCFG_PIN_RGBLED_B 2

/// CDP Channel Frequencies
#define CDPCFG_RADIO_CHANNEL_1 CDPCFG_RF_LORA_FREQ
#define CDPCFG_RADIO_CHANNEL_2 914.0
#define CDPCFG_RADIO_CHANNEL_3 913.0
#define CDPCFG_RADIO_CHANNEL_4 912.0
#define CDPCFG_RADIO_CHANNEL_5 911.0
#define CDPCFG_RADIO_CHANNEL_6 910.0

// CDP Acceptable Signal Ranges
#define RSSI_MAX (-20.0f)
#define RSSI_MIN (-131.0f)
#define SNR_MAX 30.0f
#define SNR_MIN (-30.0f)

#ifndef CDPCFG_OLED_CLASS
#if defined(CDPCFG_OLED_NONE)
// do nothing
#elif defined(CDPCFG_OLED_64x32)
// smaller displays
#define CDPCFG_OLED_CLASS U8G2_SSD1306_64X32_NONAME_F_SW_I2C
#else
// classic default
#define CDPCFG_OLED_CLASS U8G2_SSD1306_128X64_NONAME_F_SW_I2C
#endif // CDPCFG_OLED_NONE
#endif // CDPCFG_OLED_CLASS

#endif // CDPCFG_H
