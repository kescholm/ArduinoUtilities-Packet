#include <Arduino.h>
#include <unity.h>

#include "ArdPacket.h"
#include "ArdPacketSerial.h"
#include "ArdPacketBluetooth.h"

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

// BT
// -----------

#define ARD_BT_TEST_NAME "Esp32Test"

BluetoothSerial g_bt;

// Packet
// -----------

// Create and configure
static void test_packet_configure(void) {

    ArdPacketSerial serial;
    ArdPacket packet(dynamic_cast<ArdPacketStreamInterface&>(serial));

    ArdPacketBluetooth bt_serial(g_bt);
    ArdPacket bt_packet(dynamic_cast<ArdPacketStreamInterface&>(bt_serial));

    ArdPacketConfig config;
    config.crc = false;
    config.delimiter = '|';
    config.message_type_bytes = 1;
    config.payload_size_bytes = 2;
    config.max_payload_size = 2048;

    eArdPacketConfigStatus status = packet.Configure(config);
    TEST_ASSERT_EQUAL(kArdPacketConfigSuccess, status);

    status = bt_packet.Configure(config);
    TEST_ASSERT_EQUAL(kArdPacketConfigSuccess, status);

}

void setup()
{
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    UNITY_BEGIN();  // IMPORTANT LINE!

    // begin serial
    Serial.begin(SERIAL_BAUDRATE);
    delay(100);

    g_bt.begin(ARD_BT_TEST_NAME);
    delay(100);

    // Run Tests
    // ---------

    RUN_TEST(test_packet_configure);

    // Done
    // ----

    UNITY_END();

}

void loop()
{
    // never reached
}
