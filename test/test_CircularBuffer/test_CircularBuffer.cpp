#include <unity.h>
#include "circularBuffer.h"

void test_circularBuffer_constructor_default(void) {
    CircularBuffer cb(10);
    TEST_ASSERT_EQUAL(0, cb.getHead());
    TEST_ASSERT_EQUAL(0, cb.getTail());
    TEST_ASSERT_EQUAL(11, cb.getBufferEnd());
}

void test_circularBuffer_push_one_message(void) {
    CircularBuffer cb(10);
    CdpPacket packet;
    packet.muid = {0, 1, 2, 3, 4, 5, 6, 7};
    cb.push(packet);
    TEST_ASSERT_EQUAL(1, cb.getHead());
    TEST_ASSERT_EQUAL(0, cb.getTail());
    TEST_ASSERT_EQUAL(11, cb.getBufferEnd());
    CdpPacket message = cb.getMessage(0);
    
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_EQUAL(packet.muid[i], message.muid[i]);
    }
}

void test_circleBuffer_findMuid(void) {
    CircularBuffer cb(10);
    CdpPacket packet;
    packet.muid = {0, 1, 2, 3, 4, 5, 6, 7};
    cb.push(packet);
    int index = cb.findMuid(packet.muid);
    TEST_ASSERT_EQUAL(0, index);
}

void test_circularBuffer_updateMuid(void) {
    CircularBuffer cb(10);
    CdpPacket packet;
    packet.muid = {0, 1, 2, 3, 4, 5, 6, 7};
    cb.push(packet);
    std::vector<byte> newMuid = {1, 2, 3, 4, 5, 6, 7, 8};
    cb.updateMuid(packet.muid, newMuid);
    int index = cb.findMuid(newMuid);
    TEST_ASSERT_EQUAL(0, index);
}

void test_circularBuffer_ackMessage(void) {
    CircularBuffer cb(10);
    CdpPacket packet;
    packet.muid = {0, 1, 2, 3, 4, 5, 6, 7};
    cb.push(packet);
    cb.ackMessage(packet.muid);
    int index = cb.findMuid(packet.muid);
    TEST_ASSERT_EQUAL(1, cb.getMessage(index).acked);
}


void test_circularBuffer_push_two_messages(void) {
    CircularBuffer cb(10);
    CdpPacket packet;
    packet.muid = {0, 1, 2, 3, 4, 5, 6, 7};
    cb.push(packet);
    CdpPacket packet2;
    packet2.muid = {1, 2, 3, 4, 5, 6, 7, 8};
    cb.push(packet2);
    TEST_ASSERT_EQUAL(2, cb.getHead());
    TEST_ASSERT_EQUAL(0, cb.getTail());
    TEST_ASSERT_EQUAL(11, cb.getBufferEnd());
    CdpPacket message = cb.getMessage(0);
    
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_EQUAL(packet.muid[i], message.muid[i]);
    }
    CdpPacket message2 = cb.getMessage(1);
    
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_EQUAL(packet2.muid[i], message2.muid[i]);
    }
}


void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_circularBuffer_constructor_default);
    RUN_TEST(test_circularBuffer_push_one_message);
    RUN_TEST(test_circularBuffer_push_two_messages);
    RUN_TEST(test_circleBuffer_findMuid);
    RUN_TEST(test_circularBuffer_updateMuid);
    RUN_TEST(test_circularBuffer_ackMessage);
    UNITY_END();
}

void loop() {
}