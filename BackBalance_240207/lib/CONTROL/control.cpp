#include "control.h"

void Control::save2_Tx_Buffer(){
    if(tx_buff<40){
        for(int i=0; i<4; i++){
            txArray[i + 1 + tx_buff    ] = tiltX2Byte[i];
            txArray[i + 1 + tx_buff + 4] = tiltZ2Byte[i];
        }
        tx_buff += 8;
    }else{
        tx_buff_fg = true;
        tx_buff=0;
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
void Control::clear_Tx_Para_Buff(){
    for(int i=0; i<10; i++){

    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
void Control::clear_Rx_Buffer(){
    for(int i=0; i<10; i++){
        rxArray[i] = 0;
        rxLen = 0;
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
void Control::console_log(){
    Serial.printf("ADC: %d\tvolt: %.4f\n", bt_adc_raw, bt_read_value);


    // Serial.printf("[%.2f %.2f]\t[%s %s]  [%d %d %d]\t[%d]\t[%.2f (%.2f)]\n", 
    //                 tile_x_final ,tile_z_final,
    //                 check_balance_str[0], check_balance_str[1],
    //                 angleLmt[0],angleLmt[2],angleLmt[3],
    //                 bt_charge_det,
    //                 bt_read_value, bt_cap
    //             );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
void Control::check_balance(){                                   //
    // Check front and back                                      //
    if(tile_x_final >= -angleLmt[1] &&                           // eg. -15 >= A <= 15
    tile_x_final <= angleLmt[0]){                             //
        check_bal[0] = 0x00;                                     //
        check_balance_str[0] = "------";                         //
    }else if(tile_x_final < -angleLmt[1]){
        check_bal[0] = 0x03;
        check_balance_str[0] = "B Over";                         //
    }else{
        check_bal[0] = 0x0C;
        check_balance_str[0] = "F Over";                         //
    }

    // Check front and back                                      //
    if(tile_z_final >= -angleLmt[3] &&                           // eg. -15 >= A <= 15
    tile_z_final <= angleLmt[2]){                             //
        check_bal[1] = 0x00;                                     //
        check_balance_str[1] = "------";                         //
    }else if(tile_z_final < -angleLmt[3]){
        check_bal[1] = 0x03;
        check_balance_str[1] = "R Over";                         //
    }else{
        check_bal[1] = 0x0C;
        check_balance_str[1] = "L Over";                         //
    }
    
    motor_control();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
void Control::check_button(){
    if(!digitalRead(SET_ZERO_BTN)){
        if(btn_count>255){                                       // prevent button hold turn counter overflow
            btn_count = 255;
        }else{
            btn_count++;
        }
    }else{
        btn_count = 0;
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
void Control::motor_control(){
    if(check_bal[0] == 0x0C){
        digitalWrite(MOTOR_L, H);   
        digitalWrite(MOTOR_R, H);   
    }else if(check_bal[1] == 0x03){
        digitalWrite(MOTOR_R, H);
        if(check_bal[0] == 0x0C){
            digitalWrite(MOTOR_L, H);
        }else{
            digitalWrite(MOTOR_L, L);
        }
    }else if(check_bal[1] == 0x0C){
        digitalWrite(MOTOR_L, H);
        if(check_bal[0] == 0x0C){
            digitalWrite(MOTOR_R, H);
        }else{
            digitalWrite(MOTOR_R, L);
        }
    }else{
        digitalWrite(MOTOR_L, L);   
        digitalWrite(MOTOR_R, L);   
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
void Control::read_bt_volt(){
    // bt_adc_raw = analogRead(BT_VOLT_READ);
    bt_adc_raw = analogRead(A6);
    bt_read_value = (float)bt_adc_raw/ 4095 * 3.3 * 2.12;
    bt_cap = bt_read_value/4.2 *100;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
void Control::read_bt_charge(){
    bt_charge_det = digitalRead(CHARGE_DETECT);
}