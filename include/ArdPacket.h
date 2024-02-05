

#ifndef ARD_PACKET_H
#define ARD_PACKET_H

#ifdef NATIVE_TEST_BUILD
#include <arpa/inet.h>
#else
#include <Arduino.h>
#endif

#include <stdint.h>
#include <string.h>

#include "ArdCrc.h"

/**
 * @brief Packet Configuration
 *
 */
struct ArdPacketConfig
{
    /**
     * @brief Packet delimiter
     */
    uint8_t delimiter = 0;

    /**
     * @brief Number of bytes to specify message type
     *
     * Can be 1, 2, or 4 bytes
     */
    uint8_t message_type_bytes = 0;

    /**
     * @brief Number of bytes to specify size of data payload
     *
     * Can be 1, 2, or 4 bytes
     */
    uint8_t payload_size_bytes = 0;

    /**
     * @brief Maximum size of data payload
     */
    size_t max_payload_size = 0;

    /**
     * @brief Option to use CRC
     */
    bool crc = false;
};

/**
 * @brief Payload info
 */
struct ArdPacketPayloadInfo
{
    /**
     * @brief message type
     *
     */
    uint32_t message_type = 0;

    /**
     * @brief Payload size
     */
    size_t payload_size = 0;
};

/**
 * @brief Status of Configure
 */
enum eArdPacketConfigStatus
{
    kArdPacketConfigSuccess = 0,
    kArdPacketConfigInvalidMessageTypeBytes,
    kArdPacketConfigInvalidPayloadSizeBytes,
    kArdPacketConfigInvalidMaxPayloadSize
};

/**
 * @brief Status of receiving or sending payload
 */
enum eArdPacketStatus
{
    kArdPacketStatusStart = 0,
    kArdPacketStatusNotConfigured,
    kArdPacketStatusNotAvailable,
    kArdPacketStatusNotEnoughAvailable,
    kArdPacketStatusPacketSizeTooSmall,
    kArdPacketStatusNoDelimiter,
    kArdPacketStatusInvalidMessageType,
    kArdPacketStatusInvalidPayloadSize,
    kArdPacketStatusReadFailed,
    kArdPacketStatusCrcFailed,
    kArdPacketStatusHeaderInProgress,
    kArdPacketStatusPayloadInProgress,
    kArdPacketStatusDone
};

/**
 * @brief Abstract class compatible with Arduino's @c Serial interface.
 *
 * The interface should allow for non-blocking write the same way
 * Arduino's @c Serial interface is implemented as described here:
 * [https://www.arduino.cc/reference/en/language/functions/communication/serial/write/](https://www.arduino.cc/reference/en/language/functions/communication/serial/write/)
 */
class ArdPacketStreamInterface
{
   public:
    ArdPacketStreamInterface() = default;

    virtual int available() = 0;
    virtual int read() = 0;
    virtual size_t read(uint8_t *buffer, size_t size) = 0;

    virtual int availableForWrite() = 0;
    virtual size_t write(uint8_t value) = 0;
    virtual size_t write(const uint8_t *buffer, size_t size) = 0;
};

class ArdPacket
{
   public:
    explicit ArdPacket(ArdPacketStreamInterface &stream) : m_stream(stream) {}

    /**
     * @brief Configure packet
     *
     * @param config
     * @return
     */
    eArdPacketConfigStatus Configure(const ArdPacketConfig &config);

    /**
     * @brief Receive payload from data stream
     *
     * @param max_payload_size
     * @param payload
     * @param info
     * @return
     */
    eArdPacketStatus ReceivePayload(size_t max_payload_size, ArdPacketPayloadInfo &info, uint8_t *payload);

    /**
     * @brief Write payload to data stream
     *
     * @param message_type
     * @param payload_size
     * @param payload
     * @return
     */
    eArdPacketStatus SendPayload(const ArdPacketPayloadInfo &info, const uint8_t *payload);

    /**
     * @brief Reset state
     */
    void Reset()
    {
        ResetRead();
        ResetWrite();
    }

    /**
     * @brief Reset read state
     */
    void ResetRead()
    {
        ResetState(m_read);
    }

    /**
     * @brief Reset write state
     */
    void ResetWrite()
    {
        ResetState(m_write);
    }

    /**
     * @brief Copy payload into external packet buffer
     *
     * @param info
     * @param payload
     * @param max_packet_size
     * @param packet
     * @param packet_size
     * @return eArdPacketStatus
     */
    eArdPacketStatus WritePacketToBuffer(const ArdPacketPayloadInfo &info, const uint8_t *payload,
                                         size_t max_packet_size, uint8_t *packet, size_t &packet_size) const;

    /**
     * @brief
     *
     * @param packet
     * @param packet_size
     * @param info
     * @param payload
     * @return eArdPacketStatus
     */
    eArdPacketStatus ReadPacketFromBuffer(const uint8_t *packet, const size_t packet_size, ArdPacketPayloadInfo &info,
                                          size_t &payload_index) const;

   private:
    static constexpr size_t kArdPacketDelimiterBytes = 1;
    static constexpr size_t kArdPacketCrcBytes = 2;
    static constexpr size_t kArdPacketMaxPayloadSizeBytes = 4;
    static constexpr size_t kArdPacketMaxMessageTypeBytes = 4;
    static constexpr size_t kArdPacketMaxHeaderSize =
        1 + kArdPacketMaxPayloadSizeBytes + kArdPacketMaxMessageTypeBytes + 2 * kArdPacketCrcBytes;

    enum eArdPacketState
    {
        kArdPacketStateDelimiter,
        kArdPacketStateMessageType,
        kArdPacketStatePayloadSize,
        kArdPacketStateHeaderCrc,
        kArdPacketStatePayload,
        kArdPacketStatePayloadCrc,
        kArdPacketStateDone
    };

    struct ArdPacketStateData
    {
        eArdPacketState state = kArdPacketStateDelimiter;
        size_t available = 0;
        size_t payload_index = 0;
        crc_t crc = 0;
    };

    static void ConvertToBigEndian(const uint32_t value, const size_t value_bytes, uint8_t *data);
    static uint32_t ConvertFromBigEndian(const uint8_t *data, const size_t value_bytes);
    static void ResetState(ArdPacketStateData &state);

    eArdPacketStatus ProcessReadStateDelimiter();
    eArdPacketStatus ProcessReadStateHeaderCrc();
    eArdPacketStatus ProcessReadStateMessageType(ArdPacketPayloadInfo &info);
    eArdPacketStatus ProcessReadStatePayloadSize(size_t max_payload_size, ArdPacketPayloadInfo &info);
    eArdPacketStatus ProcessReadStatePayload(const ArdPacketPayloadInfo &info, uint8_t *payload);
    eArdPacketStatus ProcessReadStatePayloadCrc();

    eArdPacketStatus ProcessWriteStateDelimiter();
    eArdPacketStatus ProcessWriteStateHeaderCrc();
    eArdPacketStatus ProcessWriteStateMessageType(const ArdPacketPayloadInfo &info);
    eArdPacketStatus ProcessWriteStatePayloadSize(const ArdPacketPayloadInfo &info);
    eArdPacketStatus ProcessWriteStatePayload(const ArdPacketPayloadInfo &info, const uint8_t *payload);
    eArdPacketStatus ProcessWriteStatePayloadCrc();

    // configuration
    ArdPacketConfig m_config = {};
    size_t m_max_message_type_value = 0;

    // read and write state
    ArdPacketStateData m_read = {};
    ArdPacketStateData m_write = {};

    // stream interface
    ArdPacketStreamInterface &m_stream;
};

// inline methods

inline void ArdPacket::ConvertToBigEndian(const uint32_t value, const size_t value_bytes, uint8_t *data)
{
    // copy from data to packet
    if (value_bytes == 1)
    {
        data[0] = static_cast<uint8_t>(value);
    }
    else if (value_bytes == 2)
    {
        uint16_t swapped_value = htons(static_cast<uint16_t>(value));
        memcpy(data, &swapped_value, value_bytes);
    }
    else if (value_bytes == 4)
    {
        uint32_t swapped_value = htonl(value);
        memcpy(data, &swapped_value, value_bytes);
    }
}

inline uint32_t ArdPacket::ConvertFromBigEndian(const uint8_t *data, const size_t value_bytes)
{
    uint32_t retval = 0;
    // copy from data to packet
    if (value_bytes == 1)
    {
        retval = static_cast<uint32_t>(data[0]);
    }
    else if (value_bytes == 2)
    {
        uint16_t swap_value = 0;
        memcpy(&swap_value, data, value_bytes);
        retval = static_cast<uint32_t>(ntohs(swap_value));
    }
    else if (value_bytes == 4)
    {
        uint32_t swap_value = 0;
        memcpy(&swap_value, data, value_bytes);
        retval = ntohl(swap_value);
    }
    return retval;
}

inline eArdPacketConfigStatus ArdPacket::Configure(const ArdPacketConfig &config)
{
    eArdPacketConfigStatus status = kArdPacketConfigSuccess;

    if (config.message_type_bytes != 1 && config.message_type_bytes != 2 && config.message_type_bytes != 4)
    {
        status = kArdPacketConfigInvalidMessageTypeBytes;
    }

    if (status == kArdPacketConfigSuccess && config.payload_size_bytes != 1 && config.payload_size_bytes != 2 &&
        config.payload_size_bytes != 4)
    {
        status = kArdPacketConfigInvalidPayloadSizeBytes;
    }

    size_t header_size = 0;
    size_t header_and_crc_size = 0;
    if (status == kArdPacketConfigSuccess)
    {
        header_size = kArdPacketDelimiterBytes + config.message_type_bytes + config.payload_size_bytes;
        header_and_crc_size = header_size;
        if (config.crc)
        {
            header_size += kArdPacketCrcBytes;
            header_and_crc_size += 2 * kArdPacketCrcBytes;
        }
    }

    if (status == kArdPacketConfigSuccess)
    {
        size_t max_payload_limit = 0;
        if (config.payload_size_bytes == 1)
        {
            max_payload_limit = UINT8_MAX;
        }
        else if (config.payload_size_bytes == 2)
        {
            max_payload_limit = UINT16_MAX;
        }
        else if (config.payload_size_bytes == 4)
        {
            max_payload_limit = (UINT32_MAX < SIZE_MAX ? UINT32_MAX : SIZE_MAX);
        }

        if (config.max_payload_size > max_payload_limit)
        {
            status = kArdPacketConfigInvalidMaxPayloadSize;
        }
    }

    if (status == kArdPacketConfigSuccess)
    {
        // max message type
        if (config.message_type_bytes == 1)
        {
            m_max_message_type_value = UINT8_MAX;
        }
        else if (config.message_type_bytes == 2)
        {
            m_max_message_type_value = UINT16_MAX;
        }
        else if (config.message_type_bytes == 4)
        {
            m_max_message_type_value = (UINT32_MAX < SIZE_MAX ? UINT32_MAX : SIZE_MAX);
        }

        m_config = config;

        ResetState(m_read);
        ResetState(m_write);
    }

    return status;
}

inline eArdPacketStatus ArdPacket::ReceivePayload(const size_t max_payload_size, ArdPacketPayloadInfo &info,
                                                  uint8_t *payload)
{
    eArdPacketStatus status = kArdPacketStatusStart;
    const int read_size = m_stream.available();
    if (m_config.max_payload_size == 0)
    {
        status = kArdPacketStatusNotConfigured;
    }
    else if (read_size <= 0)
    {
        status = kArdPacketStatusNotAvailable;
    }
    else
    {
        m_read.available = static_cast<size_t>(read_size);
        bool continue_read = true;
        while (m_read.available > 0 && continue_read)
        {
            // state machine
            switch (m_read.state)
            {
                case kArdPacketStateDelimiter:
                {
                    status = ProcessReadStateDelimiter();
                    break;
                }
                case kArdPacketStateMessageType:
                {
                    if (m_read.available < m_config.message_type_bytes)
                    {
                        status = kArdPacketStatusNotEnoughAvailable;
                    }
                    else
                    {
                        status = ProcessReadStateMessageType(info);
                    }
                    break;
                }
                case kArdPacketStatePayloadSize:
                {
                    if (m_read.available < m_config.payload_size_bytes)
                    {
                        status = kArdPacketStatusNotEnoughAvailable;
                    }
                    else
                    {
                        status = ProcessReadStatePayloadSize(max_payload_size, info);
                    }
                    break;
                }
                case kArdPacketStateHeaderCrc:
                {
                    if (m_read.available < kArdPacketCrcBytes)
                    {
                        status = kArdPacketStatusNotEnoughAvailable;
                    }
                    else
                    {
                        status = ProcessReadStateHeaderCrc();
                    }
                    break;
                }
                case kArdPacketStatePayload:
                {
                    status = ProcessReadStatePayload(info, payload);
                    break;
                }
                case kArdPacketStatePayloadCrc:
                {
                    if (m_read.available < kArdPacketCrcBytes)
                    {
                        status = kArdPacketStatusNotEnoughAvailable;
                    }
                    else
                    {
                        status = ProcessReadStatePayloadCrc();
                    }
                    break;
                }
                case kArdPacketStateDone:
                {
                    status = kArdPacketStatusDone;
                    break;
                }
                default:
                {
                    status = kArdPacketStatusStart;
                    ResetState(m_read);
                    break;
                }
            }
            continue_read = (status == kArdPacketStatusHeaderInProgress || status == kArdPacketStatusPayloadInProgress);
        }
    }

    return status;
}

inline eArdPacketStatus ArdPacket::SendPayload(const ArdPacketPayloadInfo &info, const uint8_t *payload)
{
    eArdPacketStatus status = kArdPacketStatusStart;
    const int write_size = m_stream.availableForWrite();
    if (m_config.max_payload_size == 0)
    {
        status = kArdPacketStatusNotConfigured;
    }
    else if (write_size <= 0)
    {
        status = kArdPacketStatusNotAvailable;
    }
    else if (info.message_type > m_max_message_type_value)
    {
        status = kArdPacketStatusInvalidMessageType;
    }
    else if (info.payload_size == 0)
    {
        status = kArdPacketStatusNotEnoughAvailable;
        ResetState(m_write);
    }
    else if (info.payload_size > m_config.max_payload_size)
    {
        status = kArdPacketStatusInvalidPayloadSize;
        ResetState(m_write);
    }
    else
    {
        m_write.available = static_cast<size_t>(write_size);
        bool continue_write = true;
        while (m_write.available > 0 && continue_write)
        {
            // state machine
            switch (m_write.state)
            {
                case kArdPacketStateDelimiter:
                {
                    status = ProcessWriteStateDelimiter();
                    break;
                }
                case kArdPacketStateMessageType:
                {
                    if (m_write.available < m_config.message_type_bytes)
                    {
                        status = kArdPacketStatusNotEnoughAvailable;
                    }
                    else
                    {
                        status = ProcessWriteStateMessageType(info);
                    }
                    break;
                }
                case kArdPacketStatePayloadSize:
                {
                    if (m_write.available < m_config.payload_size_bytes)
                    {
                        status = kArdPacketStatusNotEnoughAvailable;
                    }
                    else
                    {
                        status = ProcessWriteStatePayloadSize(info);
                    }
                    break;
                }
                case kArdPacketStateHeaderCrc:
                {
                    if (m_write.available < kArdPacketCrcBytes)
                    {
                        status = kArdPacketStatusNotEnoughAvailable;
                    }
                    else
                    {
                        status = ProcessWriteStateHeaderCrc();
                    }
                    break;
                }
                case kArdPacketStatePayload:
                {
                    status = ProcessWriteStatePayload(info, payload);
                    break;
                }
                case kArdPacketStatePayloadCrc:
                {
                    if (m_write.available < kArdPacketCrcBytes)
                    {
                        status = kArdPacketStatusNotEnoughAvailable;
                    }
                    else
                    {
                        status = ProcessWriteStatePayloadCrc();
                    }
                    break;
                }
                case kArdPacketStateDone:
                {
                    status = kArdPacketStatusDone;
                    break;
                }
                default:
                {
                    status = kArdPacketStatusStart;
                    ResetState(m_write);
                    break;
                }
            }
            continue_write =
                (status == kArdPacketStatusHeaderInProgress || status == kArdPacketStatusPayloadInProgress);
        }
    }

    return status;
}

// Private inline methods
// ----------------------

inline void ArdPacket::ResetState(ArdPacketStateData &data_state)
{
    data_state.state = kArdPacketStateDelimiter;
    data_state.payload_index = 0;
}

// Read State Processing

inline eArdPacketStatus ArdPacket::ProcessReadStateDelimiter()
{
    eArdPacketStatus status = kArdPacketStatusStart;
    bool found_delimiter = false;
    bool read_failed = false;
    for (size_t k = 0; k < m_read.available && (!found_delimiter) && (!read_failed); ++k)
    {
        const int read_byte = m_stream.read();
        if (read_byte < 0)
        {
            read_failed = true;
        }
        else if (static_cast<uint8_t>(read_byte) == m_config.delimiter)
        {
            found_delimiter = true;
        }
    }

    if (read_failed)
    {
        status = kArdPacketStatusReadFailed;
    }
    else if (found_delimiter)
    {
        status = kArdPacketStatusHeaderInProgress;
        m_read.state = kArdPacketStateMessageType;
        if (m_config.crc)
        {
            // initial crc for header
            m_read.crc = crc_init();
            m_read.crc = crc_update(m_read.crc, &m_config.delimiter, kArdPacketDelimiterBytes);
        }
    }
    else
    {
        status = kArdPacketStatusNoDelimiter;
    }

    return status;
}

inline eArdPacketStatus ArdPacket::ProcessReadStateMessageType(ArdPacketPayloadInfo &info)
{
    eArdPacketStatus status = kArdPacketStatusStart;

    uint8_t read_data[kArdPacketMaxMessageTypeBytes];
    const size_t bytes_read = m_stream.read(read_data, m_config.message_type_bytes);
    if (bytes_read != m_config.message_type_bytes)
    {
        status = kArdPacketStatusReadFailed;
        ResetState(m_read);
    }
    else
    {
        if (m_config.crc)
        {
            m_read.crc = crc_update(m_read.crc, read_data, m_config.message_type_bytes);
        }
        // copy message type from data
        info.message_type = ConvertFromBigEndian(read_data, m_config.message_type_bytes);
        // advance state
        status = kArdPacketStatusHeaderInProgress;
        m_read.state = kArdPacketStatePayloadSize;
    }

    return status;
}

eArdPacketStatus ArdPacket::ProcessReadStatePayloadSize(const size_t max_payload_size, ArdPacketPayloadInfo &info)
{
    eArdPacketStatus status = kArdPacketStatusStart;

    uint8_t read_data[kArdPacketMaxPayloadSizeBytes];
    const size_t bytes_read = m_stream.read(read_data, m_config.payload_size_bytes);
    if (bytes_read != m_config.payload_size_bytes)
    {
        status = kArdPacketStatusReadFailed;
        ResetState(m_read);
    }
    else
    {
        if (m_config.crc)
        {
            m_read.crc = crc_update(m_read.crc, read_data, m_config.payload_size_bytes);
        }
        // copy from data to packet
        info.payload_size = ConvertFromBigEndian(read_data, m_config.payload_size_bytes);
        // check payload size
        if ((info.payload_size == 0) || (info.payload_size > m_config.max_payload_size) ||
            (max_payload_size < info.payload_size))
        {
            status = kArdPacketStatusInvalidPayloadSize;
            ResetState(m_read);
        }
        else
        {
            // advance state
            status = (m_config.crc ? kArdPacketStatusHeaderInProgress : kArdPacketStatusPayloadInProgress);
            m_read.state = (m_config.crc ? kArdPacketStateHeaderCrc : kArdPacketStatePayload);
        }
    }

    return status;
}

eArdPacketStatus ArdPacket::ProcessReadStateHeaderCrc()
{
    eArdPacketStatus status = kArdPacketStatusStart;

    uint8_t read_data[kArdPacketCrcBytes];
    const size_t bytes_read = m_stream.read(read_data, kArdPacketCrcBytes);
    if (bytes_read != kArdPacketCrcBytes)
    {
        status = kArdPacketStatusReadFailed;
        ResetState(m_read);
    }
    else
    {
        // crc from data
        m_read.crc = crc_update(m_read.crc, read_data, kArdPacketCrcBytes);
        // finalize and test
        m_read.crc = crc_finalize(m_read.crc);
        // check payload crc
        if (m_read.crc == 0)
        {
            // passed crc
            status = kArdPacketStatusPayloadInProgress;
            m_read.state = kArdPacketStatePayload;
            // initial crc for payload
            m_read.crc = crc_init();
        }
        else
        {
            status = kArdPacketStatusCrcFailed;
            ResetState(m_read);
        }
    }

    return status;
}

eArdPacketStatus ArdPacket::ProcessReadStatePayload(const ArdPacketPayloadInfo &info, uint8_t *payload)
{
    eArdPacketStatus status = kArdPacketStatusPayloadInProgress;

    const size_t remaining_payload = info.payload_size - m_read.payload_index;
    const size_t bytes_to_read = (remaining_payload < m_read.available ? remaining_payload : m_read.available);

    const size_t bytes_read = m_stream.read(&payload[m_read.payload_index], bytes_to_read);
    if (m_config.crc && bytes_read > 0)
    {
        m_read.crc = crc_update(m_read.crc, &payload[m_read.payload_index], bytes_read);
    }
    m_read.payload_index += bytes_read;

    if (bytes_read == 0)
    {
        status = kArdPacketStatusReadFailed;
        ResetState(m_read);
    }
    else if (m_read.payload_index == info.payload_size)
    {
        status = (m_config.crc ? kArdPacketStatusPayloadInProgress : kArdPacketStatusDone);
        m_read.state = (m_config.crc ? kArdPacketStatePayloadCrc : kArdPacketStateDone);
    }

    return status;
}

eArdPacketStatus ArdPacket::ProcessReadStatePayloadCrc()
{
    eArdPacketStatus status = kArdPacketStatusStart;

    uint8_t read_data[kArdPacketCrcBytes];
    const size_t bytes_read = m_stream.read(read_data, kArdPacketCrcBytes);
    if (bytes_read != kArdPacketCrcBytes)
    {
        status = kArdPacketStatusReadFailed;
        ResetState(m_read);
    }
    else
    {
        // crc from data
        m_read.crc = crc_update(m_read.crc, read_data, kArdPacketCrcBytes);
        // finalize and test
        m_read.crc = crc_finalize(m_read.crc);
        // check payload crc
        if (m_read.crc == 0)
        {
            // passed crc
            status = kArdPacketStatusDone;
            m_read.state = kArdPacketStateDone;
        }
        else
        {
            status = kArdPacketStatusCrcFailed;
            ResetState(m_read);
        }
    }

    return status;
}

// Write State Processing

eArdPacketStatus ArdPacket::ProcessWriteStateDelimiter()
{
    // write
    m_stream.write(m_config.delimiter);
    // crc update
    if (m_config.crc)
    {
        m_write.crc = crc_update(m_write.crc, &m_config.delimiter, sizeof(uint8_t));
    }
    // advance state
    m_write.state = kArdPacketStateMessageType;
    return kArdPacketStatusHeaderInProgress;
}

eArdPacketStatus ArdPacket::ProcessWriteStateMessageType(const ArdPacketPayloadInfo &info)
{
    // copy from message type to data
    // host endian copy (TODO: ensure consistent endianness)
    uint8_t write_data[kArdPacketMaxMessageTypeBytes];
    ConvertToBigEndian(info.message_type, m_config.message_type_bytes, write_data);
    // write
    m_stream.write(write_data, m_config.message_type_bytes);
    // crc update
    if (m_config.crc)
    {
        m_write.crc = crc_update(m_write.crc, write_data, m_config.message_type_bytes);
    }
    // advance state
    m_write.state = kArdPacketStatePayloadSize;
    return kArdPacketStatusHeaderInProgress;
}

eArdPacketStatus ArdPacket::ProcessWriteStatePayloadSize(const ArdPacketPayloadInfo &info)
{
    // copy from payload size to data
    uint8_t write_data[kArdPacketMaxMessageTypeBytes];
    ConvertToBigEndian(info.payload_size, m_config.payload_size_bytes, write_data);
    // write
    m_stream.write(write_data, m_config.payload_size_bytes);
    // crc update
    if (m_config.crc)
    {
        m_write.crc = crc_update(m_write.crc, write_data, m_config.payload_size_bytes);
    }
    // advance state
    m_write.state = (m_config.crc ? kArdPacketStateHeaderCrc : kArdPacketStatePayload);
    return (m_config.crc ? kArdPacketStatusHeaderInProgress : kArdPacketStatusPayloadInProgress);
}

eArdPacketStatus ArdPacket::ProcessWriteStateHeaderCrc()
{
    // finalize crc
    m_write.crc = crc_finalize(m_write.crc);
    // write
    m_stream.write(reinterpret_cast<uint8_t *>(&m_write.crc), kArdPacketCrcBytes);
    // reset crc
    m_write.crc = crc_init();
    // advance state
    m_write.state = kArdPacketStatePayload;
    return kArdPacketStatusPayloadInProgress;
}

eArdPacketStatus ArdPacket::ProcessWriteStatePayload(const ArdPacketPayloadInfo &info, const uint8_t *payload)
{
    eArdPacketStatus status = kArdPacketStatusPayloadInProgress;
    // remaining bytes
    const size_t remaining_payload = info.payload_size - m_write.payload_index;
    const size_t bytes_to_write = (remaining_payload < m_write.available ? remaining_payload : m_write.available);
    // write
    m_stream.write(&payload[m_write.payload_index], bytes_to_write);
    // crc update
    if (m_config.crc)
    {
        m_write.crc = crc_update(m_write.crc, &payload[m_write.payload_index], bytes_to_write);
    }
    // update state
    m_write.payload_index += bytes_to_write;
    if (m_write.payload_index == info.payload_size)
    {
        status = (m_config.crc ? kArdPacketStatusPayloadInProgress : kArdPacketStatusDone);
        m_write.state = (m_config.crc ? kArdPacketStatePayloadCrc : kArdPacketStateDone);
    }

    return status;
}

eArdPacketStatus ArdPacket::ProcessWriteStatePayloadCrc()
{
    // finalize crc
    m_write.crc = crc_finalize(m_write.crc);
    // write
    m_stream.write(reinterpret_cast<uint8_t *>(&m_write.crc), kArdPacketCrcBytes);
    // advance state
    m_write.state = kArdPacketStateDone;
    return kArdPacketStatusDone;
}

inline eArdPacketStatus ArdPacket::WritePacketToBuffer(const ArdPacketPayloadInfo &info, const uint8_t *payload,
                                                       const size_t max_packet_size, uint8_t *packet,
                                                       size_t &packet_size) const
{
    eArdPacketStatus status = kArdPacketStatusStart;
    if (m_config.max_payload_size == 0)
    {
        status = kArdPacketStatusNotConfigured;
    }
    else if ((max_packet_size == 0) || (max_packet_size < info.payload_size))
    {
        status = kArdPacketStatusPacketSizeTooSmall;
    }
    else if (info.message_type > m_max_message_type_value)
    {
        status = kArdPacketStatusInvalidMessageType;
    }
    else if (info.payload_size > m_config.max_payload_size)
    {
        status = kArdPacketStatusInvalidPayloadSize;
    }
    else
    {
        const size_t header_size = kArdPacketDelimiterBytes + m_config.message_type_bytes + m_config.payload_size_bytes;
        const size_t header_and_two_crc_size = header_size + (m_config.crc ? 2 * kArdPacketCrcBytes : 0.0);
        if ((max_packet_size - info.payload_size) < header_and_two_crc_size)
        {
            status = eArdPacketStatus::kArdPacketStatusPacketSizeTooSmall;
        }
        else
        {
            // Create packet

            // delimiter
            size_t packet_index = 0;
            packet[packet_index] = m_config.delimiter;
            packet_index += 1;

            // message type
            ConvertToBigEndian(info.message_type, m_config.message_type_bytes, &packet[packet_index]);
            packet_index += m_config.message_type_bytes;

            // payload size
            ConvertToBigEndian(info.payload_size, m_config.payload_size_bytes, &packet[packet_index]);
            packet_index += m_config.payload_size_bytes;

            // header crc
            if (m_config.crc)
            {
                crc_t crc = crc_init();
                crc = crc_update(crc, packet, header_size);
                crc = crc_finalize(crc);
                // write
                memcpy(&packet[packet_index], reinterpret_cast<uint8_t *>(&crc), kArdPacketCrcBytes);
                packet_index += kArdPacketCrcBytes;
            }

            // payload
            memcpy(&packet[packet_index], payload, info.payload_size);
            packet_index += info.payload_size;

            // payload crc
            if (m_config.crc)
            {
                crc_t crc = crc_init();
                crc = crc_update(crc, &packet[header_size + kArdPacketCrcBytes], info.payload_size);
                crc = crc_finalize(crc);
                // write
                memcpy(&packet[packet_index], reinterpret_cast<uint8_t *>(&crc), kArdPacketCrcBytes);
                packet_index += kArdPacketCrcBytes;
            }

            // done
            packet_size = packet_index;
            status = kArdPacketStatusDone;
        }
    }

    return status;
}

inline eArdPacketStatus ArdPacket::ReadPacketFromBuffer(const uint8_t *packet, const size_t packet_size,
                                                        ArdPacketPayloadInfo &info, size_t &payload_index) const
{
    eArdPacketStatus status = kArdPacketStatusStart;
    const size_t header_size = kArdPacketDelimiterBytes + m_config.message_type_bytes + m_config.payload_size_bytes;
    size_t header_and_crc_size = header_size;
    size_t header_and_two_crc_size = header_size;
    if (m_config.crc)
    {
        header_and_crc_size += kArdPacketCrcBytes;
        header_and_two_crc_size += 2 * kArdPacketCrcBytes;
    }
    if (m_config.max_payload_size == 0)
    {
        status = kArdPacketStatusNotConfigured;
    }
    else if (packet_size <= header_and_crc_size)
    {
        status = kArdPacketStatusPacketSizeTooSmall;
    }
    else if (packet[0] != m_config.delimiter)
    {
        status = kArdPacketStatusNoDelimiter;
    }
    else
    {
        // delimiter
        size_t packet_index = kArdPacketDelimiterBytes;

        // message type
        info.message_type = ConvertFromBigEndian(&packet[packet_index], m_config.message_type_bytes);
        packet_index += m_config.message_type_bytes;

        // payload size
        info.payload_size = ConvertFromBigEndian(&packet[packet_index], m_config.payload_size_bytes);
        packet_index += m_config.payload_size_bytes;

        // header crc
        if (m_config.crc)
        {
            crc_t crc = crc_init();
            crc = crc_update(crc, packet, header_and_crc_size);
            crc = crc_finalize(crc);
            if (crc != 0)
            {
                // crc check failed
                status = kArdPacketStatusCrcFailed;
            }
        }

        // check payload size
        if (kArdPacketStatusStart == status)
        {
            if ((info.payload_size > (packet_size - header_and_two_crc_size)) ||
                (info.payload_size > m_config.max_payload_size))
            {
                status = kArdPacketStatusInvalidPayloadSize;
            }
        }

        // header crc
        if ((kArdPacketStatusStart == status) && m_config.crc)
        {
            crc_t crc = crc_init();
            crc = crc_update(crc, &packet[header_and_crc_size], info.payload_size + kArdPacketCrcBytes);
            crc = crc_finalize(crc);
            if (crc != 0)
            {
                // crc check failed
                status = kArdPacketStatusCrcFailed;
            }
        }

        if (kArdPacketStatusStart == status)
        {
            payload_index = header_and_crc_size;
            status = kArdPacketStatusDone;
        }
    }

    return status;
}

#endif
