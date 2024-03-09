// test/test_CdpPacket/test_CdpPacket.cpp
#include <unity.h>
#include "CdpPacket.h"

void test_CdpPacket_constructor(void) {
    CdpPacket packet;
    TEST_ASSERT_EQUAL(0, packet.topic);
    TEST_ASSERT_EQUAL(0, packet.hopCount);
    TEST_ASSERT_EQUAL(0, packet.dcrc);
    TEST_ASSERT_EQUAL(DuckType::UNKNOWN, packet.duckType);
    TEST_ASSERT_TRUE(packet.sduid.empty());
    TEST_ASSERT_TRUE(packet.dduid.empty());
    TEST_ASSERT_TRUE(packet.muid.empty());
    TEST_ASSERT_TRUE(packet.data.empty());
    TEST_ASSERT_TRUE(packet.path.empty());
}

void test_CdpPacket_constructor_from_buffer(void) {
    // Create a byte buffer with a known packet format
    std::vector<byte> buffer = {
        // SDUID
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        // DDUID
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
        // MUID
        0x11, 0x12, 0x13, 0x14,
        // Topic
        0x15,
        // DuckType
        0x16,
        // HopCount
        0x17,
        // Data CRC
        0x18, 0x19, 0x1A, 0x1B,
        // Data
        0x1C, 0x1D, 0x1E, 0x1F
    };

    // Create a CdpPacket object from the byte buffer
    CdpPacket packet(buffer);

    // Verify that the CdpPacket object was initialized correctly
    TEST_ASSERT_EQUAL_HEX8_ARRAY(buffer.data(), packet.sduid.data(), 8);    
    TEST_ASSERT_EQUAL_HEX8_ARRAY(buffer.data() + 8, packet.dduid.data(), 8);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(buffer.data() + 16, packet.muid.data(), 4);
    TEST_ASSERT_EQUAL_HEX8(0x15, packet.topic);
    TEST_ASSERT_EQUAL_HEX8(0x16, packet.duckType);
    TEST_ASSERT_EQUAL_HEX8(0x17, packet.hopCount);
    TEST_ASSERT_EQUAL_HEX32(0x18191A1B, packet.dcrc);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(buffer.data() + 27, packet.data.data(), 4);
}

void test_CdpPacket_constructor_from_buffer_variant(void) {
    // Create a byte buffer with a known packet format
    std::vector<byte> buffer = {
        // SDUID
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        // DDUID
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // MUID
        0x11, 0x12, 0x13, 0x14,
        // Topic
        0x01,
        // DuckType
        0x03,
        // HopCount
        0x01,
        // Data CRC
        0x18, 0x19, 0x1A, 0x1B,
        // Data
        0x1C, 0x1D, 0x1E, 0x1F
    };

    // Create a CdpPacket object from the byte buffer
    CdpPacket packet(buffer);

    // Verify that the CdpPacket object was initialized correctly
    TEST_ASSERT_EQUAL_HEX8_ARRAY(buffer.data(), packet.sduid.data(), 8);    
    TEST_ASSERT_EQUAL_HEX8_ARRAY(buffer.data() + 8, packet.dduid.data(), 8);
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_EQUAL(0, packet.dduid.data()[i]);
    }
    
    TEST_ASSERT_EQUAL_HEX8_ARRAY(buffer.data() + 16, packet.muid.data(), 4);
    TEST_ASSERT_EQUAL_HEX8(0x01, packet.topic);
    TEST_ASSERT_EQUAL_HEX8(0x03, packet.duckType);
    TEST_ASSERT_EQUAL_HEX8(0x01, packet.hopCount);
    TEST_ASSERT_EQUAL_HEX32(0x18191A1B, packet.dcrc);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(buffer.data() + 27, packet.data.data(), 4);
}


void test_CdpPacket_reset(void) {
    CdpPacket packet;
    packet.reset();
    TEST_ASSERT_EQUAL(0, packet.topic);
    TEST_ASSERT_EQUAL(0, packet.hopCount);
    TEST_ASSERT_EQUAL(0, packet.dcrc);
    TEST_ASSERT_EQUAL(DuckType::UNKNOWN, packet.duckType);
    TEST_ASSERT_TRUE(packet.sduid.empty());
    TEST_ASSERT_TRUE(packet.dduid.empty());
    TEST_ASSERT_TRUE(packet.muid.empty());
    TEST_ASSERT_TRUE(packet.data.empty());
    TEST_ASSERT_TRUE(packet.path.empty());
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_CdpPacket_constructor);
    RUN_TEST(test_CdpPacket_reset);
    RUN_TEST(test_CdpPacket_constructor_from_buffer);
    RUN_TEST(test_CdpPacket_constructor_from_buffer_variant);
    UNITY_END();
}

void loop() {
}