#ifndef _CONTROL_
#define _CONTROL_

#include <Arduino.h>
#include "pin_use.h"
#include "parameter.h"

class Control{
    private:

    public:
        void save2_Tx_Buffer();
        void clear_Tx_Para_Buff();
        void clear_Rx_Buffer();
        void console_log();

        void check_balance();
        void check_button();
        void motor_control();

        void read_bt_volt();
        void read_bt_charge();
};









#endif