/**
 * @file DuckPayloads.cpp
 * @brief Implementation of the protobuf payload encode/decode helpers.
 *
 * @copyright
 */

#include "DuckPayloads.h"

#include "nanopb/pb_decode.h"
#include "nanopb/pb_encode.h"

namespace duckpayload {

namespace {

/**
 * @brief Shared encode helper: writes the format marker byte followed by
 * the nanopb-encoded message.
 */
template <typename T>
std::vector<uint8_t> encodeWithMarker(const pb_msgdesc_t *fields, const T &msg,
                                       size_t maxEncodedSize) {
  std::vector<uint8_t> buf(1 + maxEncodedSize);
  buf[0] = static_cast<uint8_t>(Format::kProtobuf);

  pb_ostream_t stream = pb_ostream_from_buffer(buf.data() + 1, buf.size() - 1);
  if (!pb_encode(&stream, fields, &msg)) {
    return {};
  }

  buf.resize(1 + stream.bytes_written);
  return buf;
}

/**
 * @brief Shared decode helper: verifies the format marker byte, then
 * nanopb-decodes the remainder of the buffer.
 */
template <typename T>
bool decodeWithMarker(const pb_msgdesc_t *fields, const uint8_t *data,
                       size_t length, T &out, const T &zeroInit) {
  if (data == nullptr || length < 1 ||
      data[0] != static_cast<uint8_t>(Format::kProtobuf)) {
    return false;
  }

  out = zeroInit;
  pb_istream_t stream = pb_istream_from_buffer(data + 1, length - 1);
  return pb_decode(&stream, fields, &out);
}

} // namespace

bool isProtobuf(const uint8_t *data, size_t length) {
  return data != nullptr && length > 0 &&
         data[0] == static_cast<uint8_t>(Format::kProtobuf);
}

std::vector<uint8_t> encodeGps(const duckcdp_GpsReading &reading) {
  return encodeWithMarker(&duckcdp_GpsReading_msg, reading,
                           duckcdp_GpsReading_size);
}

bool decodeGps(const uint8_t *data, size_t length, duckcdp_GpsReading &out) {
  return decodeWithMarker(&duckcdp_GpsReading_msg, data, length, out,
                           duckcdp_GpsReading_init_zero);
}

std::vector<uint8_t> encodeSos(const duckcdp_SosAlert &alert) {
  return encodeWithMarker(&duckcdp_SosAlert_msg, alert, duckcdp_SosAlert_size);
}

bool decodeSos(const uint8_t *data, size_t length, duckcdp_SosAlert &out) {
  return decodeWithMarker(&duckcdp_SosAlert_msg, data, length, out,
                           duckcdp_SosAlert_init_zero);
}

std::vector<uint8_t> encodeHealth(const duckcdp_HealthStatus &status) {
  return encodeWithMarker(&duckcdp_HealthStatus_msg, status,
                           duckcdp_HealthStatus_size);
}

bool decodeHealth(const uint8_t *data, size_t length,
                   duckcdp_HealthStatus &out) {
  return decodeWithMarker(&duckcdp_HealthStatus_msg, data, length, out,
                           duckcdp_HealthStatus_init_zero);
}

std::vector<uint8_t> encodeMTalk(const duckcdp_MTalk &msg) {
  return encodeWithMarker(&duckcdp_MTalk_msg, msg, duckcdp_MTalk_size);
}

bool decodeMTalk(const uint8_t *data, size_t length, duckcdp_MTalk &out) {
  duckcdp_MTalk zeroInit = duckcdp_MTalk_init_zero;
  return decodeWithMarker(&duckcdp_MTalk_msg, data, length, out, zeroInit);
}

std::vector<uint8_t> encodeStatusReportSos(const duckcdp_SosAlert &alert) {
  duckcdp_StatusReport report = duckcdp_StatusReport_init_zero;
  report.which_report = duckcdp_StatusReport_sos_tag;
  report.report.sos = alert;
  return encodeWithMarker(&duckcdp_StatusReport_msg, report,
                           duckcdp_StatusReport_size);
}

std::vector<uint8_t> encodeStatusReportMsg(const duckcdp_StatusMsg &msg) {
  duckcdp_StatusReport report = duckcdp_StatusReport_init_zero;
  report.which_report = duckcdp_StatusReport_msg_tag;
  report.report.msg = msg;
  return encodeWithMarker(&duckcdp_StatusReport_msg, report,
                           duckcdp_StatusReport_size);
}

std::vector<uint8_t> encodeOpText(const duckcdp_OpText &text) {
  return encodeWithMarker(&duckcdp_OpText_msg, text, duckcdp_OpText_size);
}

bool decodeOpText(const uint8_t *data, size_t length, duckcdp_OpText &out) {
  duckcdp_OpText zeroInit = duckcdp_OpText_init_zero;
  return decodeWithMarker(&duckcdp_OpText_msg, data, length, out, zeroInit);
}

} // namespace duckpayload
