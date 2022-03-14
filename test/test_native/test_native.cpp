#include <unity.h>

#include "ArdPacket.h"

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

#define TEST_MESSAGE_STRING "Hello, World!"

class ArdPacketSerialMock : public ArdPacketStreamInterface
{
   public:
    ArdPacketSerialMock() = default;

    int available() override { return m_read_available; }
    int read() override
    {
        int read_byte = -1;
        if (m_read_offset < m_read_available)
        {
            read_byte = m_read_data[m_read_offset];
            m_read_offset++;
        }
        return read_byte;
    }
    size_t read(uint8_t *buffer, size_t size) override
    {
        size_t read_index = 0;
        int read_byte = 0;
        while (read_byte >= 0 && read_index < size)
        {
            read_byte = read();
            if (read_byte >= 0)
            {
                buffer[read_index] = static_cast<uint8_t>(read_byte);
                read_index++;
            }
        }
        return read_index;
    }

    int availableForWrite() override { return m_write_available; }
    size_t write(uint8_t value) override
    {
        size_t write_count = 0;
        if (m_write_offset < m_write_available)
        {
            m_write_data[m_write_offset] = value;
            m_write_offset++;
            write_count = 1;
        }
        return write_count;
    }
    size_t write(const uint8_t *buffer, size_t size) override
    {
        size_t write_index = 0;
        size_t write_count = 1;
        while (write_count > 0 && write_index < size)
        {
            write_count = write(buffer[write_index]);
            if (write_count > 0)
            {
                write_index++;
            }
        }
        return write_index;
    }

    void MakeAvailableForRead(size_t size)
    {
        m_read_available = (kReadBufferSize < size ? kReadBufferSize : size);
        m_read_offset = 0;
    }

    void MakeAvailableForWrite(size_t size)
    {
        m_write_available = (kWriteBufferSize < size ? kWriteBufferSize : size);
        m_write_offset = 0;
    }

public:

    static constexpr int kReadBufferSize = 64;
    static constexpr int kWriteBufferSize = 64;

    size_t m_read_available = 0;
    uint8_t m_read_data[kReadBufferSize] = {0};
    size_t m_read_offset = 0;

    size_t m_write_available = 0;
    uint8_t m_write_data[kWriteBufferSize] = {0};
    size_t m_write_offset = 0;
};

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
static void test_packet_configure_pass(void) {

    ArdPacketSerialMock serial;
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

    ArdPacketSerialMock serial;
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

    ArdPacketSerialMock serial;
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

    ArdPacketSerialMock serial;
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

// Write and read sample message
static void test_packet_pass_write_read(void) {

    ArdPacketSerialMock serial;
    ArdPacket packet(dynamic_cast<ArdPacketStreamInterface&>(serial));

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
    const ArdPacketPayloadInfo input_info =
    {
        .message_type = 0,
        .payload_size = sizeof(TEST_MESSAGE_STRING)
    };

    // sizes
    const size_t header_size = ArdPacketGetHeaderSizeUtility(config);
    const size_t packet_size = ArdPacketGetPacketSizeUtility(config, input_info.payload_size);
    TEST_ASSERT_LESS_OR_EQUAL(serial.kWriteBufferSize, packet_size);

    // write
    serial.MakeAvailableForWrite(packet_size);
    const eArdPacketStatus send_status = packet.SendPayload(input_info, reinterpret_cast<const uint8_t*>(input_test_message));
    TEST_ASSERT_EQUAL(kArdPacketStatusDone, send_status);
    TEST_ASSERT_EQUAL_STRING(TEST_MESSAGE_STRING, &serial.m_write_data[header_size]);

    // read
    serial.MakeAvailableForRead(packet_size);
    memcpy(serial.m_read_data, serial.m_write_data, packet_size);

    uint8_t receive_buffer[sizeof(TEST_MESSAGE_STRING)] = {0};
    ArdPacketPayloadInfo receive_info;
    const eArdPacketStatus recv_status = packet.ReceivePayload(input_info.payload_size, receive_info, receive_buffer);
    TEST_ASSERT_EQUAL(kArdPacketStatusDone, recv_status);
    TEST_ASSERT_EQUAL_STRING(TEST_MESSAGE_STRING, receive_buffer);

}

int main(void) {

    UNITY_BEGIN();

    // Run Tests
    // ---------

    RUN_TEST(test_packet_configure_pass);
    RUN_TEST(test_packet_configure_fail_payload_bytes);
    RUN_TEST(test_packet_configure_fail_message_type);
    RUN_TEST(test_packet_configure_fail_max_payload);
    RUN_TEST(test_packet_pass_write_read);

    // Done
    // ----

    UNITY_END();

    return 0;
}
