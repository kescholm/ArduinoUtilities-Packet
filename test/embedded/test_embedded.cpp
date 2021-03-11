#include <Arduino.h>
#include <unity.h>

#include "ArdPacket.h"

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

// Serial
// ------

#ifndef SERIAL_BAUDRATE
#define SERIAL_BAUDRATE 115200
#endif

// Packet
// -----------

#define MAX_BUFFER_LEN 120

// global packet object
ArdSerialPacket g_packet_read;
ArdSerialPacket g_packet_write;

// Timing
// ------

uint32_t g_timing_now = 0;
uint32_t g_timing_prev = 0;
uint32_t g_timing_step_size = 10000U;

// Tests
// -----

static void test_ard_packet_alloc(void)
{

    // Packet Parameters
    size_t max_size = MAX_BUFFER_LEN;
    uint8_t delimiter = '|';
    bool use_crc = true;

    // allocate read
    int result = ard_packet_alloc(&g_packet_read, max_size, delimiter, use_crc);
    TEST_ASSERT_EQUAL(0, result);

    // allocate write
    result = ard_packet_alloc(&g_packet_write, max_size, delimiter, use_crc);
    TEST_ASSERT_EQUAL(0, result);

}

void setup()
{
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    UNITY_BEGIN();  // IMPORTANT LINE!

    // begin serial
    ard_serial_begin(SERIAL_BAUDRATE);
    delay(100);

    // join i2c bus
    // ard_i2c_master_begin();
    // delay(100);

    // Test initialization
    RUN_TEST(test_ard_packet_alloc);

    // flush serial
    ard_serial_write_flush();
    ard_serial_read_flush();

    // Timing
    g_timing_now = micros();
    g_timing_prev = g_timing_now;

}

// Loop

const size_t g_test_steps = 200;
size_t g_test_count = 0;

void loop()
{
    g_timing_now = micros();

    // 100 HZ
    // -----
    if ((g_timing_now - g_timing_prev) >= g_timing_step_size)
    {
        // elapsed steps
        uint32_t elapsed_steps = (g_timing_now - g_timing_prev) / g_timing_step_size;
        // advance step time
        g_timing_prev += elapsed_steps * g_timing_step_size;
        // should not miss a step
        TEST_ASSERT_EQUAL(1, elapsed_steps);

        // Do packet read/write tests

        // counter
        g_test_count++;
    }

    if (g_test_count > g_test_steps)
    {
        // done
        UNITY_END();  // stop unit testing
    }
}
