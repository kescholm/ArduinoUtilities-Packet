#include <stdint.h>

#include "ArdPacket.h"
#include "ArdCrc.h"

// Static Declarations
//--------------------

static void ard_packet_write_serial(ArdSerialPacket *serial, uint16_t *data_index,
                                  uint16_t *bytes_remaining);

static int ard_packet_read_serial(ArdSerialPacket *serial, uint16_t *data_read_index,
                                uint16_t *bytes_remaining);

// Static definitions: Serial interface
//-------------------------------------

static void ard_packet_write_serial(ArdSerialPacket *serial, uint16_t *data_index,
                                  uint16_t *bytes_remaining)
{
    const uint16_t write_len =
        ard_serial_write_noblock(&serial->packet[(*data_index)], (*bytes_remaining));
    (*data_index) += write_len;
    (*bytes_remaining) -= write_len;
}

static int ard_packet_read_serial(ArdSerialPacket *serial, uint16_t *data_read_index,
                                uint16_t *bytes_remaining)
{
    const uint16_t read_len =
        ard_serial_read_noblock(&serial->data[(*data_read_index)], (*bytes_remaining));
    (*data_read_index) += read_len;
    return (*bytes_remaining) - read_len;
}

// Interface
//----------

/**
 * @brief      allocates memory for serial packet ArdSerialPacket
 * @details    The serial packet byte array packing is as follows
 *
 *             | bytes                   | description                     |
 *             |-------------------------|---------------------------------|
 *             |  0                      | delimiter                       |
 *             |  1 to 2                 | uint16_t number of bytes (size) |
 *             |  3 to (3+size-1)        | data            |
 *             |  (3+size) to (3+size+1) | appended crc                    |
 *
 * @param      serial     serial packet to alocate memory to
 * @param[in]  size       size of data to allocate
 * @param[in]  delimiter  delimiter for readign and writing serlial packets
 *
 * @return     0 on success, -1 on failure to allocate memory
 */
int ard_packet_alloc(ArdSerialPacket *serial, const uint16_t max_size, const uint8_t delimiter,
                     const bool use_crc)
{
    // packet format:
    //  0: delimiter
    //  1 to 2: uint16_t number of bytes (size)
    //  3 to (3+size-1): data
    //  (3+size) to (3+size+1): appended crc
    if (max_size > (UINT16_MAX - 5)) return -1;
    serial->packet = (uint8_t *)calloc(max_size + 5, sizeof(uint8_t));
    if (serial->packet == NULL)
    {
        serial->size = 0;
        serial->max_size = 0;
        serial->data = NULL;
        return -2;
    }

    // alias
    serial->data = &serial->packet[3];
    // initialize
    serial->delimiter = delimiter;
    serial->max_size = max_size;
    serial->crc = use_crc;

    // write info
    serial->index = 0;
    serial->remaining_bytes = 0;
    serial->state.read = ARD_PACKET_READ_DELIMITER;
    serial->state.write = ARD_PACKET_WRITE_IDLE;

    // write packet header
    serial->packet[0] = delimiter;
    // memcpy(&serial->packet[1], &max_size, 2);

    return 0;
}

void ard_packet_free(ArdSerialPacket *serial)
{
    free(serial->packet);
    serial->packet = NULL;
    serial->size = 0;
    serial->max_size = 0;
    serial->data = NULL;
}

uint16_t ard_packet_copy_from_buffer(ArdSerialPacket *serial, const uint8_t * buf,
                                 const uint16_t size)
{
    // copy to data in serial packet
    if (size > serial->max_size) {
        serial->size = 0;
    } else {
        memcpy(serial->data, buf, size);
        serial->size = size;
    }
    return serial->size;
}

uint16_t ard_packet_copy_to_buffer(const ArdSerialPacket *serial, uint8_t * buf,
                                 const uint16_t size)
{
    // copy from serial packet to buffer
    uint16_t copy_size = 0;
    if (size <= serial->size) {
        memcpy(buf, serial->data, serial->size);
        copy_size = serial->size;
    }
    return copy_size;
}

eArdPacketWriteStatus ard_packet_write(ArdSerialPacket *serial)
{
    eArdPacketWriteStatus status = ARD_PACKET_WRITE_NONE;

    // state machine
    if (serial->state.write == ARD_PACKET_WRITE_IDLE)
    {
        if (serial->size == 0)
        {
            // no data to write
            status = ARD_PACKET_WRITE_DATA_EMPTY;
        }
        else
        {
            // set size (little to big endian)
            // serial->packet[1] = (uint8_t)(serial->size >> 8);
            // serial->packet[2] = (uint8_t)(serial->size);
            // little endian copy
            memcpy(&serial->packet[1], &serial->size, sizeof(uint16_t));

            // build packet
            serial->remaining_bytes = serial->size + 3;
            if (serial->crc)
            {
                serial->remaining_bytes += 2;
                ard_crc16_kermit_append(serial->data, serial->size);
            }

            // write index
            serial->index = 0;

            // prepared state
            serial->state.write = ARD_PACKET_WRITE_PREPARED;
            status = ARD_PACKET_WRITE_IN_PROGRESS;
        }
    }
    else if (serial->state.write == ARD_PACKET_WRITE_PREPARED ||
             serial->state.write == ARD_PACKET_WRITE_DATA)
    {
        // write to serial
        ard_packet_write_serial(serial, &serial->index, &serial->remaining_bytes);
        if (serial->remaining_bytes > 0)
        {
            // not done
            serial->state.write = ARD_PACKET_WRITE_DATA;
            status = ARD_PACKET_WRITE_IN_PROGRESS;
        }
        else
        {
            // done - success
            serial->state.write = ARD_PACKET_WRITE_DONE;
            status = ARD_PACKET_WRITE_SUCCESS;
        }
    }
    else if (serial->state.write == ARD_PACKET_WRITE_DONE)
    {
        serial->state.write = ARD_PACKET_WRITE_IDLE;
        serial->index = 0;
        serial->size = 0;
        status = ARD_PACKET_WRITE_NONE;
    }
    return status;
}

eArdPacketReadStatus ard_packet_read(ArdSerialPacket *serial)
{
    eArdPacketReadStatus status = ARD_PACKET_READ_NONE;

    // state machine
    if (serial->state.read == ARD_PACKET_READ_DELIMITER)
    {
        int ret = ard_serial_read_noblock_until(serial->delimiter, serial->max_size + 5);
        if (ret > 0)
        {
            // found delimiter
            serial->state.read = ARD_PACKET_READ_SIZE;
            status = ARD_PACKET_READ_IN_PROGRESS;
        } else if (ret == 0) {
            // no bytes available
            status = ARD_PACKET_READ_NONE;
        } else {
            // not delimiter
            status = ARD_PACKET_READ_NO_DELIMITER;
        }
    }
    else if (serial->state.read == ARD_PACKET_READ_SIZE)
    {
        // length of buffer to read
        uint16_t bytes_read = ard_serial_read_noblock(&serial->packet[1], 2);
        if (bytes_read != 2)
        {
            // failed to read
            serial->state.read = ARD_PACKET_READ_DELIMITER;
            status = ARD_PACKET_READ_SIZE_FAILED;
        }
        else
        {
            // big-endian to little endian
            // serial->size = (serial->packet[1] << 8) | (serial->packet[2] >> 8);
            // little endian copy
            memcpy(&serial->size, &serial->packet[1], sizeof(uint16_t));
            if (serial->size > serial->max_size)
            {
                // desired size too large
                serial->size = 0;
                serial->state.read = ARD_PACKET_READ_DELIMITER;
                status = ARD_PACKET_READ_SIZE_FAILED;
            }
            else
            {
                // size ok
                serial->state.read = ARD_PACKET_READ_DATA;
                serial->index = 0;
                serial->remaining_bytes = serial->size;
                if (serial->crc) serial->remaining_bytes += 2;
                status = ARD_PACKET_READ_IN_PROGRESS;
            }
        }
    }
    else if (serial->state.read == ARD_PACKET_READ_DATA)
    {
        ard_packet_read_serial(serial, &serial->index, &serial->remaining_bytes);
        if (serial->remaining_bytes > 0)
        {
            // not done
        }
        else if (serial->crc)
        {
            // done read, check crc
            if (ard_crc16_kermit_test(serial->data, serial->size))
            {
                // passed crc
                status = ARD_PACKET_READ_SUCCESS;
                serial->state.read = ARD_PACKET_READ_PREPARED;
            }
            else
            {
                // failed crc
                status = ARD_PACKET_READ_CRC_FAILED;
                serial->size = 0;
                serial->state.read = ARD_PACKET_READ_DELIMITER;
            }
        }
        else
        {
            //  done read, no crc
            status = ARD_PACKET_READ_SUCCESS;
            serial->state.read = ARD_PACKET_READ_PREPARED;
        }
    }
    else if (serial->state.read == ARD_PACKET_READ_PREPARED)
    {
        // done - restart read
        serial->size = 0;
        serial->state.read = ARD_PACKET_READ_DELIMITER;
        status = ARD_PACKET_READ_NONE;
    }

    return status;
}
