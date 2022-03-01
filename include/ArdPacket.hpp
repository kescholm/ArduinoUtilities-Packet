

#ifndef ARD_PACKET_HPP
#define ARD_PACKET_HPP

#include <cstring>
#include <cstdint>

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
 * @brief Status of Configure
 */
enum eArdPacketConfigStatus
{
    kArdPacketConfigSuccess = 0,
    kArdPacketConfigInvalidMessageTypeBytes,
    kArdPacketConfigInvalidPayloadSizeBytes,
    kArdPacketConfigInvalidMaxPayloadSize,
    kArdPacketConfigExceedsPacketMaxSize
};

/**
 * @brief Status of ProcessReceivedData
 */
enum eArdPacketReadStatus
{
    kArdPacketReadNone,
    kArdPacketReadNotConfigured,
    kArdPacketReadNotEnoughData,
    kArdPacketReadNoDelimiter,
    kArdPacketReadHeaderInProgress,
    kArdPacketReadHeaderCrcFailed,
    kArdPacketReadPayloadInProgress,
    kArdPacketReadPayloadCrcFailed,
    kArdPacketReadPayloadReady
};

template <size_t PacketMaxSize>
class ArdPacketRead
{
   public:
    ArdPacketRead() = default;

    /**
     * @brief Configure packet
     *
     * @param config
     * @return
     */
    eArdPacketConfigStatus Configure(const ArdPacketConfig &config);

    /**
     * @brief Ingest data chunk from buffer
     *
     * @param data
     * @param size
     * @return
     */
    eArdPacketReadStatus ProcessReceivedData(const uint8_t *data, size_t size, size_t &bytes_read);

    /**
     * @brief Reset state
     */
    void Reset();

    /**
     * @brief Copy payload
     *
     * @param buf
     * @param max_size
     * @return
     */
    bool GetPayload(size_t payload_max_size, uint8_t *payload, size_t &payload_size, uint32_t &message_type);

   private:
    enum eArdPacketReadState
    {
        kArdPacketStateDelimiter,
        kArdPacketStateMessageType,
        kArdPacketStatePayloadSize,
        kArdPacketStateHeaderCrc,
        kArdPacketStatePayload,
        kArdPacketStatePayloadCrc,
        kArdPacketStatePrepared
    };

    // configuration
    ArdPacketConfig m_config = {};
    // raw data
    uint8_t m_packet[PacketMaxSize] = {};

    // state
    eArdPacketReadState m_state = kArdPacketStateDelimiter;
    size_t m_payload_index = 0;

    // received packet
    uint32_t m_message_type = 0;
    size_t m_payload_size = 0;
    uint8_t *m_payload = nullptr;
};

// inline methods

template <size_t PacketMaxSize>
inline eArdPacketConfigStatus ArdPacketRead<PacketMaxSize>::Configure(const ArdPacketConfig &config)
{
    eArdPacketConfigStatus status = kArdPacketConfigSuccess;

    if (config.message_type_bytes != 1 && config.message_type_bytes != 2 && config.message_type_bytes != 4)
    {
        status = kArdPacketConfigInvalidMessageTypeBytes;
    }

    if (status == kArdPacketConfigSuccess &&
        config.payload_size_bytes != 1 && config.payload_size_bytes != 2 && config.payload_size_bytes != 4)
    {
        status = kArdPacketConfigInvalidPayloadSizeBytes;
    }

    size_t header_size = 0;
    size_t header_and_crc_size = 0;
    if (status == kArdPacketConfigSuccess)
    {
        header_size = 1 + config.message_type_bytes + config.payload_size_bytes;
        header_and_crc_size = header_size;
        if (config.crc)
        {
            header_size += sizeof(crc_t);
            header_and_crc_size += 2 * sizeof(crc_t);
        }
        if (PacketMaxSize <= header_and_crc_size)
        {
            status = kArdPacketConfigExceedsPacketMaxSize;
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
            max_payload_limit = UINT32_MAX;
        }

        if (config.max_payload_size > (PacketMaxSize - header_and_crc_size) ||
            config.max_payload_size > max_payload_limit)
        {
            status = kArdPacketConfigInvalidMaxPayloadSize;
        }
    }

    if (status == kArdPacketConfigSuccess)
    {
        m_config = config;
        memset(m_packet, 0, PacketMaxSize);
        m_packet[0] = m_config.delimiter;
        m_payload = &m_packet[header_size];

        m_state = kArdPacketStateDelimiter;
        m_payload_index = 0;

        m_message_type = 0;
        m_payload_size = 0;
    }

    return status;
}

template <size_t PacketMaxSize>
inline void ArdPacketRead<PacketMaxSize>::Reset()
{
    m_state = kArdPacketStateDelimiter;
    m_payload_index = 0;

    m_message_type = 0;
    m_payload_size = 0;
}

template <size_t PacketMaxSize>
inline eArdPacketReadStatus ArdPacketRead<PacketMaxSize>::ProcessReceivedData(const uint8_t *data, const size_t size,
                                                                              size_t &bytes_read)
{
    eArdPacketReadStatus status = kArdPacketReadNone;
    if (m_payload == nullptr)
    {
        status = kArdPacketReadNotConfigured;
    }
    else if (size == 0)
    {
        status = kArdPacketReadNotEnoughData;
    }
    else
    {
        // state machine
        switch (m_state)
        {
            case kArdPacketStateDelimiter:
            {
                bool found_delimiter = false;
                while (bytes_read < size && !found_delimiter)
                {
                    found_delimiter = (data[bytes_read] == m_config.delimiter);
                    bytes_read++;
                }
                if (found_delimiter)
                {
                    status = kArdPacketReadHeaderInProgress;
                    m_state = kArdPacketStateMessageType;
                }
                else
                {
                    status = kArdPacketReadNoDelimiter;
                }
                break;
            }
            case kArdPacketStateMessageType:
            {
                if ((size - bytes_read) >= m_config.message_type_bytes)
                {
                    size_t index = 1;
                    // copy from data to packet
                    memcpy(&m_packet[index], &data[bytes_read], m_config.message_type_bytes);
                    bytes_read += m_config.message_type_bytes;
                    // host endian copy (TODO: ensure consistent endianness)
                    memcpy(&m_message_type, &m_packet[index], m_config.message_type_bytes);
                    // advance state
                    status = kArdPacketReadHeaderInProgress;
                    m_state = kArdPacketStatePayloadSize;
                }
                else
                {
                    status = kArdPacketReadNotEnoughData;
                }
                break;
            }
            case kArdPacketStatePayloadSize:
            {
                if ((size - bytes_read) >= m_config.payload_size_bytes)
                {
                    size_t index = 1 + m_config.message_type_bytes;
                    // copy from data to packet
                    memcpy(&m_packet[index], &data[bytes_read], m_config.payload_size_bytes);
                    bytes_read += m_config.payload_size_bytes;
                    // host endian copy (TODO: ensure consistent endianness)
                    memcpy(&m_payload_size, &m_packet[index], m_config.payload_size_bytes);
                    // advance state
                    status = (m_config.crc ? kArdPacketReadHeaderInProgress : kArdPacketReadPayloadInProgress);
                    m_state = (m_config.crc ? kArdPacketStateHeaderCrc : kArdPacketStatePayload);
                }
                else
                {
                    status = kArdPacketReadNotEnoughData;
                }
                break;
            }
            case kArdPacketStateHeaderCrc:
            {
                if ((size - bytes_read) >= sizeof(crc_t))
                {
                    // copy from data to packet
                    const size_t header_size = 1 + m_config.message_type_bytes + m_config.message_type_bytes;
                    memcpy(&m_packet[header_size], &data[bytes_read], sizeof(crc_t));
                    bytes_read += sizeof(crc_t);
                    // check header crc
                    if (ard_crc16_kermit_test(m_packet, header_size + sizeof(crc_t)))
                    {
                        // passed crc
                        status = kArdPacketReadPayloadInProgress;
                        m_state = kArdPacketStatePayload;
                    }
                    else
                    {
                        status = kArdPacketReadHeaderCrcFailed;
                        Reset();
                    }
                }
                else
                {
                    status = kArdPacketReadNotEnoughData;
                }
                break;
            }
            case kArdPacketStatePayload:
            {
                status = kArdPacketReadPayloadInProgress;
                // copy from data to payload
                const size_t remaining_payload = m_payload_size - m_payload_index;
                const size_t available_data = (size - bytes_read);
                const size_t bytes_to_copy = (remaining_payload < available_data ? remaining_payload : available_data);
                if (bytes_to_copy > 0)
                {
                    memcpy(&m_payload[m_payload_index], &data[bytes_read], bytes_to_copy);
                    bytes_read += bytes_to_copy;
                    m_payload_index += bytes_to_copy;
                }
                if (m_payload_index == m_payload_size)
                {
                    status = (m_config.crc ? kArdPacketReadPayloadInProgress :kArdPacketReadPayloadReady);
                    m_state = (m_config.crc ? kArdPacketStatePayloadCrc : kArdPacketStatePrepared);
                }
                break;
            }
            case kArdPacketStatePayloadCrc:
            {
                status = kArdPacketReadPayloadInProgress;
                if ((size - bytes_read) >= sizeof(crc_t))
                {
                    // copy from data to payload
                    memcpy(&m_payload[m_payload_size], &data[bytes_read], sizeof(crc_t));
                    bytes_read += sizeof(crc_t);
                    // check payload crc
                    if (ard_crc16_kermit_test(m_payload, m_payload_size + sizeof(crc_t)))
                    {
                        // passed crc
                        status = kArdPacketReadPayloadReady;
                        m_state = kArdPacketStatePrepared;
                    }
                    else
                    {
                        status = kArdPacketReadPayloadCrcFailed;
                        Reset();
                    }
                }
                break;
            }
            case kArdPacketStatePrepared:
            {
                status = kArdPacketReadPayloadReady;
                break;
            }
            default:
            {
                status = kArdPacketReadNone;
                m_state = kArdPacketStateDelimiter;
                break;
            }
        }
    }

    return kArdPacketReadNone;
}

template <size_t PacketMaxSize>
inline bool ArdPacketRead<PacketMaxSize>::GetPayload(const size_t payload_max_size, uint8_t *payload,
                                                     size_t &payload_size, uint32_t &message_type)
{
    bool result = false;
    if (m_state == kArdPacketStatePrepared && m_payload_size <= payload_max_size)
    {
        memcpy(payload, m_payload, m_payload_size);
        payload_size = m_payload_size;
        message_type = m_message_type;
        Reset();
        result = true;
    }
    return result;
}

#endif
