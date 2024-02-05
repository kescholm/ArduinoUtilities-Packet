#include <unity.h>

#include "ArdPacketBuffer.h"
#include "ArdPacket.h"

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

#define TEST_MESSAGE_STRING "Hello, World!"

#define TEST_READ_BUFFER_SIZE 64
#define TEST_WRITE_BUFFER_SIZE 64

// utility

static size_t ArdPacketGetHeaderSizeUtility(const ArdPacketConfig &config)
{
    size_t header_size = 1 + config.message_type_bytes + config.payload_size_bytes;
    if (config.crc)
    {
        header_size += 2;
    }
    return header_size;
}

static size_t ArdPacketGetPacketSizeUtility(const ArdPacketConfig &config, const size_t payload_size)
{
    size_t packet_size = ArdPacketGetHeaderSizeUtility(config) + payload_size;
    if (config.crc)
    {
        packet_size += 2;
    }
    return packet_size;
}

// Create and configure
static void test_packet_configure_pass(void)
{
    ArdPacketBuffer packet_buffer;
    uint8_t read_buffer[TEST_READ_BUFFER_SIZE] = {'\0'};
    uint8_t write_buffer[TEST_WRITE_BUFFER_SIZE] = {'\0'};
    TEST_ASSERT_TRUE(packet_buffer.set_read_buffer(read_buffer, TEST_READ_BUFFER_SIZE));
    TEST_ASSERT_TRUE(packet_buffer.set_write_buffer(read_buffer, TEST_READ_BUFFER_SIZE));
    ArdPacket packet(packet_buffer);

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
static void test_packet_configure_fail_payload_bytes(void)
{
    ArdPacketBuffer packet_buffer;
    uint8_t read_buffer[TEST_READ_BUFFER_SIZE] = {'\0'};
    uint8_t write_buffer[TEST_WRITE_BUFFER_SIZE] = {'\0'};
    TEST_ASSERT_TRUE(packet_buffer.set_read_buffer(read_buffer, TEST_READ_BUFFER_SIZE));
    TEST_ASSERT_TRUE(packet_buffer.set_write_buffer(read_buffer, TEST_READ_BUFFER_SIZE));
    ArdPacket packet(packet_buffer);

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
static void test_packet_configure_fail_message_type(void)
{
    ArdPacketBuffer packet_buffer;
    uint8_t read_buffer[TEST_READ_BUFFER_SIZE] = {'\0'};
    uint8_t write_buffer[TEST_WRITE_BUFFER_SIZE] = {'\0'};
    TEST_ASSERT_TRUE(packet_buffer.set_read_buffer(read_buffer, TEST_READ_BUFFER_SIZE));
    TEST_ASSERT_TRUE(packet_buffer.set_write_buffer(read_buffer, TEST_READ_BUFFER_SIZE));
    ArdPacket packet(packet_buffer);

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
static void test_packet_configure_fail_max_payload(void)
{
    ArdPacketBuffer packet_buffer;
    uint8_t read_buffer[TEST_READ_BUFFER_SIZE] = {'\0'};
    uint8_t write_buffer[TEST_WRITE_BUFFER_SIZE] = {'\0'};
    TEST_ASSERT_TRUE(packet_buffer.set_read_buffer(read_buffer, TEST_READ_BUFFER_SIZE));
    TEST_ASSERT_TRUE(packet_buffer.set_write_buffer(read_buffer, TEST_READ_BUFFER_SIZE));
    ArdPacket packet(packet_buffer);

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

// Write and read sample message
static void test_packet_pass_write_read(void)
{
    ArdPacketBuffer packet_buffer;
    uint8_t read_buffer[TEST_READ_BUFFER_SIZE] = {'\0'};
    uint8_t write_buffer[TEST_WRITE_BUFFER_SIZE] = {'\0'};
    TEST_ASSERT_TRUE(packet_buffer.set_read_buffer(read_buffer, TEST_READ_BUFFER_SIZE));
    TEST_ASSERT_TRUE(packet_buffer.set_write_buffer(read_buffer, TEST_READ_BUFFER_SIZE));
    ArdPacket packet(packet_buffer);

    // configure
    ArdPacketConfig config;
    config.crc = true;
    config.delimiter = '|';
    config.message_type_bytes = 1;
    config.payload_size_bytes = 2;
    config.max_payload_size = 2048;
    const eArdPacketConfigStatus status = packet.Configure(config);
    TEST_ASSERT_EQUAL(kArdPacketConfigSuccess, status);

    // input message
    const char *input_test_message = TEST_MESSAGE_STRING;
    const ArdPacketPayloadInfo input_info = {.message_type = 0, .payload_size = sizeof(TEST_MESSAGE_STRING)};

    // sizes
    const size_t header_size = ArdPacketGetHeaderSizeUtility(config);
    const size_t packet_size = ArdPacketGetPacketSizeUtility(config, input_info.payload_size);
    TEST_ASSERT_LESS_OR_EQUAL(TEST_WRITE_BUFFER_SIZE, packet_size);

    // write
    packet_buffer.set_write_buffer(write_buffer, packet_size);
    const eArdPacketStatus send_status =
        packet.SendPayload(input_info, reinterpret_cast<const uint8_t *>(input_test_message));
    TEST_ASSERT_EQUAL(kArdPacketStatusDone, send_status);
    TEST_ASSERT_EQUAL_STRING(TEST_MESSAGE_STRING, &write_buffer[header_size]);

    // read
    packet_buffer.set_read_buffer(read_buffer, packet_size);
    memcpy(read_buffer, write_buffer, packet_size);

    uint8_t receive_buffer[sizeof(TEST_MESSAGE_STRING)] = {0};
    ArdPacketPayloadInfo receive_info;
    const eArdPacketStatus recv_status = packet.ReceivePayload(input_info.payload_size, receive_info, receive_buffer);
    TEST_ASSERT_EQUAL(kArdPacketStatusDone, recv_status);
    TEST_ASSERT_EQUAL_STRING(TEST_MESSAGE_STRING, receive_buffer);
}

// Write and read sample message
static void test_packet_pass_static_write_read(void)
{
    uint8_t data_buffer[TEST_WRITE_BUFFER_SIZE] = {'\0'};

    ArdPacketBuffer packet_buffer = {};
    ArdPacket packet(packet_buffer);

    // configure
    ArdPacketConfig config;
    config.crc = true;
    config.delimiter = '|';
    config.message_type_bytes = 2;
    config.payload_size_bytes = 2;
    config.max_payload_size = TEST_WRITE_BUFFER_SIZE - 9;
    const eArdPacketConfigStatus status = packet.Configure(config);
    TEST_ASSERT_EQUAL(kArdPacketConfigSuccess, status);

    // input message
    const char *input_test_message = TEST_MESSAGE_STRING;
    const ArdPacketPayloadInfo input_info = {.message_type = 0, .payload_size = sizeof(TEST_MESSAGE_STRING)};

    // sizes
    const size_t header_size = ArdPacketGetHeaderSizeUtility(config);
    const size_t packet_size = ArdPacketGetPacketSizeUtility(config, input_info.payload_size);
    TEST_ASSERT_LESS_OR_EQUAL(TEST_WRITE_BUFFER_SIZE, packet_size);

    // write
    size_t packet_size_result = 0;
    const eArdPacketStatus send_status = packet.WritePacketToBuffer(
        input_info, reinterpret_cast<const uint8_t *>(input_test_message),
        TEST_READ_BUFFER_SIZE, data_buffer, packet_size_result);
    TEST_ASSERT_EQUAL(kArdPacketStatusDone, send_status);
    TEST_ASSERT_EQUAL_STRING(TEST_MESSAGE_STRING, &data_buffer[header_size]);

    // read
    ArdPacketPayloadInfo receive_info;
    size_t payload_index = 0;
    const eArdPacketStatus recv_status = packet.ReadPacketFromBuffer(data_buffer, packet_size_result, receive_info, payload_index);
    TEST_ASSERT_EQUAL(kArdPacketStatusDone, recv_status);
    TEST_ASSERT_EQUAL_STRING(TEST_MESSAGE_STRING, &data_buffer[payload_index]);
    TEST_ASSERT_EQUAL(7, payload_index);
}

int main(void)
{
    UNITY_BEGIN();

    // Run Tests
    // ---------

    RUN_TEST(test_packet_configure_pass);
    RUN_TEST(test_packet_configure_fail_payload_bytes);
    RUN_TEST(test_packet_configure_fail_message_type);
    RUN_TEST(test_packet_configure_fail_max_payload);
    RUN_TEST(test_packet_pass_write_read);

    RUN_TEST(test_packet_pass_static_write_read);

    // Done
    // ----

    UNITY_END();

    return 0;
}
