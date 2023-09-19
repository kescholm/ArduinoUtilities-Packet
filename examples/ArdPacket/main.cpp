
#include <Arduino.h>

#include "ArdPacketWifi.h"

uint32_t g_timing_prev = 0;
uint32_t g_timing_start = 0;
const uint32_t g_timing_step = 100000;

WiFiClient g_wifi_client;
const char *g_wifi_ssid = "SSID";
const char *g_wifi_pass = "WifiPasscode";

ArdPacketWifi g_packet_wifi = ArdPacketWifi(g_wifi_client);
ArdPacket g_packet = ArdPacket(g_packet_wifi);

const IPAddress g_host(192, 168, 2, 100);
const int g_port = 9040;
int g_timeout = 1000;

void setup()
{
    // start serial
    Serial.begin(SERIAL_BAUDRATE);
    delay(2500);

    // Start Wifi
    Serial.print("Connecting to WiFi ..");
    Serial.flush();
    WiFi.mode(WIFI_STA);
    WiFi.begin(g_wifi_ssid, g_wifi_pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print('.');
        delay(1000);
    }
    Serial.print("Connected. ");
    Serial.println(WiFi.localIP());

    // connect to server
    Serial.print("Connecting to Server ..");
    while (!g_wifi_client.connect(g_host, g_port, g_timeout))
    {
        Serial.print('.');
    }
    Serial.println("Connected");

    // create packet
    ArdPacketConfig packet_config = {};
    packet_config.crc = true;
    packet_config.delimiter = '|';
    packet_config.max_payload_size = 128;
    packet_config.message_type_bytes = 1;
    packet_config.payload_size_bytes = 2;
    g_packet.Configure(packet_config);

    // send payload
    uint8_t send_payload[30] = {'\0'};
    memcpy(send_payload, "Hello World!", sizeof("Hello World!"));
    ArdPacketPayloadInfo send_payload_info = {};
    send_payload_info.message_type = 0x01;
    send_payload_info.payload_size = sizeof("Hello World!");
    g_packet.SendPayload(send_payload_info, send_payload);

    // start timer
    g_timing_start = micros();
    g_timing_prev = g_timing_start;
}

void loop()
{
    const uint32_t now = micros();
    if ((now - g_timing_prev) > g_timing_step)
    {
        const double freq = 1.0 * (2.0 * M_PI);
        const double now_secs = static_cast<double>(now - g_timing_start) * 1e-6;

        size_t max_payload_size = 30;
        char recv_payload[30] = {'\0'};
        ArdPacketPayloadInfo recv_payload_info = {};
        eArdPacketStatus status = g_packet.ReceivePayload(max_payload_size, recv_payload_info, (uint8_t *)recv_payload);
        if (status == kArdPacketStatusDone)
        {
            // print received payload
            Serial.print("Received type {");
            Serial.print(recv_payload_info.message_type);
            Serial.print("}: ");
            recv_payload[recv_payload_info.payload_size - 1] = '\0';
            Serial.println(recv_payload);

            // send payload
            uint8_t send_payload[30] = {'\0'};
            memcpy(send_payload, "Tick", sizeof("Tick"));
            ArdPacketPayloadInfo send_payload_info = {};
            send_payload_info.message_type = 0x02;
            send_payload_info.payload_size = sizeof("Tick");
            g_packet.SendPayload(send_payload_info, send_payload);
        }

        g_timing_prev += g_timing_step;
    }
}
