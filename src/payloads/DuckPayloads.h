/**
 * @file DuckPayloads.h
 * @brief Encode/decode helpers for protobuf-based CDP LoRa payloads.
 *
 * Wraps the nanopb-generated `duckcdp_GpsReading`, `duckcdp_SosAlert`,
 * `duckcdp_HealthStatus` and `duckcdp_MTalk` messages (see
 * duck_payloads.proto) so callers can work with plain `std::vector<uint8_t>`
 * buffers compatible with `Duck::sendData()`.
 *
 * Every buffer produced/consumed here starts with a one-byte format marker
 * (see `duckpayload::Format`) so a receiver can tell a new protobuf-encoded
 * payload apart from a legacy plain-text one during a mixed-version
 * rollout.
 *
 * @copyright
 */

#ifndef DUCKPAYLOADS_H_
#define DUCKPAYLOADS_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "generated/duck_payloads.pb.h"

namespace duckpayload {

/**
 * @brief First byte of every CdpPacket DATA payload produced/consumed by
 * this module.
 */
enum class Format : uint8_t {
  kLegacyText = 0x00, ///< pre-existing plain-text AT-command-style payload
  kProtobuf = 0x01,   ///< payload encoded with duck_payloads.proto messages
};

/**
 * @brief Returns true if the payload starts with the protobuf format marker.
 * @param data pointer to the raw CdpPacket DATA bytes
 * @param length number of bytes available at `data`
 */
bool isProtobuf(const uint8_t *data, size_t length);

/**
 * @brief Encode a GpsReading message for the `gps` topic.
 * @return the marker byte followed by the encoded message, or an empty
 * vector if encoding failed.
 */
std::vector<uint8_t> encodeGps(const duckcdp_GpsReading &reading);

/**
 * @brief Decode a GpsReading message previously produced by encodeGps().
 * @param data pointer to the raw CdpPacket DATA bytes (including the marker byte)
 * @param length number of bytes available at `data`
 * @param out destination for the decoded message
 * @return true if `data` was a valid protobuf GpsReading payload
 */
bool decodeGps(const uint8_t *data, size_t length, duckcdp_GpsReading &out);

/**
 * @brief Encode a SosAlert message for the `alert` topic.
 * @return the marker byte followed by the encoded message, or an empty
 * vector if encoding failed.
 */
std::vector<uint8_t> encodeSos(const duckcdp_SosAlert &alert);

/**
 * @brief Decode a SosAlert message previously produced by encodeSos().
 * @param data pointer to the raw CdpPacket DATA bytes (including the marker byte)
 * @param length number of bytes available at `data`
 * @param out destination for the decoded message
 * @return true if `data` was a valid protobuf SosAlert payload
 */
bool decodeSos(const uint8_t *data, size_t length, duckcdp_SosAlert &out);

/**
 * @brief Encode a HealthStatus message for the `health` topic.
 * @return the marker byte followed by the encoded message, or an empty
 * vector if encoding failed.
 */
std::vector<uint8_t> encodeHealth(const duckcdp_HealthStatus &status);

/**
 * @brief Decode a HealthStatus message previously produced by encodeHealth().
 * @param data pointer to the raw CdpPacket DATA bytes (including the marker byte)
 * @param length number of bytes available at `data`
 * @param out destination for the decoded message
 * @return true if `data` was a valid protobuf HealthStatus payload
 */
bool decodeHealth(const uint8_t *data, size_t length, duckcdp_HealthStatus &out);

/**
 * @brief Encode an MTalk message for topic 26 (MamaDuck-to-MamaDuck chat).
 * @return the marker byte followed by the encoded message, or an empty
 * vector if encoding failed.
 */
std::vector<uint8_t> encodeMTalk(const duckcdp_MTalk &msg);

/**
 * @brief Decode an MTalk message previously produced by encodeMTalk().
 * @param data pointer to the raw CdpPacket DATA bytes (including the marker byte)
 * @param length number of bytes available at `data`
 * @param out destination for the decoded message
 * @return true if `data` was a valid protobuf MTalk payload
 */
bool decodeMTalk(const uint8_t *data, size_t length, duckcdp_MTalk &out);

/**
 * @brief Encode a StatusReport wrapping a SosAlert for the `status` topic
 * (phone-triggered SOS; see handleSOS in the example sketches).
 * @return the marker byte followed by the encoded message, or an empty
 * vector if encoding failed.
 */
std::vector<uint8_t> encodeStatusReportSos(const duckcdp_SosAlert &alert);

/**
 * @brief Encode a StatusReport wrapping a StatusMsg for the `status` topic
 * (phone-composed message, or the hardware "Roger" acknowledgement).
 * @return the marker byte followed by the encoded message, or an empty
 * vector if encoding failed.
 */
std::vector<uint8_t> encodeStatusReportMsg(const duckcdp_StatusMsg &msg);

/**
 * @brief Encode an OpText message for the operator/mesh text topics
 * (22/23/24/25).
 * @return the marker byte followed by the encoded message, or an empty
 * vector if encoding failed.
 */
std::vector<uint8_t> encodeOpText(const duckcdp_OpText &text);

/**
 * @brief Decode an OpText message previously produced by encodeOpText().
 * @param data pointer to the raw CdpPacket DATA bytes (including the marker byte)
 * @param length number of bytes available at `data`
 * @param out destination for the decoded message
 * @return true if `data` was a valid protobuf OpText payload
 */
bool decodeOpText(const uint8_t *data, size_t length, duckcdp_OpText &out);

} // namespace duckpayload

#endif // DUCKPAYLOADS_H_
