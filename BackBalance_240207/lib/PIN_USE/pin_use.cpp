#include "pin_use.h"

void pin_used:: pinsetup(){
  pinMode(MOTOR_L, OUTPUT);
  pinMode(MOTOR_R, OUTPUT);

  pinMode(MOTOR_L, LOW);
  pinMode(MOTOR_R, LOW);
  
  pinMode(SET_ZERO_BTN, INPUT_PULLUP);
  pinMode(CHARGE_DETECT, INPUT_PULLUP);
}