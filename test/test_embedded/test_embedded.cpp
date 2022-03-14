#include <Arduino.h>
#include <unity.h>

#include "ArdPacket.h"
#include "ArdPacketSerial.h"

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

// Packet
// -----------

// Create and configure
static void test_packet_configure_pass(void) {

    ArdPacketSerial serial;
    ArdPacket packet(dynamic_cast<ArdPacketStreamInterface&>(serial));

    ArdPacketConfig config;
    config.crc = false;
    config.delimiter = '|';
    config.message_type_bytes = 1;

    config.payload_size_bytes = 1;
    config.max_payload_size = UINT8_MAX;
    eArdPacketConfigStatus status = packet.Configure(config);
    TEST_ASSERT_EQUAL(kArdPacketConfigSuccess, status);

    config.payload_size_bytes = 2;
    config.max_payload_size = UINT16_MAX;
    status = packet.Configure(config);
    TEST_ASSERT_EQUAL(kArdPacketConfigSuccess, status);

    config.payload_size_bytes = 4;
    config.max_payload_size = UINT32_MAX;
    status = packet.Configure(config);
    TEST_ASSERT_EQUAL(kArdPacketConfigSuccess, status);

    config.message_type_bytes = 2;
    status = packet.Configure(config);
    TEST_ASSERT_EQUAL(kArdPacketConfigSuccess, status);

    config.message_type_bytes = 4;
    status = packet.Configure(config);
    TEST_ASSERT_EQUAL(kArdPacketConfigSuccess, status);

}

// Fail create and configure payload size
static void test_packet_configure_fail_payload_bytes(void) {

    ArdPacketSerial serial;
    ArdPacket packet(dynamic_cast<ArdPacketStreamInterface&>(serial));

    ArdPacketConfig config;
    config.crc = false;
    config.delimiter = '|';
    config.message_type_bytes = 1;
    config.max_payload_size = UINT8_MAX;

    config.payload_size_bytes = 0;
    eArdPacketConfigStatus status = packet.Configure(config);
    TEST_ASSERT_EQUAL(kArdPacketConfigInvalidPayloadSizeBytes, status);

    config.payload_size_bytes = 3;
    status = packet.Configure(config);
    TEST_ASSERT_EQUAL(kArdPacketConfigInvalidPayloadSizeBytes, status);

    config.payload_size_bytes = 5;
    status = packet.Configure(config);
    TEST_ASSERT_EQUAL(kArdPacketConfigInvalidPayloadSizeBytes, status);
}

// Create and configure
static void test_packet_configure_fail_message_type(void) {

    ArdPacketSerial serial;
    ArdPacket packet(dynamic_cast<ArdPacketStreamInterface&>(serial));

    ArdPacketConfig config;
    config.crc = false;
    config.delimiter = '|';
    config.payload_size_bytes = 1;
    config.max_payload_size = UINT8_MAX;

    config.message_type_bytes = 0;
    eArdPacketConfigStatus status = packet.Configure(config);
    TEST_ASSERT_EQUAL(kArdPacketConfigInvalidMessageTypeBytes, status);

    config.message_type_bytes = 3;
    status = packet.Configure(config);
    TEST_ASSERT_EQUAL(kArdPacketConfigInvalidMessageTypeBytes, status);

    config.message_type_bytes = 5;
    status = packet.Configure(config);
    TEST_ASSERT_EQUAL(kArdPacketConfigInvalidMessageTypeBytes, status);

}

// Fail create and configure max payload size
static void test_packet_configure_fail_max_payload(void) {

    ArdPacketSerial serial;
    ArdPacket packet(dynamic_cast<ArdPacketStreamInterface&>(serial));

    ArdPacketConfig config;
    config.crc = false;
    config.delimiter = '|';
    config.message_type_bytes = 1;

    config.payload_size_bytes = 1;
    config.max_payload_size = UINT8_MAX + 1;
    eArdPacketConfigStatus status = packet.Configure(config);
    TEST_ASSERT_EQUAL(kArdPacketConfigInvalidMaxPayloadSize, status);

    config.payload_size_bytes = 2;
    config.max_payload_size = UINT16_MAX + 2;
    status = packet.Configure(config);
    TEST_ASSERT_EQUAL(kArdPacketConfigInvalidMaxPayloadSize, status);
}

void setup()
{
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    UNITY_BEGIN();  // IMPORTANT LINE!

    // begin serial
    Serial.begin(ARD_SERIAL_BAUDRATE);
    delay(100);

    // Run Tests
    // ---------

    RUN_TEST(test_packet_configure_pass);
    RUN_TEST(test_packet_configure_fail_payload_bytes);
    RUN_TEST(test_packet_configure_fail_message_type);
    RUN_TEST(test_packet_configure_fail_max_payload);

    // Done
    // ----

    UNITY_END();

}

void loop()
{
    // never reached
}
