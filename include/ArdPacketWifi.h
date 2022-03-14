
#ifndef ARD_PACKET_Wifi_H
#define ARD_PACKET_Wifi_H

#include <Arduino.h>
#include <WiFi.h>

#include "ArdPacket.h"

class ArdPacketWifi : public ArdPacketStreamInterface
{
   public:
    explicit ArdPacketWifi(WiFiClient &wifi) : m_wifi(wifi) {}

    int available() override
    {
        return m_wifi.available();
    }
    int read() override
    {
        return m_wifi.read();
    }
    size_t read(uint8_t *buffer, size_t size) override
    {
        return m_wifi.read(buffer, size);
    }

    int availableForWrite() override;
    size_t write(uint8_t value) override
    {
        return m_wifi.write(value);
    }
    size_t write(const uint8_t *buffer, size_t size) override
    {
        return m_wifi.write(buffer, size);
    }

    void SetAvailableForWrite(int size);

   private:
    WiFiClient &m_wifi;
    int m_available_for_write = 64;
};

inline int ArdPacketWifi::availableForWrite()
{
    return m_available_for_write;
}

inline void ArdPacketWifi::SetAvailableForWrite(const int size)
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
