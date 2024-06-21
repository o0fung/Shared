#ifndef _PIN_USED_H_
#define _PIN_USED_H_

#include <Arduino.h>
#include "./parameter.h"

#define MOTOR_L              19                                  // Left Motor
#define MOTOR_R              18                                  // Right Motor
#define SET_ZERO_BTN         5
#define CHARGE_DETECT        17
#define BT_VOLT_READ         34

//              ----------------------------------------------
//              | GND                                    GND |
//              | 3V3                                   IO23 | 
//         PROG | EN           ESP32-WROOM32-D          IO22 | I2C_SCL
//              | S.VP                                   TXD | TXD/ PROG
//              | S.NP                                   RXD | RXD/ PROG
// BT_VOLT_READ | IO34                                  IO21 | I2C_SDA
//              | IO35                                    NC |
//              | IO32                                  IO19 | Motor_L
//              | IO33                                  IO18 | Motor_R
//              | IO25                                   IO5 | Set_Zero_Btn
//              | IO26                                  IO17 | Charge_Detect
//              | IO27      I                    I      IO16 | 
//              | IO14   G  O                    O  I    IO4 | 
//              | IO12   N  1                    1  O    IO0 | PROG   
//              |        D  3  X  X  X  X  X  X  5  2        |
//              ----------------------------------------------

// ----- CLASSES ------------------------------------------------------------- //
class pin_used{
    /// PUBLIC //////////////////////////////////////////////////////////////////
    public:
        void pinsetup();                                                       // Pin setup function
        
    /// PRIVATE /////////////////////////////////////////////////////////////////
    private:
};

#endif    