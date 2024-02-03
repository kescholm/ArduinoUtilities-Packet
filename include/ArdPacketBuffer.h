
#ifndef ARD_PACKET_BUFFER_H
#define ARD_PACKET_BUFFER_H

#include <Arduino.h>

#include "ArdPacket.h"

class ArdPacketBuffer : public ArdPacketStreamInterface
{
   public:
    ArdPacketBuffer() = default;

    bool set_read_buffer(uint8_t *buffer, size_t buffer_size)
    {
        if (buffer != nullptr)
        {
            m_read_buffer = buffer;
            m_read_size = buffer_size;
            m_read_index = 0;
            return true;
        }
        else
        {
            m_read_buffer = nullptr;
            m_read_size = 0;
            m_read_index = 0;
            return false;
        }
    }

    bool set_write_buffer(uint8_t *buffer, size_t buffer_size)
    {
        if (buffer != nullptr)
        {
            m_write_buffer = buffer;
            m_write_size = buffer_size;
            m_write_index = 0;
            return true;
        }
        else
        {
            m_write_buffer = nullptr;
            m_write_size = 0;
            m_write_index = 0;
            return false;
        }
    }

    int available() override
    {
        return m_read_size - m_read_index;
    }
    int read() override
    {
        int retval = -1;
        const size_t read_remaining = m_read_size - m_read_index;
        if (read_remaining > 0)
        {
            retval = m_read_buffer[m_read_index];
            m_read_index++;
        }
        return retval;
    }

    size_t read(uint8_t *buffer, size_t size) override
    {
        const size_t read_remaining = m_read_size - m_read_index;
        const size_t read_size = (size < read_remaining ? size : read_remaining);
        if (read_size > 0)
        {
            memcpy(buffer, &m_read_buffer[m_read_index], read_size);
            m_read_index = m_read_index + read_size;
        }
        return read_size;
    }

    int availableForWrite() override
    {
        return m_write_size - m_write_index;
    }
    size_t write(uint8_t value) override
    {
        size_t retval = 0;
        if ((m_write_size - m_write_index) > 0)
        {
            m_write_buffer[m_write_index] = value;
            m_write_index++;
            retval = 1;
        }
        return retval;
    }
    size_t write(const uint8_t *buffer, size_t size) override
    {
        const size_t available_write = (m_write_size - m_write_index);
        const size_t write_size = (size < available_write ? size : available_write);
        if (write_size > 0)
        {
            memcpy(&m_write_buffer[m_write_index], buffer, write_size);
            m_write_index = m_write_index + write_size;
        }
        return write_size;
    }

   private:

    uint8_t *m_read_buffer = nullptr;
    size_t m_read_size = 0;
    size_t m_read_index = 0;

    uint8_t *m_write_buffer = nullptr;
    size_t m_write_size = 0;
    size_t m_write_index = 0;

};

#endif
