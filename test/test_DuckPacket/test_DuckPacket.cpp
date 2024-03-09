#include <unity.h>
#include <DuckPacket.h>
#include <bloomfilter.h>

// Testing public methods only 

void test_DuckPacket_constructor_default(void) {
    DuckPacket dp;
    TEST_ASSERT_EQUAL(0, dp.getDeviceId().size());
}

void test_DuckPacket_constructor_with_duid(void) {
    std::vector<byte> duid = {
        0x50, 0x61, 0x70, 0x61,
        0x44, 0x75, 0x63, 0x6B
    };
    DuckPacket dp(duid);
    TEST_ASSERT_EQUAL(8, dp.getDeviceId().size());
    TEST_ASSERT_EQUAL(0x50, dp.getDeviceId()[0]);
    TEST_ASSERT_EQUAL(0x61, dp.getDeviceId()[1]);
    TEST_ASSERT_EQUAL(0x70, dp.getDeviceId()[2]);
    TEST_ASSERT_EQUAL(0x61, dp.getDeviceId()[3]);
    TEST_ASSERT_EQUAL(0x44, dp.getDeviceId()[4]);
    TEST_ASSERT_EQUAL(0x75, dp.getDeviceId()[5]);
    TEST_ASSERT_EQUAL(0x63, dp.getDeviceId()[6]);
    TEST_ASSERT_EQUAL(0x6B, dp.getDeviceId()[7]);
}

void test_DuckPacket_setDeviceId(void) {
    std::vector<byte> duid = {
        0x50, 0x61, 0x70, 0x61,
        0x44, 0x75, 0x63, 0x6B
    };
    DuckPacket dp;

    TEST_ASSERT_EQUAL(0, dp.getDeviceId().size());

    dp.setDeviceId(duid);
    TEST_ASSERT_EQUAL(8, dp.getDeviceId().size());
    TEST_ASSERT_EQUAL(0x50, dp.getDeviceId()[0]);
    TEST_ASSERT_EQUAL(0x61, dp.getDeviceId()[1]);
    TEST_ASSERT_EQUAL(0x70, dp.getDeviceId()[2]);
    TEST_ASSERT_EQUAL(0x61, dp.getDeviceId()[3]);
    TEST_ASSERT_EQUAL(0x44, dp.getDeviceId()[4]);
    TEST_ASSERT_EQUAL(0x75, dp.getDeviceId()[5]);
    TEST_ASSERT_EQUAL(0x63, dp.getDeviceId()[6]);
    TEST_ASSERT_EQUAL(0x6B, dp.getDeviceId()[7]);
}

void test_DuckPacket_prepareForSending(void) {
    std::vector<byte> duid = {
        0x50, 0x61, 0x70, 0x61,
        0x44, 0x75, 0x63, 0x6B
    };
    DuckPacket dp(duid);
    std::vector<byte> targetDevice = {
        0x50, 0x61, 0x70, 0x61,
        0x90, 0x75, 0x43, 0x67
    };
    byte duckType = DuckType::LINK;
    byte topic = topics::status;
    std::vector<byte> app_data = {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08
    };
    BloomFilter filter;
    int result = dp.prepareForSending(&filter, targetDevice, duckType, topic, app_data);
    TEST_ASSERT_EQUAL(DUCK_ERR_NONE, result);
}

void test_DuckPacket_prepareForSending_data_length_zero(void) {
    std::vector<byte> duid = {
        0x50, 0x61, 0x70, 0x61,
        0x44, 0x75, 0x63, 0x6B
    };
    DuckPacket dp(duid);
    std::vector<byte> targetDevice = {
        0x50, 0x61, 0x70, 0x61,
        0x90, 0x75, 0x43, 0x67
    };
    byte duckType = DuckType::LINK;
    byte topic = topics::status;
    std::vector<byte> app_data = {};
    BloomFilter filter(10, 5, 16, 5);
    int result = dp.prepareForSending(&filter, targetDevice, duckType, topic, app_data);
    TEST_ASSERT_EQUAL(DUCKPACKET_ERR_SIZE_INVALID, result);
}

void test_DuckPacket_prepareForSending_data_length_exceeds_max(void) {
    std::vector<byte> duid = {
        0x50, 0x61, 0x70, 0x61,
        0x44, 0x75, 0x63, 0x6B
    };
    DuckPacket dp(duid);
    std::vector<byte> targetDevice = {
        0x50, 0x61, 0x70, 0x61,
        0x90, 0x75, 0x43, 0x67
    };
    byte duckType = DuckType::LINK;
    byte topic = topics::status;
    std::vector<byte> app_data;
    for (int i = 0; i < MAX_DATA_LENGTH + 1; i++) {
        app_data.push_back(i);
    }
    BloomFilter filter(10, 5, 16, 5);
    int result = dp.prepareForSending(&filter, targetDevice, duckType, topic, app_data);
    TEST_ASSERT_EQUAL(DUCKPACKET_ERR_SIZE_INVALID, result);
}

void test_prepareForRelaying_returns_true(void) {
    DuckPacket dp;
    BloomFilter filter(10, 5, 16, 5);
    std::vector<byte> message = {
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
    bool result = dp.prepareForRelaying(&filter, message);
    TEST_ASSERT_TRUE(result);
}

void test_prepareForRelaying_returns_false(void) {
    DuckPacket dp;
    BloomFilter filter(10, 5, 16, 5);
    std::vector<byte> message = {
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
    // add the message MUID to the filter and then try to relay the message
    // the message should not be relayed because it has already been seen
    filter.bloom_add(&message[MUID_POS], MUID_LENGTH);
    bool result = dp.prepareForRelaying(&filter, message);
    TEST_ASSERT_FALSE(result);
}


void test_DuckPacket_getBuffer(void) {
    std::vector<byte> duid = {
        0x50, 0x61, 0x70, 0x61,
        0x44, 0x75, 0x63, 0x6B
    };
    DuckPacket dp(duid);
    std::vector<byte> targetDevice = {
        0x50, 0x61, 0x70, 0x61,
        0x90, 0x75, 0x43, 0x67
    };
    byte duckType = DuckType::LINK;
    byte topic = topics::status;
    std::vector<byte> app_data = {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08
    };
    BloomFilter filter;
    int rc = dp.prepareForSending(&filter, targetDevice, duckType, topic, app_data);
    TEST_ASSERT_EQUAL(DUCK_ERR_NONE, rc);

    std::vector<byte> buffer = dp.getBuffer();
    TEST_ASSERT_EQUAL(topic, buffer[TOPIC_POS]);
    TEST_ASSERT_EQUAL(duckType, buffer[DUCK_TYPE_POS]);
    TEST_ASSERT_EQUAL(app_data[0], buffer[DATA_POS]);
    TEST_ASSERT_EQUAL(app_data[7], buffer[DATA_POS + 7]);
    TEST_ASSERT_EQUAL(0x00, buffer[HOP_COUNT_POS]);
}

   
void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_DuckPacket_constructor_default);
    RUN_TEST(test_DuckPacket_constructor_with_duid);
    RUN_TEST(test_DuckPacket_setDeviceId);
    RUN_TEST(test_DuckPacket_prepareForSending);
    RUN_TEST(test_DuckPacket_prepareForSending_data_length_zero);
    RUN_TEST(test_DuckPacket_prepareForSending_data_length_exceeds_max);
    RUN_TEST(test_prepareForRelaying_returns_true);
    RUN_TEST(test_prepareForRelaying_returns_false);
    RUN_TEST(test_DuckPacket_getBuffer);
    UNITY_END();
}

void loop() {
}