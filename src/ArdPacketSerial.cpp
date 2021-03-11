#include <Arduino.h>

// Arduino Serial wrappers
// -----------------------

static size_t ard_serial_write_noblock(const uint8_t * buf, uint16_t len)
{
    const uint16_t bytes_available = Serial.availableForWrite();
    const uint16_t bytes_to_write = (len < bytes_available ? len : bytes_available);
    return Serial.write(buf, bytes_to_write);
}

static uint16_t ard_serial_read_noblock(uint8_t * buf, const uint16_t len)
{
    const uint16_t bytes_available = Serial.available();
    const uint16_t bytes_to_read = (len < bytes_available ? len : bytes_available);
    for (uint16_t i = 0; i < bytes_to_read; i++)
    {
        buf[i] = (uint8_t)Serial.read();
    }
    return bytes_to_read;
}

static int ard_serial_read_noblock_until(const uint8_t delimiter, const uint16_t max_tries)
{
    const uint16_t bytes_available = Serial.available();
    const uint16_t bytes_to_read = (max_tries < bytes_available ? max_tries : bytes_available);
    for (uint16_t i = 0; i < bytes_to_read; i++)
    {
        int c = Serial.read();
        if (c == -1)
        {
            return -1;
        }
        else if ((uint8_t)c == delimiter)
        {
            return 1;
        }
    }
    return 0;
}

void ard_serial_begin(const unsigned long baud_rate)
{
    Serial.begin(baud_rate);
    while (!Serial) continue;
}

void ard_serial_write_flush(void) { Serial.flush(); }

void ard_serial_read_flush(void)
{
    const int max_count = Serial.available();
    int count = 0;
    while (Serial.read() != -1 && count < max_count)
    {
        count++;
    }
}
