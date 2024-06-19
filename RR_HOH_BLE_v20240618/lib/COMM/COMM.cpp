#include <Arduino.h>
#include "COMM.h"

COMM::COMM(Motor *ptr_motor, System *ptr_sys, IMU *ptr_imu) {
    _ptr_motor = ptr_motor;
    _ptr_sys = ptr_sys;
    _ptr_imu = ptr_imu;
}

void COMM::decode(const uint8_t *buffer, uint16_t length) {
    // check if the HEADER byte is valid
    // check if the LENGTH byte is valid, 
    //   i.e. match the length of data packet -1 (i.e. exclude HEADER)
    uint8_t packet_length = length - 1;
    if ((buffer[0] == DATA_HEADER) && (buffer[1] == packet_length)) {
        
        // compute checksum value
        uint16_t checksum = 0;
        for (uint8_t i = 0; i < packet_length - 1; i++) {  // minus 1 to exclude HEADER & CHECKSUM
            checksum += buffer[i + 1];  // plus 1 to exclude HEADER
        }
        
        // check if the CHECKSUM byte is valid,
        //    i.e. match the sum of data packet (exclude HEADER) with bit inversion
        if (buffer[packet_length] == (uint8_t) (~checksum)) {
            
            // display successful data packet content
            Serial.printf(">> Received %c: 0x FF ", (char) buffer[2]);
            for (uint8_t i = 0; i < packet_length - 1; i++) {  // minus 1 to exclude HEADER & CHECKSUM
                Serial.printf("%x ", buffer[i + 1]);
            }
            Serial.printf("%x\n", buffer[packet_length]);

            // Third byte is the command byte:
            switch (buffer[2]) {

                case 'e':     // reset to extension (101)
                    // 0x FF 03 65 97
                    _ptr_motor->reset_skips();
                    _ptr_motor->reset_to_extension();
                    break;

                case 'f':     // reset to flexioin (102)
                    // 0x FF 03 66 96
                    _ptr_motor->reset_skips();
                    _ptr_motor->reset_to_flexion();
                    break;
                
                case 'c':     // calibration (99)
                    // 0x FF 03 63 99
                    _ptr_motor->reset_skips();
                    _ptr_motor->calibration();
                    break;
                
                case 'p':     // cpm (normal version) (112)
                    // 0x FF 04 70 00 8B
                    for (uint8_t i = 0; i < N_FINGERS; i++) {
                        _ptr_motor->set_skip(i, bitRead(buffer[3], i));
                    }
                    _ptr_motor->cpm_mass();
                    break;

                case 's':     // cpm (sequential version) (115)
                    // 0x FF 04 73 00 88
                    for (uint8_t i = 0; i < N_FINGERS; i++) {
                        _ptr_motor->set_skip(i, bitRead(buffer[3], i));
                    }
                    _ptr_motor->cpm_mass(true, false);
                    break;

                case 'r':     // cpm (sequential version with reverse sequence) (114)
                    // 0x FF 04 72 03 86
                    for (uint8_t i = 0; i < N_FINGERS; i++) {
                        _ptr_motor->set_skip(i, bitRead(buffer[3], i));
                    }
                    _ptr_motor->cpm_mass(true, true);
                    break;

                case 'Z':     // cpm (sleep or pause) (90)
                    // 0x FF 03 5A A2
                    if ((_ptr_motor->get_state() >= _ptr_motor->CPM) && (_ptr_motor->get_state_cpm() > _ptr_motor->CPM_PAUSE)) {
                        _ptr_motor->mass_stop();
                        _ptr_motor->set_state_cpm_prev(_ptr_motor->get_state_cpm());
                        _ptr_motor->set_state_cpm(_ptr_motor->CPM_PAUSE);
                    }
                    break;
                
                case 'z':     // cpm (wake or resume) (122)
                    // 0x FF 03 7A 82
                    if ((_ptr_motor->get_state() >= _ptr_motor-> CPM) && (_ptr_motor->get_state_cpm() == _ptr_motor->CPM_PAUSE)) {
                        _ptr_motor->set_state_cpm(_ptr_motor->get_state_cpm_prev());
                        _ptr_motor->set_state_cpm_prev(_ptr_motor->CPM_END);
                    }
                    break;
                
                case 'x':     // stop motor (120)
                    // 0x FF 03 78 84
                    _ptr_motor->mass_stop();
                    _ptr_motor->reset_states();
                    _ptr_motor->reset_skips();
                    break;
                
                case 'm':     // move motor (109)
                    // 0x FF 08 6D B2 B2 B2 B2 B2 10 (e.g. 50% with ROM limit)
                    uint8_t tpos[N_FINGERS];
                    bool tlim[N_FINGERS];
                    for (uint8_t i = 0; i < N_FINGERS; i++) {
                        tpos[i] = buffer[i + 3] & 0x7F;
                        tlim[i] = (bool) (buffer[i + 3] >> 7);
                    }
                    _ptr_motor->mass_move(tpos, tlim);
                    break;

                case 'M':     // activate offset of accelerometer (77)
                    // 0x FF 03 4D AF
                    _ptr_imu->set_has_offset_acc(false);
                    _ptr_imu->set_zero_pos();
                    // _ptr_motor->cpm_mass(1);
                    break;

                case 'o':     // move motor in cpm only one cycle (111)
                    // 0x FF 03 6F 8D
                    // _ptr_imu->set_has_offset_acc(false);
                    // _ptr_imu->set_zero_pos();
                    _ptr_motor->cpm_mass(1);
                    break;
                
                case 'l':     // set limit for extension (e.g. 20) (108)
                    // 0x FF 08 6C 14 14 14 14 14 27
                    for (uint8_t i = 0; i < N_FINGERS; i++) {
                        _ptr_motor->set_limit_extension(buffer[i + 3], i);
                    }
                    break;
                
                case 'L':     // set limit for flexion (e.g. 80) (76)
                    // 0x FF 08 4C 50 50 50 50 50 1B
                    for (uint8_t i = 0; i < N_FINGERS; i++) {
                        _ptr_motor->set_limit_flexion(buffer[i + 3], i);
                    }
                    break;

                case 't':  {   // set cpm wait time for extension (e.g. 1000000) (116)
                        // 0x FF 07 74 00 0F 42 40 F3
                        binaryfloat wait_time;
                        for (uint8_t i = 0; i < SIZE_FLOATPT; i++) {
                            wait_time.binary[i] = buffer[i + 3];
                        }
                        _ptr_motor->set_dt_wait_ext(wait_time.integer);
                        break;
                    }

                case 'T':  {   // set cpm wait time for flexion (e.g. 1000000) (84)
                        // 0x FF 07 54 00 0F 42 40 13
                        binaryfloat wait_time;
                        for (uint8_t i = 0; i < SIZE_FLOATPT; i++) {
                            wait_time.binary[i] = buffer[i + 3];
                        }
                        _ptr_motor->set_dt_wait_flex(wait_time.integer);
                        break;
                    }

                case 'v':  {  // set to send version message (105)
                    // 0x FF 03 76 86
                    _ptr_sys->set_enable_version(true);
                    break;
                }

                case 'V':  {  // set to send development version message (105)
                    // 0x FF 03 56 A6
                    _ptr_sys->set_enable_development(true);
                    break;
                }

                case 'D':     // run data stream  (68)
                    // 0x FF 03 44 B8
                    _ptr_sys->set_enable_data(true);
                    break;

                case 'd':     // stop data stream  (100)
                    // 0x FF 03 64 98
                    _ptr_sys->set_enable_data(false);
                    break;

                case 'G':     // run get status stream  (71)
                    // 0x FF 03 47 B5
                    _ptr_sys->set_enable_status(true);
                    break;

                case 'g':     // stop get status stream  (103)
                    // 0x FF 03 71 95
                    _ptr_sys->set_enable_status(false);
                    break;
                
                case 'i':     // get info / setting message  (105)
                    // 0x FF 03 69 93
                    _ptr_sys->set_enable_setting(true);
                    break;
                
                case 'I':     // get info / debug message  (73)
                    // 0x FF 03 49 B3
                    Serial.println(">> ===================");
                    Serial.print(">> Version: ");
                    Serial.println(_ptr_sys->get_version());
                    Serial.print(">> Development: ");
                    Serial.println(_ptr_sys->get_development());
                    Serial.print(">> Connected Cable: ");
                    Serial.println((_ptr_sys->status & 4) ? "Yes" : "No");
                    Serial.print(">> Connected Brace: ");
                    Serial.println((_ptr_sys->status & 2) ? "Yes" : "No");
                    Serial.print(">> Side: ");
                    if (_ptr_sys->status & 2) {
                        Serial.println((_ptr_sys->status & 1) ? "Right" : "Left");
                    }
                    else {
                        Serial.println("Not Detected");
                    }
                    for (uint8_t i = 0; i < N_FINGERS; i++) {
                        Serial.printf(">> #%d Pos = ", i);
                        Serial.print(_ptr_motor->pos[i]);
                        Serial.printf("; Ext = ", i);
                        Serial.print(_ptr_motor->get_limit_extension(i));
                        Serial.printf("; Flex = ", i);
                        Serial.println(_ptr_motor->get_limit_flexion(i));
                    }
                    Serial.print(">> Time Taken to Move Full ROM: ");
                    Serial.println(_ptr_motor->get_dt_full_rom());
                    Serial.print(">> CPM Wait Time at Extension: ");
                    Serial.println(_ptr_motor->get_dt_wait_ext());
                    Serial.print(">> CPM Wait Time at Flexion: ");
                    Serial.println(_ptr_motor->get_dt_wait_flex());
                    Serial.print(">> Count: ");
                    Serial.print(_ptr_sys->get_count());
                    Serial.print("; dt: ");
                    Serial.println(_ptr_sys->get_dt());
                    Serial.print(">> ACC: ");
                    Serial.print(_ptr_imu->acc.x.array[0]); Serial.print('\t');
                    Serial.print(_ptr_imu->acc.y.array[0]); Serial.print('\t');
                    Serial.print(_ptr_imu->acc.z.array[0]); Serial.println('\t');
                    Serial.print(">> GYR: ");
                    Serial.print(_ptr_imu->gyr.x.array[0]); Serial.print('\t');
                    Serial.print(_ptr_imu->gyr.y.array[0]); Serial.print('\t');
                    Serial.print(_ptr_imu->gyr.z.array[0]); Serial.println('\t');
                    Serial.println(">> ===================");

                    break;
                
                default:
                    break;
            }
        }
    }
}

void COMM::encode_status(uint8_t *buffer, uint8_t *size) {
    // BYTE #1 :
    //      bit 0-4 :   finger is moving?   [boolean x 5]
    //      bit 5-7 :   motor status        [uint8_t x 1] and used 3 bits only
    //                  0 = IDLE
    //                  1 = RESET_FLEX
    //                  2 = RESET_EXT
    //                  3 = CALIBRATE
    //                  4 = FREE
    //                  5 = CPM
    //                  6 = CPM_SEQ
    //                  7 = CPM_SEQ_REV
    _byte_status_1 = _ptr_motor->state << N_FINGERS;
    for (uint8_t i = 0; i < N_FINGERS; i++) {
        if (_ptr_motor->is_moving[i]) {
            bitSet(_byte_status_1, i);
        }
    }
    // BYTE #2 :
    //      bit 0-4 :   finger is flexing?  [boolean x 5]
    //      bit 5-7 :   system status       [uint8_t x 1] and used 3 bits only
    //                  0 = LEFT side (0) or RIGHT side (1)
    //                  1 = Brace Connected (1) or Disconnected (0)
    //                  2 = Cable Connected (1) or Disconnected (0)
    _byte_status_2 = _ptr_sys->status << N_FINGERS;
    for (uint8_t i = 0; i < N_FINGERS; i++) {
        if (_ptr_motor->is_flexing[i]) {
            bitSet(_byte_status_2, i);
        }
    }
    // BYTE #2 :
    //      bit 5-7 :   system status       [uint8_t x 1] and used 3 bits only
    //                  0 = LEFT side (0) or RIGHT side (1)
    //                  1 = Brace Connected (1) or Disconnected (0)
    //                  2 = Cable Connected (1) or Disconnected (0)
    _byte_status_3 = _ptr_motor->state_cpm << 4;
    // BYTE #3-#7 :
    //      bit 0-6 :   finger position     [uint8_t x 1] and used 7 bits only
    //                      in range of 0 % to 100 %
    //      bit 7   :   finger motor has current flowing    [boolean x 1]
    //                      HAS current (1) or NO current (0)
    for (uint8_t i = 0; i < N_FINGERS; i++) {
        _byte_motor[i] = (uint8_t) (_ptr_motor->pos[i]) | _ptr_motor->get_has_current(i) << 7;
    }

    // construct data packet
    buffer[0] = DATA_HEADER;                // HEADER 0xFF
    buffer[1] = STATUS_DATA_LENGTH;                // LENGHT of data packet (excluding header and checksum)
    buffer[2] = (uint8_t) 'm';
    buffer[3] = _byte_status_1;             // BYTE #1: finger moving & motor state
    buffer[4] = _byte_status_2;             // BYTE #2: finger flexing & system status
    buffer[5] = _byte_status_3;             // BYTE #3: cpm state
    buffer[6] = _byte_motor[0];             // BYTE #3: finger 0 position & current flows
    buffer[7] = _byte_motor[1];             // BYTE #4: finger 1 position & current flows
    buffer[8] = _byte_motor[2];             // BYTE #5: finger 2 position & current flows
    buffer[9] = _byte_motor[3];             // BYTE #6: finger 3 position & current flows
    buffer[10] = _byte_motor[4];             // BYTE #7: finger 4 position & current flows
    
    // compute checksum
    uint16_t checksum = 0;
    for (uint8_t i = 0; i < STATUS_DATA_LENGTH - 1; i++) {
        checksum += buffer[i + 1];
    }
    buffer[STATUS_DATA_LENGTH] = ~ ((uint8_t) checksum);     // CHECKSUM (invert of sum of packet content)

    *size = STATUS_DATA_LENGTH + 1;
}

void COMM::encode_data(uint8_t *buffer, uint8_t *size) {
    // construct data packet
    buffer[0] = DATA_HEADER;                // HEADER 0xFF
    buffer[1] = IMU_DATA_LENGTH;            // LENGHT of data packet (excluding header and checksum)
    buffer[2] = (uint8_t) 'd';
    
    binaryfloat data_count;
    data_count.integer = _ptr_sys->get_count();
    for (uint8_t i = 0; i < SIZE_FLOATPT; i++) {
        buffer[i + 3] = data_count.binary[i];
    }

    binaryfloat data_dt;
    data_dt.integer = _ptr_sys->get_dt();
    for (uint8_t i = 0; i < SIZE_FLOATPT; i++) {
        buffer[i + 7] = data_dt.binary[i];
    }

    binaryfloat data_tilt_x;
    data_tilt_x.floatpt = _ptr_imu->ang.x.array[0];
    for (uint8_t i = 0; i < SIZE_FLOATPT; i++) {
        buffer[i + 11] = data_tilt_x.binary[i];
    }

    binaryfloat data_tilt_y;
    data_tilt_y.floatpt = _ptr_imu->ang.y.array[0];
    for (uint8_t i = 0; i < SIZE_FLOATPT; i++) {
        buffer[i + 15] = data_tilt_y.binary[i];
    }

    binaryfloat data_tilt_z;
    data_tilt_z.floatpt = _ptr_imu->ang.z.array[0];
    for (uint8_t i = 0; i < SIZE_FLOATPT; i++) {
        buffer[i + 19] = data_tilt_z.binary[i];
    }

    // compute checksum
    uint16_t checksum = 0;
    for (uint8_t i = 0; i < IMU_DATA_LENGTH - 1; i++) {
        checksum += buffer[i + 1];
    }
    buffer[IMU_DATA_LENGTH] = ~ ((uint8_t) checksum);     // CHECKSUM (invert of sum of packet content)

    *size = IMU_DATA_LENGTH + 1;

    if (_ptr_imu->get_working()) {
        
        Serial.print(_ptr_imu->ang.x.array[0]); Serial.print('\t');
        Serial.print(_ptr_imu->ang.y.array[0]); Serial.print('\t');
        Serial.print(_ptr_imu->ang.z.array[0]); Serial.print('\t');

        Serial.print(_ptr_imu->accel.x.filt[0]); Serial.print('\t');
        Serial.print(_ptr_imu->accel.y.filt[0]); Serial.print('\t');
        Serial.print(_ptr_imu->accel.z.filt[0]); Serial.print('\t');

        Serial.print(_ptr_sys->get_count()); Serial.print('\t');
        Serial.print(_ptr_sys->get_dt()); Serial.print('\t');
        
        Serial.println("");
    }
}

void COMM::encode_development(uint8_t *buffer, uint8_t *size) {
    // get the version info
    char *ver = _ptr_sys->get_development();
    uint8_t len = strlen(ver);

    // construct the data packet for version
    buffer[0] = DATA_HEADER;                // HEADER 0xFF
    buffer[1] = len + 3;                    // LENGHT of version packet (excluding header and checksum)
    buffer[2] = (uint8_t) 'V';
    for (uint8_t i = 0; i < len; i++) {
        buffer[i + 3] = ver[i];
    }

    // compute checksum
    uint16_t checksum = 0;
    for (uint8_t i = 0; i < len + 2; i++) {
        checksum += buffer[i + 1];
    }
    buffer[len + 3] = ~ ((uint8_t) checksum);     // CHECKSUM (invert of sum of packet content)

    *size = len + 4;
}

void COMM::encode_version(uint8_t *buffer, uint8_t *size) {
    // get the version info
    char *ver = _ptr_sys->get_version();
    uint8_t len = strlen(ver);

    // construct the data packet for version
    buffer[0] = DATA_HEADER;                // HEADER 0xFF
    buffer[1] = len + 3;                    // LENGHT of version packet (excluding header and checksum)
    buffer[2] = (uint8_t) 'v';
    for (uint8_t i = 0; i < len; i++) {
        buffer[i + 3] = ver[i];
    }

    // compute checksum
    uint16_t checksum = 0;
    for (uint8_t i = 0; i < len + 2; i++) {
        checksum += buffer[i + 1];
    }
    buffer[len + 3] = ~ ((uint8_t) checksum);     // CHECKSUM (invert of sum of packet content)

    *size = len + 4;
}

void COMM::encode_setting(uint8_t *buffer, uint8_t *size) {
    // construct the data packet for version
    buffer[0] = DATA_HEADER;            // HEADER 0xFF
    buffer[1] = SETTING_LENGTH;         // LENGTH of version packet (excluding header and checksum)
    buffer[2] = (uint8_t) 's';

    for (uint8_t i = 0; i < N_FINGERS; i++) {
        buffer[i + 3] = _ptr_motor->get_limit_extension(i);
    }

    for (uint8_t i = 0; i < N_FINGERS; i++) {
        buffer[i + 8] = _ptr_motor->get_limit_flexion(i);
    }

    binaryfloat dt_full_rom;
    dt_full_rom.integer = _ptr_motor->get_dt_full_rom();
    for (uint8_t i = 0; i < SIZE_FLOATPT; i++) {
        buffer[i + 13] = dt_full_rom.binary[i];
    }

    binaryfloat dt_wait_ext;
    dt_wait_ext.integer = _ptr_motor->get_dt_wait_ext();
    for (uint8_t i = 0; i < SIZE_FLOATPT; i++) {
        buffer[i + 17] = dt_wait_ext.binary[i];
    }

    binaryfloat dt_wait_flex;
    dt_wait_flex.integer = _ptr_motor->get_dt_wait_flex();
    for (uint8_t i = 0; i < SIZE_FLOATPT; i++) {
        buffer[i + 21] = dt_wait_flex.binary[i];
    }

    // compute checksum
    uint16_t checksum = 0;
    for (uint8_t i = 0; i < SETTING_LENGTH - 1; i++) {
        checksum += buffer[i + 1];
    }
    buffer[SETTING_LENGTH] = ~ ((uint8_t) checksum);     // CHECKSUM (invert of sum of packet content)

    *size = SETTING_LENGTH + 1;
}
