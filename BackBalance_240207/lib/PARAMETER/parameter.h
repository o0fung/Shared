#ifndef PARAMETER_H
#define PARAMETER_H

#include <Arduino.h>
#include <math.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
// - - - B L U E T O O T H   P A R A - - - - - - - - - - - - - - //                                                  //
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
#define SERVICE "0A0B0C01-4241-434B-5F42-414C414E4345"           // SERVICE UUID                                     //
#define CHAR_RX "0A0B0C02-4241-434B-5F42-414C414E4345"           // CHARACTERISTIC RX UUID                           //
#define CHAR_TX "0A0B0C03-4241-434B-5F42-414C414E4345"           // CHARACTERISTIC TX UUID                           //
                                                                 //
#define H1                   0xA0                                // Header for set zero
#define H11                  0xC0                                // Set zero command
#define H12                  0xA0                                // Reset tile value
#define H13                  0x0A                                // Reset angle threshold

#define H2                   0xA1                                // Header for set angle limit
#define H21                  0xC0                                // Front position
#define H22                  0x30                                // Back position
#define H23                  0x0C                                // Left position
#define H24                  0x03                                // Right position
#define H25                  0x00                                // All position

#define H3                   0xB0                                // Header for battery value
#define H4                   0xB1                                // Header for charing status

#define H5                   0xF0                                // Header for firmware

#define H6                   0xAA                                // Header for test internal EEPROM

#define R                    0x00                                // Read data
#define W                    0x01                                // Read data

#define EERPOM_SIZE          40

extern bool                  deviceConnected;
extern bool                  oldDeviceConnected;

extern char                  DeviceName[20];
extern char                  FirmwareVer[20];

extern uint8_t               tx_buff;
extern bool                  tx_buff_fg;
extern uint8_t               txArray[41];
extern uint8_t               txStatus[10];
extern uint8_t               txLen;
extern uint8_t               tx_count;

extern uint8_t               rxArray[10];
extern uint8_t               rxLen;
// #define SERVICE "484f4801-4241-434B-5F42-414C414E4345"           // SERVICE UUID                                     //
// #define CHAR_RX "484f4802-4241-434B-5F42-414C414E4345"           // CHARACTERISTIC RX UUID                           //
// #define CHAR_TX "484f4803-4241-434B-5F42-414C414E4345"           // CHARACTERISTIC TX UUID                           //

//                                                               //                                                  //
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
// - - - C O M M O N   P A R A - - - - - - - - - - - - - - - - - //                                                  //
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
#define COM                  115200                              // COM PORT BUAD RATE                               //
#define SR_001               100000                              // 1sec Counter (1,000,000 us; 1Hz)                 //
#define SR_200               5000                                // 5 ms Counter (5,000 us; 200Hz)                   //
#define L                    LOW                                 // Motor off                                        //
#define H                    HIGH                                // Motor on                                         //

extern unsigned long         timer;                              // loop time duration                               //
extern double                delta_t;                            //                                                  //
extern uint8_t               btn_count;                          // Button pressed counter                           //

extern String                check_balance_str[2];
extern uint8_t               check_bal[2];
extern uint8_t               defualt_angle;
extern uint8_t               angleLmt[4];

extern uint16_t              bt_adc_raw;                         // Battery ADC value from GPIO
extern double                bt_read_value;                      // Battery voltage
extern double                bt_cap;                             // Battery capacitance in %
extern uint8_t               bt_charge_det;                      // Battery charge detect
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
// - - - M P U   P A R A - - - - - - - - - - - - - - - - - - - - //                                                  //
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
#define SAMPLRT_DIV          4                                   // MPU6050 setting
#define DLPF_CONF            6                                   // MPU6050 setting
#define CUTOFF_FREQ          3.0                                 // for 3Hz lower-pass filter
#define SAMPLE_TIME          14850                               // for 50Hz sampling rate
                                                                 //
#define A_FACTOR             2 * 9.81 / 32767                    // Convert accel signal from int16 to +/-2g
#define G_FACTOR             250.0 / 32767                       // Convert gyro signal from int16 to +/-250 deg/s
#define R_FACTOR             57.295779                           // convert radian to degree (180/pi)
#define COE                  1000000.0                           //
                                                                 //
#define N_CHANNEL            6                                   // Number of data channel
#define N_BUFFER             3                                   // Number of data saved in buffer for 2nd order IIRF//
                                                                 // 
#define FILTER_WEIGH         0.9                                 // Complentary Filter, weight on Gyroscope data

// #define MOTOR_L              19                                  // Left Motor
// #define MOTOR_R              18                                  // Right Motor
// #define SET_ZERO_BTN         5

extern double   raw[N_CHANNEL][N_BUFFER];
extern double   filt[N_CHANNEL][N_BUFFER];
extern double   tilt_x[N_BUFFER];                                  //
extern double   tilt_z[N_BUFFER];                                  //
extern double   tile_x_cal;
extern double   tile_z_cal;
extern double   tile_x_final;
extern double   tile_z_final;
extern double   b0, b1, b2, a1, a2;                                // Butterworth filter coefficients

// extern double a_z, a_x;
// extern double a_zy, a_xy;

extern double   AngleRoll, AnglePitch;

extern float    Pitch_lmt, Pitch_cal;

extern uint8_t  tiltX2Byte[4];
extern uint8_t  tiltZ2Byte[4];

#endif