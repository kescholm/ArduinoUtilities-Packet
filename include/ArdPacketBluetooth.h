
#ifndef ARD_PACKET_BLUETOOTH_H
#define ARD_PACKET_BLUETOOTH_H

#include <Arduino.h>
#include <BluetoothSerial.h>

#include "ArdPacket.h"

class ArdPacketBluetooth : public ArdPacketStreamInterface
{
   public:
    explicit ArdPacketBluetooth(BluetoothSerial &bt) : m_bt(bt) {}

    int available() override
    {
        return m_bt.available();
    }
    int read() override
    {
        return m_bt.read();
    }
    size_t read(uint8_t *buffer, size_t size) override;

    int availableForWrite() override;
    size_t write(uint8_t value) override
    {
        return m_bt.write(value);
    }
    size_t write(const uint8_t *buffer, size_t size) override
    {
        return m_bt.write(buffer, size);
    }

    void SetAvailableForWrite(int size);

   private:
    BluetoothSerial &m_bt;
    int m_available_for_write = 64;
};

inline size_t ArdPacketBluetooth::read(uint8_t *buffer, const size_t size)
{
    size_t read_index = 0;
    int read_byte = 0;
    while (read_byte >= 0 && read_index < size)
    {
        read_byte = m_bt.read();
        if (read_byte >= 0)
        {
            buffer[read_index] = static_cast<uint8_t>(read_byte);
            read_index++;
        }
    }
    return read_index;
}

inline int ArdPacketBluetooth::availableForWrite()
{
    return m_available_for_write;
}

inline void ArdPacketBluetooth::SetAvailableForWrite(const int size)
{
    if (size > 0)
    {
        m_available_for_write = size;
    }
    else
    {
        m_available_for_write = 0;
    }
}

#endif
