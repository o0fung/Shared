#include <Arduino.h>
#include <Wire.h>
#include "System.h"

System::System() {};

void System::init(uint8_t pin_i2c_dat, uint8_t pin_i2c_clk) {
    status = 0;

    _count = 0;
    _pin_i2c_dat = pin_i2c_dat;
    _pin_i2c_clk = pin_i2c_clk;
    
    Wire.begin(_pin_i2c_dat, _pin_i2c_clk, FREQ_I2C_FOR_ICM);
}

bool System::check_time(uint32_t *timer, uint32_t period) {
    // update the time passed since the last timer check
    // notify if a particular time period has passed
    _dt = get_time_diff(timer);
    if (_dt >= period) {
        // update the timer at the timed interval
        *timer = micros();
        return true;
    }
    return false;
}

uint32_t System::get_time_diff(uint32_t *timer) {
    return micros() - *timer;
}

uint32_t System::count_up() {
    _count = (_count == 0xFFFFFFFF) ? 0 : _count + 1;
    return _count;
}

uint32_t System::get_count() {
    return _count;
}

uint32_t System::get_dt() {
    return _dt;
}

void System::set_version(char *ver) {
    _version = ver;
}

char* System::get_version() {
    return _version;
}

void System::set_development(char *ver) {
    _development = ver;
}

char* System::get_development() {
    return _development;
}

void System::set_enable_version(bool val) {
    _enable_send_version = val;
}

void System::set_enable_development(bool val) {
    _enable_send_development = val;
}

void System::set_enable_setting(bool val) {
    _enable_send_setting = val;
}

void System::set_enable_status(bool val) {
    _enable_send_status = val;
}

void System::set_enable_data(bool val) {
    _enable_send_data = val;
}

bool System::get_enable_development() {
    return _enable_send_development;
}

bool System::get_enable_version() {
    return _enable_send_version;
}

bool System::get_enable_setting() {
    return _enable_send_setting;
}

bool System::get_enable_status() {
    return _enable_send_status;
}

bool System::get_enable_data() {
    return _enable_send_data;
}

Cable::Cable() {};

void Cable::init(uint8_t pin_cable_conn) {
    _is_connected_cable = false;
    _current_cable_conn = false;
    _pin_cable_conn = pin_cable_conn;
    pinMode(_pin_cable_conn, INPUT_PULLUP);
}

bool Cable::check(uint8_t *status) {
    // default: pull-up meaning cable not connected
    // connected: pull-down
    _current_cable_conn = ! digitalRead(_pin_cable_conn);
    if (_current_cable_conn != _is_connected_cable) {
        if (_current_cable_conn) {
            *status = *status | 0x04;        // cable connected (0xb00000100) 
        }
        else {
            *status = *status & 0xFB;        // cable disconnected (0xb11111011) 
        }
        _is_connected_cable = _current_cable_conn;
    }
    return _is_connected_cable;
}

bool Cable::get_cable_conn() {
    return _is_connected_cable;
}

Brace::Brace() {};

void Brace::init(uint8_t addr) {
    _is_connected_brace = false;
    _current_brace_conn = false;
    _side = 0;
    _current_side = 0;
    _addr = addr;
    _err = 0;
}

bool Brace::check_conn(uint8_t *status) {
    // tap the I2C to confirm connection
    Wire.beginTransmission(_addr);
    _err = Wire.endTransmission();
    
    _current_brace_conn = (_err == 0);
    if (_current_brace_conn != _is_connected_brace) {
        if (_current_brace_conn) {
            *status = *status | 0x06;        // brace connected (0xb00000110) 
        }
        else {
            *status = *status & 0xFC;        // brace disconnect (0xb11111100) 
        }
        _is_connected_brace = _current_brace_conn;
    }
    return _is_connected_brace;
}

bool Brace::check_side(uint8_t *status) {
    // send a byte via I2C
    Wire.beginTransmission(_addr);
    Wire.write(0x00);
    _err = Wire.endTransmission();
    // request a byte from I2C, which contains info about L/R side
    Wire.requestFrom(_addr, (uint8_t) 1);
    _current_side = (Wire.available()) ? Wire.read() : 0;


    if (_current_side != _side) {
        // update the L/R side setting
        switch (_current_side) {
            case 2:     // Right
                *status = *status | 0x01;        // right brace (0xb00000001) 
                break;
            case 1:     // Left
                *status = *status & 0xFE;        // left brace (0xb11111110) 
                break;
            case 0:     // Not Connected
            default:
                *status = *status & 0xFC;        // brace disconnect (0xb11111100) 
                break;
        }
        _side = _current_side;
    }
    return _side;
}

bool Brace::get_brace_conn() {
    return _is_connected_brace;
}

bool Brace::get_side_lr() {
    return _side;
}

