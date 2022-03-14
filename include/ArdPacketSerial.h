
#ifndef ARD_PACKET_SERIAL_H
#define ARD_PACKET_SERIAL_H

#include <Arduino.h>

#include "ArdPacket.h"

class ArdPacketSerial : public ArdPacketStreamInterface
{
   public:
    ArdPacketSerial() : m_serial(Serial) {}
    ArdPacketSerial(HardwareSerial &serial) : m_serial(serial) {}

    int available() override
    {
        return m_serial.available();
    }
    int read() override
    {
        return m_serial.read();
    }

#if defined(__AVR__)
    size_t read(uint8_t *buffer, size_t size) override;
#else
    size_t read(uint8_t *buffer, size_t size) override
    {
        return m_serial.read(buffer, size);
    }
#endif

    int availableForWrite() override
    {
        return m_serial.availableForWrite();
    }
    size_t write(uint8_t value) override
    {
        return m_serial.write(value);
    }
    size_t write(const uint8_t *buffer, size_t size) override
    {
        return m_serial.write(buffer, size);
    }

   private:
    HardwareSerial &m_serial;
};

#ifdef __AVR__
inline size_t ArdPacketSerial::read(uint8_t *buffer, size_t size)
{
    size_t read_index = 0;
    int read_byte = 0;
    while (read_byte >= 0 && read_index < size)
    {
        read_byte = m_serial.read();
        if (read_byte >= 0)
        {
            buffer[read_index] = static_cast<uint8_t>(read_byte);
            read_index++;
        }
    }
    return read_index;
}
#endif

#endif
