#include "parameter.h"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
// - - - B L U E T O O T H   P A R A - - - - - - - - - - - - - - //                                                  //
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
extern bool     deviceConnected     = false;                     //
extern bool     oldDeviceConnected  = false;                     //
                                                                 //
extern char     DeviceName[20]      = "BACK_BALANCE3";           //
extern char     FirmwareVer[20]     = "V1.0.0.0";                //
                                                                 //
extern uint8_t  tx_buff             = 0;                         //
extern bool     tx_buff_fg          = false;
extern uint8_t  txArray[41]         = {0xA0,
                                       0,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,0,0};
extern uint8_t txStatus[10]         = {0,0,0,                    // HDR, CMD, R/W
                                       0,0,0,0,0,0,0};           // VALUE
extern uint8_t  txLen               = 0;
extern uint8_t  tx_count            = 0;

extern uint8_t  rxArray[10]         = {0,0,0,0,0,0,0,0,0,0};
extern uint8_t  rxLen               = 0;
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
// - - - C O M M O N   P A R A - - - - - - - - - - - - - - - - - //                                                  //
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
extern unsigned long    timer       = 0;                         // loop time duration                               //
extern double           delta_t     = 0;                         //
extern uint8_t          btn_count   = 0;                         // Button pressed counter

extern String   check_balance_str[2]    = {"", ""};
extern uint8_t  check_bal[2]            = {0,0};
extern uint8_t  defualt_angle           = 15;
extern uint8_t  angleLmt[4]             = {15, 15, 15, 15};      // Angle limit for forward, backward, left and right

extern uint16_t bt_adc_raw              = 0;                     // Battery voltage reading from GPIO
extern double   bt_read_value           = 0.0;                   // Battery voltage
extern double   bt_cap                  = 0.0;                   // Battery capacitance in %
extern uint8_t  bt_charge_det           = 0;                     // Battery charge detect
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
// - - - M P U   P A R A - - - - - - - - - - - - - - - - - - - - //                                                  //
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
extern double raw[N_CHANNEL][N_BUFFER]  = {{0,0,0},{0,0,0},
                                           {0,0,0},{0,0,0},
                                           {0,0,0},{0,0,0}};
extern double filt[N_CHANNEL][N_BUFFER] = {{0,0,0},{0,0,0},
                                           {0,0,0},{0,0,0},
                                           {0,0,0},{0,0,0}};
extern double tilt_x[N_BUFFER]          = {0,0,0};
extern double tilt_z[N_BUFFER]          = {0,0,0};
extern double tile_x_cal                = 0;
extern double tile_z_cal                = 0;
extern double tile_x_final              = 0;
extern double tile_z_final              = 0;

// extern double a_z = 0, a_x = 0;
// extern double a_zy = 0, a_xy =0 ;

extern double b0 = 0;                                            // Butterworth filter coefficients
extern double b1 = 0;
extern double b2 = 0;
extern double a1 = 0;
extern double a2 = 0;                                   

extern double   AngleRoll               = 0; 
extern double   AnglePitch              = 0;

extern float    Pitch_lmt               = 15.0;
extern float    Pitch_cal               = 90.0;

extern uint8_t  tiltX2Byte[4] = {0,0,0,0};
extern uint8_t  tiltZ2Byte[4] = {0,0,0,0};