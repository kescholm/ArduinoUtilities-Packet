

#ifndef ARD_PACKET_H
#define ARD_PACKET_H

#include <cstdint>
#include <cstring>

#include <Arduino.h>

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
 * @brief Status of ReceivePayload
 */
enum eArdPacketReadStatus
{
    kArdPacketRead = 0,
    kArdPacketReadNotConfigured,
    kArdPacketReadNotAvailable,
    kArdPacketReadFailed,
    kArdPacketReadNoDelimiter,
    kArdPacketReadInvalidPayloadSize,
    kArdPacketReadHeaderInProgress,
    kArdPacketReadHeaderCrcFailed,
    kArdPacketReadPayloadInProgress,
    kArdPacketReadPayloadCrcFailed,
    kArdPacketReadPayloadReady
};

/**
 * @brief Status of GetPayload
 */
// enum eArdPacketPayloadStatus
// {
//     kArdPacketPayloadSuccess = 0,
//     kArdPacketPayloadNotReady,
//     kArdPacketPayloadExceedsOuputSize
// };

/**
 * @brief Status of GeneratePacketHeader
 */
enum eArdPacketGenerateStatus
{
    kArdPacketGenerateSuccess = 0,
    kArdPacketGenerateInvalidMessageType,
    kArdPacketGenerateExceedsMaxPayloadSize,
    kArdPacketGenerateExceedsMaxPacketSize
};

class ArdPacket
{
   public:
    ArdPacket() = default;
    explicit ArdPacket(Stream &stream) : m_stream(stream) {}

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
    eArdPacketReadStatus ReceivePayload(size_t max_payload_size, uint8_t *payload, ArdPacketPayloadInfo &info);

    /**
     * @brief Reset state
     */
    void ResetReadState();

    /**
     * @brief Reset state
     */
    void ResetWriteState();

    // /**
    //  * @brief Copy prepared payload to external buffer
    //  *
    //  * @param buf
    //  * @param max_size
    //  * @return
    //  */
    // eArdPacketPayloadStatus GetPayload(size_t payload_max_size, uint8_t *payload, size_t &payload_size,
    //                                    uint32_t &message_type);

    /**
     * @brief Copy payload into packet buffer
     *
     * @param buf
     * @param max_size
     * @return
     */
    eArdPacketGenerateStatus GeneratePacketHeader(uint32_t message_type, size_t payload_size,
                                                       const uint8_t *payload, size_t max_packet_size,
                                                       uint8_t *packet, size_t &packet_size) const;

   private:

    static constexpr size_t kArdPacketMaxPayloadSizeBytes = 4;
    static constexpr size_t kArdPacketMaxMessageTypeBytes = 4;
    static constexpr size_t kArdPacketMaxHeaderSize = 1 + kArdPacketMaxPayloadSizeBytes + kArdPacketMaxMessageTypeBytes + 2 * sizeof(crc_t);

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

    enum eArdPacketWriteState
    {
        kArdPacketWriteStateHeader,
        kArdPacketWriteStatePayload,
        kArdPacketWriteStatePrepared
    };

    eArdPacketReadStatus ProcessReadStateDelimiter();
    eArdPacketReadStatus ProcessReadStateCrc();
    eArdPacketReadStatus ProcessReadStateMessageType(ArdPacketPayloadInfo &info);
    eArdPacketReadStatus ProcessReadStatePayloadSize(ArdPacketPayloadInfo &info);
    eArdPacketReadStatus ProcessReadStatePayload(const ArdPacketPayloadInfo &info, size_t max_payload_size, uint8_t *payload);

    // configuration
    ArdPacketConfig m_config = {};
    size_t m_max_message_type = 0;

    // read data
    eArdPacketReadState m_read_state = kArdPacketStateDelimiter;
    size_t m_read_payload_index = 0;
    crc_t m_read_crc = 0;

    // write data
    eArdPacketWriteState m_write_state = kArdPacketWriteStateHeader;
    size_t m_write_payload_index = 0;
    crc_t m_write_crc = 0;

    // stream interface
    Stream &m_stream = Serial;
};

// size_t ard_serial_write_noblock(const uint8_t * buf, uint16_t len)
// {
//     const uint16_t bytes_available = Serial.availableForWrite();
//     const uint16_t bytes_to_write = (len < bytes_available ? len : bytes_available);
//     return Serial.write(buf, bytes_to_write);
// }


// inline methods

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
        header_size = 1 + config.message_type_bytes + config.payload_size_bytes;
        header_and_crc_size = header_size;
        if (config.crc)
        {
            header_size += sizeof(crc_t);
            header_and_crc_size += 2 * sizeof(crc_t);
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
            m_max_message_type = UINT8_MAX;
        }
        else if (config.message_type_bytes == 2)
        {
            m_max_message_type = UINT16_MAX;
        }
        else if (config.message_type_bytes == 4)
        {
            m_max_message_type = UINT32_MAX;
        }

        m_config = config;

        ResetReadState();
        ResetWriteState();
    }

    return status;
}

inline void ArdPacket::ResetReadState()
{
    m_read_state = kArdPacketStateDelimiter;
    m_read_payload_index = 0;
}

inline void ArdPacket::ResetWriteState()
{
    m_write_state = kArdPacketWriteStateHeader;
    m_write_payload_index = 0;
}

inline eArdPacketReadStatus ArdPacket::ProcessReadStateDelimiter()
{
    eArdPacketReadStatus status = kArdPacketRead;
    const int read_size = m_stream.available();
    if (read_size <= 0)
    {
        status = kArdPacketReadNotAvailable;
    }
    else
    {
        bool found_delimiter = false;
        bool read_failed = false;
        for (size_t k = 0; k < read_size && (!found_delimiter) && (!read_failed); ++k)
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

        if (found_delimiter)
        {
            status = kArdPacketReadHeaderInProgress;
            m_read_state = kArdPacketStateMessageType;
            if (m_config.crc)
            {
                // initial crc for header
                m_read_crc = crc_init();
                m_read_crc = crc_update(m_read_crc, &m_config.delimiter, 1);
            }
        }
        else if (read_failed)
        {
            status = kArdPacketReadFailed;
        }
        else
        {
            status = kArdPacketReadNoDelimiter;
        }
    }

    return status;
}

inline eArdPacketReadStatus ArdPacket::ProcessReadStateMessageType(ArdPacketPayloadInfo &info)
{
    eArdPacketReadStatus status = kArdPacketRead;
    const int read_size = m_stream.available();
    if (read_size < m_config.message_type_bytes)
    {
        status = kArdPacketReadNotAvailable;
    }
    else
    {
        uint8_t read_data[kArdPacketMaxMessageTypeBytes];
        bool read_failed = false;
        for (size_t k = 0; k < m_config.message_type_bytes && (!read_failed); ++k)
        {
            const int read_byte = m_stream.read();
            if (read_byte < 0)
            {
                read_failed = true;
            }
            else
            {
                read_data[k] = static_cast<uint8_t>(read_byte);
            }
        }

        if (read_failed)
        {
            status = kArdPacketReadFailed;
            ResetReadState();
        }
        else
        {
            if (m_config.crc)
            {
                m_read_crc = crc_update(m_read_crc, read_data, m_config.message_type_bytes);
            }
            // copy message type from data
            // host endian copy (TODO: ensure consistent endianness)
            info.message_type = 0;
            memcpy(&info.message_type, read_data, m_config.message_type_bytes);
            // advance state
            status = kArdPacketReadHeaderInProgress;
            m_read_state = kArdPacketStatePayloadSize;
        }
    }
    return status;
}

eArdPacketReadStatus ArdPacket::ProcessReadStatePayloadSize(ArdPacketPayloadInfo &info)
{
    eArdPacketReadStatus status = kArdPacketRead;
    const int read_size = m_stream.available();
    if (read_size < m_config.payload_size_bytes)
    {
        status = kArdPacketReadNotAvailable;
    }
    else
    {
        uint8_t read_data[kArdPacketMaxPayloadSizeBytes];
        bool read_failed = false;
        for (size_t k = 0; k < m_config.payload_size_bytes && (!read_failed); ++k)
        {
            const int read_byte = m_stream.read();
            if (read_byte < 0)
            {
                read_failed = true;
            }
            else
            {
                read_data[k] = static_cast<uint8_t>(read_byte);
            }
        }


        if (read_failed)
        {
            status = kArdPacketReadFailed;
            ResetReadState();
        }
        else
        {
            if (m_config.crc)
            {
                m_read_crc = crc_update(m_read_crc, read_data, m_config.payload_size_bytes);
            }
            // copy from data to packet
            // host endian copy (TODO: ensure consistent endianness)
            memcpy(&info.payload_size, read_data, m_config.payload_size_bytes);
            // check payload size
            if (info.payload_size == 0 || (info.payload_size > m_config.max_payload_size))
            {
                status = kArdPacketReadInvalidPayloadSize;
                ResetReadState();
            }
            else
            {
                // advance state
                status = (m_config.crc ? kArdPacketReadHeaderInProgress : kArdPacketReadPayloadInProgress);
                m_read_state = (m_config.crc ? kArdPacketStateHeaderCrc : kArdPacketStatePayload);
            }
        }
    }
    return status;
}

eArdPacketReadStatus ArdPacket::ProcessReadStatePayload(const ArdPacketPayloadInfo &info, const size_t max_payload_size, uint8_t *payload)
{
    eArdPacketReadStatus status = kArdPacketRead;
    const int read_size = m_stream.available();
    if (read_size <= 0)
    {
        status = kArdPacketReadNotAvailable;
    }
    else if (max_payload_size < info.payload_size)
    {
        status = kArdPacketReadInvalidPayloadSize;
        ResetReadState();
    }
    else
    {
        status = kArdPacketReadPayloadInProgress;
        const size_t remaining_payload = info.payload_size - m_read_payload_index;
        const size_t bytes_to_read = (remaining_payload < read_size ? remaining_payload : read_size);
        bool read_failed = false;
        for (size_t k = 0; k < bytes_to_read && (!read_failed); ++k)
        {
            const int read_byte = m_stream.read();
            if (read_byte < 0)
            {
                read_failed = true;
            }
            else
            {
                payload[m_read_payload_index] = static_cast<uint8_t>(read_byte);
                m_read_payload_index++;
            }
        }

        if (read_failed)
        {
            status = kArdPacketReadFailed;
            ResetReadState();
        }
        else if (m_read_payload_index == info.payload_size)
        {
            status = (m_config.crc ? kArdPacketReadPayloadInProgress : kArdPacketReadPayloadReady);
            m_read_state = (m_config.crc ? kArdPacketStatePayloadCrc : kArdPacketStatePrepared);
        }
    }
    return status;
}

eArdPacketReadStatus ArdPacket::ProcessReadStateCrc()
{

    eArdPacketReadStatus status = kArdPacketRead;
    const int read_size = m_stream.available();
    if (read_size < sizeof(crc_t))
    {
        status = kArdPacketReadNotAvailable;
    }
    else
    {
        uint8_t read_data[sizeof(crc_t)];
        bool read_failed = false;
        for (size_t k = 0; k < sizeof(crc_t) && (!read_failed); ++k)
        {
            const int read_byte = m_stream.read();
            if (read_byte < 0)
            {
                read_failed = true;
            }
            else
            {
                read_data[k] = static_cast<uint8_t>(read_byte);
            }
        }

        if (read_failed)
        {
            status = kArdPacketReadFailed;
            ResetReadState();
        }
        else
        {
            // crc from data
            m_read_crc = crc_update(m_read_crc, read_data, sizeof(crc_t));
            // finalize and test
            m_read_crc = crc_finalize(m_read_crc);
            // check payload crc
            if (m_read_crc == 0)
            {
                // passed crc
                status = kArdPacketReadPayloadInProgress;
                m_read_state = kArdPacketStatePayload;
                // initial crc for payload
                m_read_crc = crc_init();
            }
            else
            {
                status = kArdPacketReadHeaderCrcFailed;
                ResetReadState();
            }
        }
    }
    return status;
}

inline eArdPacketReadStatus ArdPacket::ReceivePayload(const size_t max_payload_size, uint8_t *payload, ArdPacketPayloadInfo &info)
{
    eArdPacketReadStatus status = kArdPacketRead;
    if (m_config.max_payload_size == 0)
    {
        status = kArdPacketReadNotConfigured;
    }
    else
    {
        // state machine
        switch (m_read_state)
        {
            case kArdPacketStateDelimiter:
            {
                status = ProcessReadStateDelimiter();
                break;
            }
            case kArdPacketStateMessageType:
            {
                status = ProcessReadStateMessageType(info);
                break;
            }
            case kArdPacketStatePayloadSize:
            {
                status = ProcessReadStatePayloadSize(info);
                break;
            }
            case kArdPacketStateHeaderCrc:
            {
                status = ProcessReadStateCrc();
                break;
            }
            case kArdPacketStatePayload:
            {
                status = ProcessReadStatePayload(info, max_payload_size, payload);
                break;
            }
            case kArdPacketStatePayloadCrc:
            {
                status = ProcessReadStateCrc();
                break;
            }
            case kArdPacketStatePrepared:
            {
                status = kArdPacketReadPayloadReady;
                break;
            }
            default:
            {
                status = kArdPacketRead;
                ResetReadState();
                break;
            }
        }
    }

    return status;
}

// template <Stream &m_stream, Stream &m_stream>
// eArdPacketStreamStatus ArdPacketStream<m_stream, m_stream>::
//     ReceivePayload(size_t max_payload_size, uint8_t *payload,
//     size_t &payload_size, eArdPacketReadStatus &read_status)
// {

// // inline eArdPacketPayloadStatus ArdPacket::GetPayload(const size_t payload_max_size, uint8_t *payload,
//                                                                         size_t &payload_size, uint32_t &message_type)
// {
//     eArdPacketPayloadStatus result = kArdPacketPayloadSuccess;
//     if (m_read_state != kArdPacketStatePrepared)
//     {
//         result = kArdPacketPayloadNotReady;
//     }
//     else if (info.payload_size > payload_max_size)
//     {
//         result = kArdPacketPayloadExceedsOuputSize;
//     }
//     else
//     {
//         memcpy(payload, m_payload, info.payload_size);
//         payload_size = info.payload_size;
//         message_type = m_read_message_type;
//         Reset();
//     }
//     return result;
// }

// inline eArdPacketGenerateStatus ArdPacket::GeneratePacketHeader(
//     const uint32_t message_type, const size_t payload_size, const uint8_t *payload, const size_t max_packet_size, uint8_t *packet,
//     size_t &packet_size) const
// {
//     eArdPacketGenerateStatus status = kArdPacketGenerateSuccess;
//     if (message_type > m_max_message_type)
//     {
//         status = kArdPacketGenerateInvalidMessageType;
//     }
//     else if (payload_size > m_config.max_payload_size)
//     {
//         status = kArdPacketGenerateExceedsMaxPayloadSize;
//     }
//     else if (payload_size > max_packet_size)
//     {
//         status = kArdPacketGenerateExceedsMaxPacketSize;
//     }
//     else
//     {
//         const size_t header_size = 1 + m_config.message_type_bytes + m_config.payload_size_bytes;
//         const size_t header_and_crc_size = header_size + (m_config.crc ? 2 * sizeof(crc_t) : 0.0);
//         if ((max_packet_size - payload_size) < header_and_crc_size)
//         {
//             status = kArdPacketGenerateExceedsMaxPacketSize;
//         }
//         else
//         {
//             // Create packet

//             // delimiter
//             size_t packet_index = 0;
//             packet[packet_index] = m_config.delimiter;
//             packet_index += 1;

//             // message type
//             // TODO: Choose endianness
//             memcpy(&packet[packet_index], &message_type, m_config.message_type_bytes);
//             packet_index += m_config.message_type_bytes;

//             // payload size
//             // TODO: Choose endianness
//             memcpy(&packet[packet_index], &payload_size, m_config.payload_size_bytes);
//             packet_index += m_config.payload_size_bytes;

//             // header crc
//             if (m_config.crc)
//             {
//                 ard_crc16_kermit_append(packet, header_size);
//                 packet_index += sizeof(crc_t);
//             }

//             // payload
//             memcpy(&packet[packet_index], payload, payload_size);

//             // payload crc
//             if (m_config.crc)
//             {
//                 ard_crc16_kermit_append(&packet[packet_index], payload_size);
//             }

//             // done
//             packet_size = header_and_crc_size + payload_size;
//         }
//     }

//     return status;
// }

#endif
