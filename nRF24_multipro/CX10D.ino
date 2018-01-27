#define CX10D_RF_BIND_CHANNEL   0x02
#define CX10D_NUM_RF_CHANNELS   16
#define CX10D_PACKET_SIZE       11
#define CX10D_PACKET_PERIOD     3000
#define CX10D_BIND_COUNT        1500

static uint8_t cx10d_phase;
static uint16_t cx10d_bind_counter;
static uint32_t cx10d_packet_counter;
static uint8_t cx10d_tx_addr[5];
static uint8_t cx10d_current_chan;
static uint8_t cx10d_txid[4];
static uint8_t cx10d_rf_chans[16];

enum {
    CX10D_BIND,
    CX10D_DATA
};

#define BTN_TAKEOFF  1
#define BTN_DESCEND  2
#define CX10D_FLAG_LAND     0x80
#define CX10D_FLAG_TAKEOFF  0x40

uint8_t cx10wd_getButtons()
{
    static uint8_t btn_state;
    static uint8_t command;
    // startup
    if (cx10d_packet_counter < 50) {
        btn_state = 0;
        command = 0;
        cx10d_packet_counter++;
    }
    // auto land
    else if (GET_FLAG_INV(AUX1, 1) && !(btn_state & BTN_DESCEND)) {
        btn_state |= BTN_DESCEND;
        btn_state &= ~BTN_TAKEOFF;
        command ^= CX10D_FLAG_LAND;
    }
    // auto take off
    else if (GET_FLAG(AUX1, 1) && !(btn_state & BTN_TAKEOFF)) {
        btn_state |= BTN_TAKEOFF;
        btn_state &= ~BTN_DESCEND;
        command ^= CX10D_FLAG_TAKEOFF;
    }
    return command;
}

void cx10d_send_packet(u8 bind)
{
    uint16_t aileron, elevator, throttle, rudder;
    if (bind) {
        packet[0] = 0xaa;
        memcpy(&packet[1], cx10d_txid, 4);
        memset(&packet[5], 0, CX10D_PACKET_SIZE - 5);
    }
    else {
        packet[0] = 0x55;

        // sticks
        aileron =  3000 - ppm[AILERON];
        elevator = 3000 - ppm[ELEVATOR];
        throttle = ppm[THROTTLE];
        rudder = ppm[RUDDER];

        packet[1] = aileron & 0xff;
        packet[2] = aileron >> 8;
        packet[3] = elevator & 0xff;
        packet[4] = elevator >> 8;
        packet[5] = throttle & 0xff;
        packet[6] = throttle >> 8;
        packet[7] = rudder & 0xff;
        packet[8] = rudder >> 8;
        // buttons
        packet[8] |= GET_FLAG(AUX2, 0x10); // flip
        packet[9] = 0x02; // rate (0-2)
        packet[10] = cx10wd_getButtons(); // auto land / take off management
    }

    // Power on, TX mode, CRC enabled
    XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));

    NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? CX10D_RF_BIND_CHANNEL : cx10d_rf_chans[cx10d_current_chan++]);
    cx10d_current_chan %= CX10D_NUM_RF_CHANNELS;

    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();

    XN297_WritePayload(packet, CX10D_PACKET_SIZE);
}

void cx10d_initialize_txid() 
{
    memcpy(cx10d_txid, transmitterID, 4);
    // not thoroughly figured out txid/channels mapping yet
    // for now 5 msb of txid[0] must be cleared
    cx10d_txid[0] &= 7;
    uint8_t offset = 6 + ((cx10d_txid[0] & 7) * 3);
    cx10d_rf_chans[0] = 0x14; // works only if txid[0] < 8
    for (uint8_t i = 1; i<16; i++) {
        cx10d_rf_chans[i] = cx10d_rf_chans[i - 1] + offset;
        if (cx10d_rf_chans[i] > 0x41)
            cx10d_rf_chans[i] -= 0x33;
        if (cx10d_rf_chans[i] < 0x14)
            cx10d_rf_chans[i] += offset;
    }
}

void cx10d_initialize_rf() 
{
    const uint8_t cx10d_bind_address[] = { 0xcc,0xcc,0xcc,0xcc,0xcc };

    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    XN297_SetTXAddr(cx10d_bind_address, 5);
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits
    NRF24L01_SetBitrate(NRF24L01_BR_1M);
    NRF24L01_SetPower(RF_POWER);
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);     // Set feature bits on
    NRF24L01_Activate(0x73);
}

void CX10D_init() 
{
    cx10d_initialize_txid();
    cx10d_initialize_rf();
    cx10d_bind_counter = CX10D_BIND_COUNT;
    cx10d_current_chan = 0;
    cx10d_phase = CX10D_BIND;
}

uint32_t process_CX10D() 
{
    uint32_t nextPacket = micros() + CX10D_PACKET_PERIOD;
    switch (cx10d_phase) {
    case CX10D_BIND:
        if (cx10d_bind_counter == 0) {
            cx10d_tx_addr[0] = 0x55;
            memcpy(&cx10d_tx_addr[1], cx10d_txid, 4);
            XN297_SetTXAddr(cx10d_tx_addr, 5);
            cx10d_phase = CX10D_DATA;
            cx10d_packet_counter = 0;
            digitalWrite(ledPin, HIGH);
        }
        else {
            cx10d_send_packet(1);
            digitalWrite(ledPin, cx10d_bind_counter-- & 0x10);
        }
        break;
    case CX10D_DATA:
        cx10d_send_packet(0);
        break;
    }
    return nextPacket;
}
