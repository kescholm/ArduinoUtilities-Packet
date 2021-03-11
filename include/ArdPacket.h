
/**
 * @file       ArdPacket.h
 * @author     Kyle Chisholm (dev@kylechisholm.ca)
 * @brief      Serial packet protocol
 *
 * @details
 *
 * See group @ref ArdPacket
 *
 */

#ifndef ARD_PACKET_H
#define ARD_PACKET_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ARD_PACKET_MAX_WRITE_COUNT 20

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup   ArdPacket Serial packet protocol
 * @brief      Read and write serial packets with header and CRC
 *
 * @{
 *
 */

// /**
//  * @brief      read until a specified byte (delimiter)
//  */
// typedef int (*pArdReadUntilCallback)(uint8_t, uint16_t);

// /**
//  * @brief      read all requested bytes into buffer
//  */
// typedef uint16_t (*pArdReadCallback)(uint8_t *, const uint16_t);

// /**
//  * @brief      write entire byte array to serial buffer
// */
// typedef size_t (*pArdWriteCallback)(const uint8_t *, const uint16_t);

/**
 * @brief Read packet from serial state machine
 *
 */
typedef enum eArdPacketReadState {
    ARD_PACKET_READ_DELIMITER,
    ARD_PACKET_READ_SIZE,
    ARD_PACKET_READ_DATA,
    ARD_PACKET_READ_PREPARED
} eArdPacketReadState;

/**
 * @brief Status of read operation
 *
 */
typedef enum eArdPacketReadStatus {
    ARD_PACKET_READ_NONE,
    ARD_PACKET_READ_NO_DELIMITER,
    ARD_PACKET_READ_SIZE_FAILED,
    ARD_PACKET_READ_IN_PROGRESS,
    ARD_PACKET_READ_CRC_FAILED,
    ARD_PACKET_READ_SUCCESS
} eArdPacketReadStatus;

/**
 * @brief Write packet to serial state machine
 *
 */
typedef enum eArdPacketWriteState {
    ARD_PACKET_WRITE_IDLE,
    ARD_PACKET_WRITE_PREPARED,
    ARD_PACKET_WRITE_DATA,
    ARD_PACKET_WRITE_DONE
} eArdPacketWriteState;

/**
 * @brief Status of write operation to serial
 *
 */
typedef enum eArdPacketWriteStatus {
    ARD_PACKET_WRITE_NONE,
    ARD_PACKET_WRITE_DATA_EMPTY,
    ARD_PACKET_WRITE_IN_PROGRESS,
    ARD_PACKET_WRITE_SUCCESS
} eArdPacketWriteStatus;

/**
 * Serial communications buffer container.
 */
typedef struct ArdSerialPacket {
    /**
     * @brief Full data packet including header and crc
     *
     */
    uint8_t *packet;
    /**
     * @brief Data buffer payload in packet
     *
     */
    uint8_t *data;
    /**
     * @brief Size of data payload
     *
     */
    uint16_t size;
    /**
     * @brief Maximum size of data payload
     *
     */
    uint16_t max_size;
    /**
     * @brief Packet delimiter
     *
     */
    uint8_t delimiter;
    /**
     * @brief Option to use CRC
     *
     */
    bool crc;

    /**
     * @brief Internal read/write index
     *
     */
    uint16_t index;
    /**
     * @brief Internal remaining bytes to read/write
     *
     */
    uint16_t remaining_bytes;

    union uArdPackectState {
        /**
         * @brief Serial read state
         *
         */
        eArdPacketReadState read;

        /**
         * @brief Serial write state
         *
         */
        eArdPacketWriteState write;
    } state;

} ArdSerialPacket;

/**
 * @brief      allocates memory for serial packet ArdSerialPacket
 * @details    The serial packet byte array packing is as follows
 *
 *             | bytes                   | description                     |
 *             |-------------------------|---------------------------------|
 *             |  0                      | delimiter                       |
 *             |  1 to 2                 | uint16_t number of bytes (size) |
 *             |  3 to (3+size-1)        | data                            |
 *             |  (3+size) to (3+size+1) | appended crc                    |
 *
 * @param      serial     serial packet to allocate memory to
 * @param[in]  size       size of data to allocate
 * @param[in]  delimiter  delimiter for readign and writing serial packets
 *
 * @return     0 on success, -1 on failure to allocate memory
 */
int ard_packet_alloc(ArdSerialPacket *serial, const uint16_t max_size, const uint8_t delimiter,
                     const bool use_crc);

/**
 * @brief Free memory allocated to serial packet
 *
 * @param serial serial packet to free
 */
void ard_packet_free(ArdSerialPacket *serial);

/**
 * @brief Copy buffer to packet data
 *
 * @param serial Serial packet object
 * @param buf buffer to copy to serial object
 * @return number of bytes copied
 */
uint16_t ard_packet_copy_from_buffer(ArdSerialPacket *serial, const uint8_t *buf,
                                     const uint16_t size);

/**
 * @brief Copy from serial packet to buffer
 *
 * @param serial Serial packet object
 * @param buf buffer to receive data from
 * @param size size of buf
 * @return number of bytes copied
 */
uint16_t ard_packet_copy_to_buffer(const ArdSerialPacket *serial, uint8_t *buf,
                                   const uint16_t size);

/**
 * @brief      { function_description }
 *
 * @param      serial  The serial
 * @param[in]  size    The size
 *
 * @return     if >= 0 it is number of bytes remaining to write, error code otherwise
 */
eArdPacketWriteStatus ard_packet_write(ArdSerialPacket *serial);

/**
 * @brief      { function_description }
 *
 * @param      serial     The serial
 * @param[in]  fixed_len  The fixed length
 * @param[out] data_read_index current index of serial->data buffer for next read (if any read
 * bytes remaining)
 *
 * @return     if >= 0 it is number of bytes remaining to read, error code otherwise
 */
eArdPacketReadStatus ard_packet_read(ArdSerialPacket *serial);

// Serial
// ------

/**
 * @brief      read all requested bytes into buffer
 * @details    There is no check if the requested number of bytes
 *             is greater than the serial buffer and will always fail in this case.
 *
 * @param      buf   byte array
 * @param[in]  len   length of byte array
 *
 * @return     Returns number of bytes read into buffer
 *
 */
uint16_t ard_serial_read_noblock(uint8_t *buf, const uint16_t len);

/**
 * @brief      read until a specified byte (delimiter)
 * @details    The serial buffer is consumed and read until the desired byte is
 *             consumed, there are no more bytes available to read, or the
 *             maximum number of bytes read is reached
 *
 * @param[in]  delimiter  desired byte to read until
 * @param[in]  max_tries  maximum number of bytes to read
 * @param[out] bytes_read number of bytes read
 *
 * @return     @c true if delimiter found, @ false otherwise
 */
int ard_serial_read_noblock_until(const uint8_t delimiter, const uint16_t max_tries);

/**
 * @brief      write entire byte array to serial buffer
 * @details    This will only attempt to write if all @c len bytes are available to
 *             write to the serial buffer.
 *
 * @param[in]  buf   byte array to send
 * @param[in]  len   length of byte array
 *
 * @return     returns number of bytes sent or -1 if not enough bytes available to start write
 */
size_t ard_serial_write_noblock(const uint8_t *buf, uint16_t len);

/**
 * @brief      setup Arduino serial communications
 * @details    A wrapper for Arduino C++ functions Serial.begin(baud_rate)
 *
 * @param[in]  baud_rate  baud rate for serial communications
 */
void ard_serial_begin(const unsigned long baud_rate);

/**
 * @brief      flush serial write buffer
 */
void ard_serial_write_flush(void);

/**
 * @brief      flush serial read buffer
 */
void ard_serial_read_flush(void);

void dada() {
    bool use_crc = true;
    uint8_t delimiter = '|';
}
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
