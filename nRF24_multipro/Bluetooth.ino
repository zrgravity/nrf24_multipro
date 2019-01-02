

#define BT_PPM_PACKET_PERIOD        50000
#define BT_RC_PACKET_PERIOD         50000
#define BT_PPM_STARTCHAR            '$'
#define PPM_DEADZONE                25


static uint32_t BT_packet_period;

uint32_t process_BT()
{
    // Return timeout/update rate in us
    uint32_t nextPacket = micros() + BT_packet_period;

    // Send values
    switch(current_protocol) {
        case PROTO_BT_PPM:
            send_BT_PPM();
            break;
        case PROTO_BT_RC:
            send_BT_RC();
            break;
    }

    // Return timeout
    return nextPacket;
}


void BT_init()
{
    switch(current_protocol) {
        case PROTO_BT_PPM:
            BT_packet_period = BT_PPM_PACKET_PERIOD;
            break;
        case PROTO_BT_RC:
            BT_packet_period = BT_RC_PACKET_PERIOD;
            break;
    }

}


void send_BT_PPM()
{
    // Build packet
    uint8_t length = 0;
    packet[length++] = BT_PPM_STARTCHAR; //         [0] -   Start char
    packet[length++] = CHANNELS; //                 [1] -   Length
    packet[length++] = lowByte(ppm[AILERON]); //    [2] -   AILERON low
    packet[length++] = highByte(ppm[AILERON]); //   [3] -   AILERON high
    packet[length++] = lowByte(ppm[ELEVATOR]); //   [4] -   ELEVATOR low
    packet[length++] = highByte(ppm[ELEVATOR]); //  [5] -   ELEVATOR high
    packet[length++] = lowByte(ppm[THROTTLE]); //   [6] -   THROTTLE low
    packet[length++] = highByte(ppm[THROTTLE]); //  [7] -   THROTTLE high
    packet[length++] = lowByte(ppm[RUDDER]); //     [8] -   RUDDER low
    packet[length++] = highByte(ppm[RUDDER]); //    [9] -   RUDDER high
    packet[length++] = lowByte(ppm[AUX1]); //       [10] -   AUX1 low
    packet[length++] = highByte(ppm[AUX1]); //      [11] -   AUX1 high
    packet[length++] = lowByte(ppm[AUX2]); //       [12] -   AUX2 low
    packet[length++] = highByte(ppm[AUX2]); //      [13] -   AUX2 high
    packet[length++] = lowByte(ppm[AUX3]); //       [14] -   AUX3 low
    packet[length++] = highByte(ppm[AUX3]); //      [15] -   AUX3 high

    // Calculate CRC
    const uint16_t initial = 0xb5d2;
    uint16_t crc = initial;
    for (uint8_t i = 0; i < length; ++i)
        crc = crc16_update(crc, packet[i]);
    packet[length++] = lowByte(crc); //             [16] - CRC low
    packet[length++] = highByte(crc); //            [17] - CRC high

    // Send packet
    for (uint8_t i = 0; i < length; ++i)
    {
        Serial.write(packet[i]);
    }
}


void send_BT_RC()
{
    char command = 'S';
    // Convert into characters to send
    // Elevator up -> Forward
    if (ppm[ELEVATOR] > PPM_MID + PPM_DEADZONE)
        command = 'F';
    // Elevator Down -> Backward
    else if (ppm[ELEVATOR] < PPM_MID - PPM_DEADZONE)
        command = 'B';

    // Turns have priority - restart if
    // Aileron left -> left
    if (ppm[AILERON] < PPM_MID - PPM_DEADZONE)
        command = 'L';
    // Aileron right -> right
    else if (ppm[AILERON] > PPM_MID + PPM_DEADZONE)
        command = 'R';

    // Else stop (no control out of deadzone)

    // Switch commands
    // W for led change (aux3, ch7)
    static bool w_sent = false;
    if (ppm[AUX3] > PPM_MAX_COMMAND && !w_sent)
    {
        command = 'W';
        w_sent = true;
    } else if (ppm[AUX3] < PPM_MIN_COMMAND)
    {
        w_sent = false;
    }

    // U for servo (aux1, ch5)
    static bool u_sent = false;
    if (ppm[AUX1] > PPM_MAX_COMMAND && !u_sent)
    {
        command = 'U';
        u_sent = true;
    } else if (ppm[AUX1] < PPM_MIN_COMMAND && u_sent)
    {
        command = 'u';
        u_sent = false;
    }

    // Send command
    Serial.write(command);
}


//static const uint16_t polynomial = 0x1021;
//static const uint16_t initial    = 0xb5d2;
//uint16_t crc16_update(uint16_t crc, unsigned char a)
//{
//    crc ^= a << 8;
//    for (uint8_t i = 0; i < 8; ++i) {
//        if (crc & 0x8000) {
//            crc = (crc << 1) ^ polynomial;
//            } else {
//            crc = crc << 1;
//        }
//    }
//    return crc;
//}
