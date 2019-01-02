/*
This is a basic protocol for the transmission of PPM data. Set the 5B address to be the
same on both transmitter and receiver, and set up this protocol to be selected using nRF24_multipro.
Select the protocol on the transmitter at startup.

This protocol uses 4 different channels and skips the binding procedure.


|__ __ __ __| |__ __ __ __ __|

INCOMPLETE!!!

Transmitter address is randomly generated (transmitterID[4] from _multipro.ino)
Receiver address is fixed (0x75 0x1a 0x31 0x3d 0x14)
// If bind is implemented, it would be random and set during bind

Auto Ack is disabled
1Mbps data rate
5B address
RX pipe 0 enabled
Fixed length packet (2 * # channels)

 */

#define CHANNELS 7 // number of channels in ppm stream, 12 ideally
//enum chan_order{
//    AILERON,
//    ELEVATOR,
//    THROTTLE,
//    RUDDER,
//    AUX1,
//    AUX2,
//    AUX3
//};
#define HEADER_LENGTH   4
#define PACKET_LENGTH   HEADER_LENGTH + (CHANNELS * 2)
#define PACKET_PERIOD_US    3000

static const byte rx_address[5] = {0x75, 0x1a, 0x31, 0x3d, 0x14};
static uint8_t rf_channel = 0x15; // random channel
static uint8_t packet_count = 0;

void basic_proto_tx_init(void)
{
    // Set up NRF24L01
    // Flush TX
    NRF24L01_FlushTx();
    // Clear status bits (data ready, data sent, retransmit)
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    // Fixed packet length
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);
    // Set power
    NRF24L01_SetPower(TX_POWER_20mW);
    // Disable auto ack
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);
    // 5B Address
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0b11);
    // Setup TX address to destination
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_address, 5);
    // Setup RX pipe 0 to tx id
    uint8_t tx_address[5];
    tx_address[0] = transmitterID[3];
    tx_address[1] = transmitterID[2];
    tx_address[2] = transmitterID[1];
    tx_address[3] = transmitterID[0];
    tx_address[4] = 0xA5;
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, tx_address, 5);
    // 1 Mbps
    NRF24L01_SetBitrate(NRF24L01_BR_1M);
    // Set channel
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_channel);
    // 2B CRC, Power Up
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, _BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
}

// Process protocol
// Returns the next time in us to check
uint32_t process_basic_proto_tx(void)
{
    uint32_t nextPacket = micros() + PACKET_PERIOD_US;

    // Setup radio for transmit
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, _BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_FlushTx();

    basic_proto_tx_send_packet();
    
    packet_count++;
    if (packet_count > 127)
        digitalWrite(ledPin, HIGH);
    else
        digitalWrite(ledPin, LOW);

    return nextPacket;
}

void basic_proto_tx_send_packet(void)
{
    uint8_t packet[PACKET_LENGTH];
    uint8_t loc = 0;
    // Header (4B)
    packet[loc++] = 0x5A;
    packet[loc++] = 0x65;
    packet[loc++] = 0x6B;
    packet[loc++] = 0x65;
    // Data (14B)
    packet[loc++] = lowByte(ppm[AILERON]);
    packet[loc++] = highByte(ppm[AILERON]);
    packet[loc++] = lowByte(ppm[ELEVATOR]);
    packet[loc++] = highByte(ppm[ELEVATOR]);
    packet[loc++] = lowByte(ppm[THROTTLE]);
    packet[loc++] = highByte(ppm[THROTTLE]);
    packet[loc++] = lowByte(ppm[RUDDER]);
    packet[loc++] = highByte(ppm[RUDDER]);
    packet[loc++] = lowByte(ppm[AUX1]);
    packet[loc++] = highByte(ppm[AUX1]);
    packet[loc++] = lowByte(ppm[AUX2]);
    packet[loc++] = highByte(ppm[AUX2]);
    packet[loc++] = lowByte(ppm[AUX3]);
    packet[loc++] = highByte(ppm[AUX3]);

//    Serial.print(F("Packet: "));
//    for (uint8_t i = 0; i < PACKET_LENGTH; i++){
//        Serial.print(packet[i], HEX);
//        Serial.print(" ");
//    }
//    Serial.println();

    NRF24L01_WritePayload(packet, PACKET_LENGTH);
}
