#include <Arduino.h>
#include <unity.h>
#include <string.h>

#include "payloads/DuckPayloads.h"

void setUp(void) {}
void tearDown(void) {}

// A GpsReading with a fix round-trips through encode/decode with all fields intact.
void test_gps_reading_with_fix_round_trips(void) {
  duckcdp_GpsReading in = duckcdp_GpsReading_init_zero;
  in.has_fix = true;
  in.source = duckcdp_GpsSource_GPS_SOURCE_DEVICE;
  in.no_fix_reason = duckcdp_GpsNoFixReason_GPS_REASON_NONE;
  in.lat_e7 = 37774900;   // 3.77749 deg * 1e7
  in.lng_e7 = -122419400; // -12.24194 deg * 1e7
  in.alt_m = 30;
  in.spd_dkmh = 125; // 12.5 km/h
  in.hdg_deg = 270;
  in.sats = 9;
  in.batt_pct = 87;

  std::vector<uint8_t> encoded = duckpayload::encodeGps(in);
  TEST_ASSERT_FALSE(encoded.empty());
  TEST_ASSERT_TRUE(duckpayload::isProtobuf(encoded.data(), encoded.size()));

  duckcdp_GpsReading out;
  TEST_ASSERT_TRUE(duckpayload::decodeGps(encoded.data(), encoded.size(), out));
  TEST_ASSERT_TRUE(out.has_fix);
  TEST_ASSERT_EQUAL_INT(duckcdp_GpsSource_GPS_SOURCE_DEVICE, out.source);
  TEST_ASSERT_EQUAL_INT32(37774900, out.lat_e7);
  TEST_ASSERT_EQUAL_INT32(-122419400, out.lng_e7);
  TEST_ASSERT_EQUAL_INT32(30, out.alt_m);
  TEST_ASSERT_EQUAL_UINT32(125, out.spd_dkmh);
  TEST_ASSERT_EQUAL_UINT32(270, out.hdg_deg);
  TEST_ASSERT_EQUAL_UINT32(9, out.sats);
  TEST_ASSERT_EQUAL_UINT32(87, out.batt_pct);
}

// A no-fix GpsReading (e.g. phone GPS timed out) still round-trips correctly.
void test_gps_reading_without_fix_round_trips(void) {
  duckcdp_GpsReading in = duckcdp_GpsReading_init_zero;
  in.has_fix = false;
  in.source = duckcdp_GpsSource_GPS_SOURCE_PHONE;
  in.no_fix_reason = duckcdp_GpsNoFixReason_GPS_REASON_NO_RESPONSE;
  in.batt_pct = 42;

  std::vector<uint8_t> encoded = duckpayload::encodeGps(in);
  TEST_ASSERT_FALSE(encoded.empty());

  duckcdp_GpsReading out;
  TEST_ASSERT_TRUE(duckpayload::decodeGps(encoded.data(), encoded.size(), out));
  TEST_ASSERT_FALSE(out.has_fix);
  TEST_ASSERT_EQUAL_INT(duckcdp_GpsSource_GPS_SOURCE_PHONE, out.source);
  TEST_ASSERT_EQUAL_INT(duckcdp_GpsNoFixReason_GPS_REASON_NO_RESPONSE, out.no_fix_reason);
  TEST_ASSERT_EQUAL_UINT32(42, out.batt_pct);
}

// SosAlert triggered by the hardware button, with phone-relayed GPS, round-trips.
void test_sos_alert_round_trips(void) {
  duckcdp_SosAlert in = duckcdp_SosAlert_init_zero;
  in.origin = duckcdp_SosOrigin_SOS_ORIGIN_DEVICE;
  in.gps_source = duckcdp_GpsSource_GPS_SOURCE_PHONE;
  in.has_gps = true;
  in.lat_e7 = 1234567;
  in.lng_e7 = -7654321;
  in.alt_m = 5;
  in.spd_dkmh = 0;
  in.hdg_deg = 0;
  in.batt_pct = 63;

  std::vector<uint8_t> encoded = duckpayload::encodeSos(in);
  TEST_ASSERT_FALSE(encoded.empty());

  duckcdp_SosAlert out;
  TEST_ASSERT_TRUE(duckpayload::decodeSos(encoded.data(), encoded.size(), out));
  TEST_ASSERT_EQUAL_INT(duckcdp_SosOrigin_SOS_ORIGIN_DEVICE, out.origin);
  TEST_ASSERT_EQUAL_INT(duckcdp_GpsSource_GPS_SOURCE_PHONE, out.gps_source);
  TEST_ASSERT_TRUE(out.has_gps);
  TEST_ASSERT_EQUAL_INT32(1234567, out.lat_e7);
  TEST_ASSERT_EQUAL_INT32(-7654321, out.lng_e7);
  TEST_ASSERT_EQUAL_UINT32(63, out.batt_pct);
}

// HealthStatus round-trips.
void test_health_status_round_trips(void) {
  duckcdp_HealthStatus in = duckcdp_HealthStatus_init_zero;
  in.counter = 4242;
  in.free_memory = 51200;

  std::vector<uint8_t> encoded = duckpayload::encodeHealth(in);
  TEST_ASSERT_FALSE(encoded.empty());

  duckcdp_HealthStatus out;
  TEST_ASSERT_TRUE(duckpayload::decodeHealth(encoded.data(), encoded.size(), out));
  TEST_ASSERT_EQUAL_UINT32(4242, out.counter);
  TEST_ASSERT_EQUAL_INT32(51200, out.free_memory);
}

// A legacy plain-text payload (format marker 0x00, or no marker at all) must
// be rejected by the protobuf decoders rather than misinterpreted.
void test_legacy_text_payload_is_rejected(void) {
  const char* legacy = "GPS,SRC:PHONE,LAT:1.234,LNG:5.678,BATT:80";
  duckcdp_GpsReading out;
  TEST_ASSERT_FALSE(duckpayload::decodeGps(reinterpret_cast<const uint8_t*>(legacy),
                                            strlen(legacy), out));
  TEST_ASSERT_FALSE(duckpayload::isProtobuf(reinterpret_cast<const uint8_t*>(legacy),
                                             strlen(legacy)));
}

void setup() {
  delay(2000); // allow board / serial monitor to settle before running tests

  UNITY_BEGIN();
  RUN_TEST(test_gps_reading_with_fix_round_trips);
  RUN_TEST(test_gps_reading_without_fix_round_trips);
  RUN_TEST(test_sos_alert_round_trips);
  RUN_TEST(test_health_status_round_trips);
  RUN_TEST(test_legacy_text_payload_is_rejected);
  UNITY_END();
}

void loop() {}
