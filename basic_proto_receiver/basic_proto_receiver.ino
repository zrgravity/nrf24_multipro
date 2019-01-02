/*
This is a basic protocol for the transmission of PPM data. Set the 5B address to be the
same on both transmitter and receiver, and set up this protocol to be selected using nRF24_multipro.
Select the protocol on the transmitter at startup.

//This protocol uses 4 different channels and skips the binding procedure.
This protocol starts with only using 1 RF channel.


|__ __ __ __| |__ __ __ __ __|

INCOMPLETE!!!

Transmitter address is randomly generated (transmitterID[4] from _multipro.ino)
Receiver address is fixed (0x75 0x1a 0x31 0x3d 0x14)
// If bind is implemented, it would be random and set during bind

Auto Ack is used on receiver only
1Mbps data rate
5B address
RX pipe 0 enabled
Fixed length packet (2 * # channels)
2B CRC

 */

#include <SPI.h>
#include "RF24.h"

#define ledPin  13 // LED  - D13
#define MOSI    11
#define MISO    12
#define SCK     13
#define CE_pin  9
#define CS_pin  10 // OR CSN OR SS
// #define INT // Not used


#define CHANNELS 7 // number of channels in ppm stream, 12 ideally
enum chan_order{
    AILERON,
    ELEVATOR,
    THROTTLE,
    RUDDER,
    AUX1,  // (CH5)  led light, or 3 pos. rate on CX-10, H7, or inverted flight on H101
    AUX2,  // (CH6)  VR Knob
    AUX3
};
#define HEADER_LENGTH   4
#define PACKET_LENGTH   HEADER_LENGTH + (CHANNELS * 2)
#define PACKET_PERIOD_US    3000

// SPI bus plus CE and CS pins
RF24 radio(CE_pin, CS_pin);

static const byte rx_address[5] = {0x75, 0x1a, 0x31, 0x3d, 0x14};
static uint8_t rf_channel = 0x15; // random channel

#define PPM_MIN 1000
static uint16_t ppm[12] = {PPM_MIN,PPM_MIN,PPM_MIN,PPM_MIN,PPM_MIN,PPM_MIN,
                           PPM_MIN,PPM_MIN,PPM_MIN,PPM_MIN,PPM_MIN,PPM_MIN,};

// Setup radio
void basic_proto_rx_init(void)
{
    // Set up NRF24L01
    radio.begin();
    // Flush TX
    radio.flush_tx();
    // Clear status bits (data ready, data sent, retransmit)
    // Fixed packet length
    radio.setPayloadSize(PACKET_LENGTH);
    // 2B CRC
    radio.setCRCLength(RF24_CRC_16);
    // Set power
    radio.setPALevel(RF24_PA_LOW);
    // Set up Auto ack on pipe 0
    radio.setAutoAck(0, false);
    // 5B Address
    radio.setAddressWidth(5);
    // Setup TX address to same as RX pipe 0
        // See Enhanced Shock Burst info on pg. 65 of data sheet
    radio.openWritingPipe(rx_address);
    // Setup RX pipe 0
    radio.openReadingPipe(0, rx_address);
    // 1 Mbps
    radio.setDataRate(RF24_1MBPS);
    // Set channel
    radio.setChannel(rf_channel);

    radio.startListening();
}

void basic_proto_rx_process(void)
{
    if (radio.available())
    {
        uint8_t packet[PACKET_LENGTH];
        while (radio.available())
        {
            radio.read(packet, sizeof(packet));
        }
        uint8_t *payload = packet + HEADER_LENGTH;
        ppm[AILERON] =  payload[0] +  ((uint16_t)payload[1] << 8);
        ppm[ELEVATOR] = payload[2] +  ((uint16_t)payload[3] << 8);
        ppm[THROTTLE] = payload[4] +  ((uint16_t)payload[5] << 8);
        ppm[RUDDER] =   payload[6] +  ((uint16_t)payload[7] << 8);
        ppm[AUX1] =     payload[8] +  ((uint16_t)payload[9] << 8);
        ppm[AUX2] =     payload[10] + ((uint16_t)payload[11] << 8);
        ppm[AUX3] =     payload[12] + ((uint16_t)payload[13] << 8);
    }
}



void setup()
{
  Serial.begin(115200);
  Serial.println(F("Test reception of packet"));

  basic_proto_rx_init();
}

uint32_t ms_since_update = 0;

void loop()
{
    basic_proto_rx_process();

    if (millis() - ms_since_update > 1000)
    {
        #ifdef PRINT_PPM
//        Serial.println(F("PPM Values:"));
        Serial.println(F("AILR\tELEV\tTHRO\tRDDR\tAUX1\tAUX2\tAUX3"));
        Serial.print(ppm[AILERON]);
        Serial.print(F("\t"));
        Serial.print(ppm[ELEVATOR]);
        Serial.print(F("\t"));
        Serial.print(ppm[THROTTLE]);
        Serial.print(F("\t"));
        Serial.print(ppm[RUDDER]);
        Serial.print(F("\t"));
        Serial.print(ppm[AUX1]);
        Serial.print(F("\t"));
        Serial.print(ppm[AUX2]);
        Serial.print(F("\t"));
        Serial.print(ppm[AUX3]);
        Serial.println(F("\t"));
        #endif

        ms_since_update = millis();
    }
}

