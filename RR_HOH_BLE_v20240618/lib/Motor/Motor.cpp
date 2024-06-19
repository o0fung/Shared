#include <Arduino.h>
#include "Motor.h"
#include "Mcp320x.h"
#include "System.h"
#include "Memory.h"

Motor::Motor(System *ptr_sys, Memory *ptr_memory) {
    _ptr_sys = ptr_sys;
    _ptr_mem = ptr_memory;
}

void Motor::init_adc(MCP3208 *adc) {
  _ptr_adc = adc;
}

void Motor::init_mot_ctrl(uint8_t pins_fingers_ctrl[N_FINGERS][2]) {
    // Initiate motor control pins
    for (uint8_t i = 0; i < N_FINGERS; i++) {
        for (uint8_t j = 0; j < 2; j++) {
            _pins_fingers_ctrl[i][j] = pins_fingers_ctrl[i][j];
            pinMode(_pins_fingers_ctrl[i][j], OUTPUT);
        }
    }
}

void Motor::init_cur_read(uint8_t pin_spi_gpio) {
    // Initiate motor current read pin
    _pin_spi_gpio = pin_spi_gpio;
    pinMode(_pin_spi_gpio, OUTPUT);
    digitalWrite(_pin_spi_gpio, HIGH);
}

void Motor::init_control() {
    for (uint8_t i = 0; i < N_FINGERS; i++) {
        _cur_val->clear();
        _limit_flexion[i] = DEFAULT_LIMIT_FLEXION;
        _limit_extension[i] = DEFAULT_LIMIT_EXTENSION;
    }

    set_rep_to_go(0);
    set_dt_full_rom(_ptr_mem->load_uint32(MEM_DT_FULL_ROM, DEFAULT_DT_FULL_ROM));

    reset_states();
    reset_skips();
    set_countdown(MIN_RESPONSE_TIME);
    set_zero_timer();
    set_zero_timer2();
}

void Motor::reset_states() {
    set_state(IDLE);
    set_state_calibrate(CAL_END);
    set_state_cpm(CPM_END);
}

bool Motor::check_time() {
    // update the time passed since the last timer check
    // notify if a particular time period has passed
    _dt = get_time_diff();
    if (_dt >= _countdown) {
        // update the timer at the timed interval
        set_zero_timer();
        return true;
    }
    return false;
}

uint32_t Motor::get_time_diff() {
    return micros() - _timer;
}

void Motor::set_zero_timer() {
    _timer = micros();
}

uint32_t Motor::get_time_diff2() {
    return micros() - _timer2;
}

void Motor::set_zero_timer2() {
    _timer2 = micros();
}

void Motor::to_default_timer() {
    set_countdown(MIN_RESPONSE_TIME);
    set_state(IDLE);
}

void Motor::set_countdown(uint32_t val) {
    _countdown = val;
}

uint32_t Motor::get_countdown() {
    return _countdown;
}

void Motor::set_state(Motor::State val) {
    state = val;
}

Motor::State Motor::get_state() {
    return state;
}

void Motor::set_state_calibrate(Motor::State_Calibrate val) {
    state_calibrate = val;
}

Motor::State_Calibrate Motor::get_state_calibrate() {
    return state_calibrate;
}

void Motor::set_state_cpm(Motor::State_CPM val) {
    state_cpm = val;
}

Motor::State_CPM Motor::get_state_cpm() {
    return state_cpm;
}

void Motor::set_state_cpm_prev(Motor::State_CPM val) {
    state_cpm_prev = val;
}

Motor::State_CPM Motor::get_state_cpm_prev() {
    return state_cpm_prev;
}

void Motor::set_dt_full_rom(uint32_t val) {
    _dt_full_rom = val;
}

uint32_t Motor::get_dt_full_rom() {
    return _dt_full_rom;
}

void Motor::set_skip(uint8_t n, bool val) {
    _skip[n] = val;
}

void Motor::reset_skips() {
    for (uint8_t i = 0; i < N_FINGERS; i++) {
        _skip[i] = false;
    }
}

void Motor::set_rep_to_go(int8_t n) {
    _rep_to_go = n;
}

void Motor::minus_rep_to_go() {
    if (_rep_to_go > 0) {
        _rep_to_go--;
    }
}

uint8_t Motor::get_rep_to_go() {
    return _rep_to_go;
}

void Motor::flexion(uint8_t n) {
    // move target finger to flexion direction
    digitalWrite(_pins_fingers_ctrl[n][0], HIGH);
    digitalWrite(_pins_fingers_ctrl[n][1], LOW);
}

void Motor::extension(uint8_t n) {
    // move target finger to extension direction
    digitalWrite(_pins_fingers_ctrl[n][0], LOW);
    digitalWrite(_pins_fingers_ctrl[n][1], HIGH);
}

void Motor::stop(uint8_t n) {
    // stop target finger
    digitalWrite(_pins_fingers_ctrl[n][0], LOW);
    digitalWrite(_pins_fingers_ctrl[n][1], LOW);
}

void Motor::reset_to_flexion() {
    // move all fingers to flexion direction for 3 seconds
    // will push the motor to one extreme (flexion)
    for (uint8_t i = 0; i < N_FINGERS; i++) {
        if (_skip[i]) {
            continue;
        }
        flexion(i);
        is_flexing[i] = true;
        is_moving[i] = true;
    }
    set_countdown(RESET_TIME);
    set_state(RESET_FLEX);
    Serial.println(">> Motors are moving to full flexion...");
    set_zero_timer();
}

void Motor::reset_to_extension() {
    // move all fingers to extension direction for 3 seconds
    // will push the motor to one extreme (extension)
    for (uint8_t i = 0; i < N_FINGERS; i++) {
        if (_skip[i]) {
            continue;
        }
        extension(i);
        is_flexing[i] = false;
        is_moving[i] = true;
    }
    set_countdown(RESET_TIME);
    set_state(RESET_EXT);
    Serial.println(">> Motors are moving to full extension...");
    set_zero_timer();
}

void Motor::loop_reset(bool is_zero) {
    // stop after the timer reached the target duration
    if ((!check_time()) && (!check_early_release())) {
        return;
    }

    mass_stop();
    to_default_timer();
    for (uint8_t i = 0; i < N_FINGERS; i++) {
        if (_skip[i]) {
            continue;
        }
        pos[i] = (is_zero) ? 0.0 : 100.0;
    }
    Serial.println(">> Motors reset completed.");
    reset_skips();
    reset_states();
    read_motor_current();
}

bool Motor::check_early_release() {
    uint8_t all_moving = 0x1F;  // check if all fingers reached target pos

    read_motor_current();
    _dt = get_time_diff();
    if (_dt > THRESHOLD_RESET_MIN_TIME) {
        for (uint8_t i = 0; i < N_FINGERS; i++) {
            if (_skip[i]) {
                bitClear(all_moving, i);
                continue;
            }
            // when current of a finger dropped below certain threshold
            if (!_has_current[i]) {
                is_moving[i] = false;
                bitClear(all_moving, i);
                stop(i);
            }
        }
    }

    return (all_moving == 0);
}

void Motor::calibration() {
    set_state(CALIBRATE);
    set_state_calibrate(CAL_START);
}

void Motor::loop_calibration() {
    Motor::State current_state = state;
    switch (state_calibrate) {
        
        case CAL_START:
            // first reset the motors to full extension
            reset_to_extension();
            set_state(current_state);
            set_state_calibrate(CAL_RESET_EXT);
            break;

        case CAL_RESET_EXT:
            // stop after the fingers fully extended
            if ((!check_time()) && (!check_early_release())) {
                return;
            }

            mass_stop();
            set_state_calibrate(CAL_RESET_FLEX);

            break;

        case CAL_RESET_FLEX:
            // second push the motors from full extension to full flexion
            // start counting the time to move a full range of motion
            reset_to_flexion();
            set_state(current_state);
            set_state_calibrate(CAL_TEST_FLEX);
            break;

        case CAL_TEST_FLEX: {
            // stop after the fingers fully extended
            if (check_time()) {
                mass_stop();
                set_state_calibrate(CAL_END);
            }
            // measure the current supplied to the motor
            read_motor_current();
            _dt = get_time_diff();
            if (_dt > THRESHOLD_DT_FULL_ROM) {
                for (uint8_t i = 0; i < N_FINGERS; i++) {
                    // when current of a finger dropped below certain threshold
                    if (!_has_current[i]) {
                        mass_stop();
                        set_dt_full_rom(_dt);
                        _ptr_mem->save_uint32(MEM_DT_FULL_ROM, _dt);
                        set_state_calibrate(CAL_TEST_EXT);
                        break;
                    }
                }
            }
            break;
        }

        case CAL_TEST_EXT: {
            Serial.print(">> Calibrated Full ROM Time (in micro second): ");
            Serial.println(get_dt_full_rom());
            // now the fingers are all at full flexion, i.e. pos 100
            for (uint8_t i = 0; i < N_FINGERS; i++) {
                pos[i] = 100.0;
            }
            // test the calibrated results by move fingers to full extension
            mass_extension(true);
            
            // end the calibration
            set_state_calibrate(CAL_END);
            break;
        }

        case CAL_END:
        default:
            // end the calibration for unexpected events
            reset_states();
            reset_skips();
            break;
    }
}

void Motor::mass_flexion(bool is_limited) {
    uint8_t tpos[N_FINGERS] = {100, 100, 100, 100, 100};
    bool tlim[N_FINGERS] = {is_limited, is_limited, is_limited, is_limited, is_limited};
    mass_move(tpos, tlim);
}

void Motor::mass_extension(bool is_limited) {
    uint8_t tpos[N_FINGERS] = {0, 0, 0, 0, 0};
    bool tlim[N_FINGERS] = {is_limited, is_limited, is_limited, is_limited, is_limited};
    mass_move(tpos, tlim);
}

void Motor::mass_move(uint8_t target_percent[N_FINGERS], bool is_limited[N_FINGERS]) {
    for (uint8_t i = 0; i < N_FINGERS; i++) {
        
        // limit target percent to range of 0 - 100
        target_percent[i] = (target_percent[i] >= TARGET_POS_MAX) ? TARGET_POS_MAX : target_percent[i];
        target_percent[i] = (target_percent[i] <= TARGET_POS_MIN) ? TARGET_POS_MIN : target_percent[i];
        // register the target percent for each fingers
        _target_pos[i] = target_percent[i];

        if (is_limited[i]) {
            // limit target pos to specified limit
            _target_pos[i] = (_target_pos[i] >= _limit_flexion[i]) ? _limit_flexion[i] : _target_pos[i];
            _target_pos[i] = (_target_pos[i] <= _limit_extension[i]) ? _limit_extension[i] : _target_pos[i];
        }
    }
    set_state(FREE);
    set_zero_timer();
}

void Motor::mass_stop() {
    for (uint8_t i = 0; i < N_FINGERS; i++) {
        stop(i);
        is_moving[i] = false;
        _target_pos[i] = pos[i];
    }
}

void Motor::loop_free() {
    uint8_t all_moving = 0x1F;  // check if all fingers reached target pos
    
    for (uint8_t i = 0; i < N_FINGERS; i++) {
        if (abs((int) pos[i] - _target_pos[i]) < THRESHOLD_MIN_POS_DIFF) {
            is_moving[i] = false;
            bitClear(all_moving, i);
            stop(i);
        }
        else if ((int) pos[i] < _target_pos[i]) {
            is_flexing[i] = true;
            is_moving[i] = true;
            flexion(i);
        }
        else if ((int) pos[i] > _target_pos[i]) {
            is_flexing[i] = false;
            is_moving[i] = true;
            extension(i);
        }
        else {
            is_moving[i] = false;
            stop(i);
            reset_states();
            reset_skips();
        }
    }

    if (all_moving == 0) {
        reset_states();
        reset_skips();
    }
    
    _dt = get_time_diff();
    update_pos();
    set_zero_timer();
}

void Motor::cpm_mass() {
    _is_cpm_sequential = false;
    set_rep_to_go(-1);
    set_state(CPM);
    set_state_cpm(CPM_START);
}

void Motor::cpm_mass(uint8_t n) {
    cpm_mass();
    set_rep_to_go(n);
}

void Motor::cpm_mass(bool is_sequential, bool is_from_thumb) {
    cpm_mass();
    _is_cpm_sequential = is_sequential;
    _is_cpm_from_thumb = is_from_thumb;
    if (_is_cpm_sequential) {
        set_state(CPM_SEQ);
        if (is_from_thumb) {
            set_state(CPM_SEQ_REV);
        }
    }
}

void Motor::loop_cpm() {
    Motor::State current_state = state;
    switch (state_cpm) {

        case CPM_START:
            // first reset the motors to full extension
            reset_to_extension();
            set_state(current_state);
            set_state_cpm(CPM_RESET_EXT);
            break;

        case CPM_RESET_EXT:
            // stop after the fingers fully extended
            if ((!check_time()) && (!check_early_release())) {
                return;
            }

            mass_stop();
            // set the current finger positions to 0% flexion
            for (uint8_t i = 0; i < N_FINGERS; i++) {
                if (_skip[i]) {
                    continue;
                }
                pos[i] = 0.0;
            }
            // next step is to wait for certain cpm time for flexion
            set_countdown(get_dt_wait_flex());
            set_state_cpm(CPM_WAIT_FLEX);
            
            break;

        case CPM_PAUSE:
            set_zero_timer();
            set_zero_timer2();
            break;

        case CPM_WAIT_FLEX:
            if (_rep_to_go == 0) {
                mass_stop();
                reset_states();
                reset_skips();
                break;
            }

            if (check_time()) {
                // after waiting for certain time
                // assign to move the fingers to flexion direction
                mass_flexion(true);
                set_state(current_state);
                if (_is_cpm_sequential) {
                    set_state_cpm(CPM_SEQ_FLEX);
                }
                else {
                    set_state_cpm(CPM_LOOP_FLEX);
                }
                set_zero_timer();
                set_zero_timer2();
            }
            break;

        case CPM_SEQ_FLEX:
        case CPM_LOOP_FLEX: {
                uint8_t all_moving = 0x1F;  // check if all fingers reached target pos
                uint8_t i = 0;

                for (uint8_t ii = 0; ii < N_FINGERS; ii++) {
                    if (_is_cpm_sequential) {
                        i = (_is_cpm_from_thumb) ? ii : N_FINGERS - 1 - ii;
                        if (get_time_diff2() < DT_SEQUENTIAL_FINGERS * ii) {
                            continue;
                        }
                    }
                    else {
                        i = ii;
                    }
                    
                    if (_skip[i]) {
                        bitClear(all_moving, i);
                        continue;
                    }

                    if ((int) pos[i] < _target_pos[i]) {
                        // move the fingers to target flexion
                        is_flexing[i] = true;
                        is_moving[i] = true;
                        bitSet(all_moving, i);
                        flexion(i);
                    }
                    else {
                        // stop and to next step after the fingers moved to target flexion
                        is_moving[i] = false;
                        bitClear(all_moving, i);
                        stop(i);
                        // set the current finger positions to 100% extension
                        pos[i] = _target_pos[i];
                    }
                }

                if (all_moving == 0x00) {
                    // all fingers have reached the target pos
                    set_countdown(get_dt_wait_ext());
                    set_state_cpm(CPM_WAIT_EXT);
                    set_zero_timer();
                }
                
                _dt = get_time_diff();
                update_pos();
                set_zero_timer();

                break;
            }

        case CPM_WAIT_EXT:
            if (check_time()) {
                // after waiting for certain time
                // assign to move the fingers to extension direction
                mass_extension(true);
                set_state(current_state);
                if (_is_cpm_sequential) {
                    set_state_cpm(CPM_SEQ_EXT);
                }
                else {
                    set_state_cpm(CPM_LOOP_EXT);
                }
                set_zero_timer();
                set_zero_timer2();
            }
            break;

        case CPM_SEQ_EXT:
        case CPM_LOOP_EXT: {
                uint8_t all_moving = 0x1F;  // check if all fingers reached target pos
                uint8_t i = 0;

                for (uint8_t ii = 0; ii < N_FINGERS; ii++) {
                    if (_is_cpm_sequential) {
                        i = (_is_cpm_from_thumb) ? ii : N_FINGERS - 1 - ii;
                        if (get_time_diff2() < DT_SEQUENTIAL_FINGERS * ii) {
                            continue;
                        }
                    }
                    else {
                        i = ii;
                    }
                    
                    if (_skip[i]) {
                        bitClear(all_moving, i);
                        continue;
                    }

                    if ((int) pos[i] > _target_pos[i]) {
                        // move the fingers to target flexion
                        is_flexing[i] = false;
                        is_moving[i] = true;
                        bitSet(all_moving, i);
                        extension(i);
                    }
                    else {
                        // stop and to next step after the fingers moved to target flexion
                        is_moving[i] = false;
                        bitClear(all_moving, i);
                        stop(i);
                        // set the current finger positions to 100% extension
                        pos[i] = _target_pos[i];
                    }
                }
                
                if (all_moving == 0) {
                    // all fingers have reached the target pos
                    minus_rep_to_go();
                    set_countdown(get_dt_wait_flex());
                    set_state_cpm(CPM_WAIT_FLEX);
                    set_zero_timer();
                }

                _dt = get_time_diff();
                update_pos();
                set_zero_timer();

                break;
            }

        default:
            break;
    }
}

void Motor::set_has_current(bool val, uint8_t n) {
    _has_current[n] = val;
}

bool Motor::get_has_current(uint8_t n) {
    return _has_current[n];
}

void Motor::set_limit_flexion(uint8_t val, uint8_t n) {
    if ((val >= TARGET_POS_MIN) && (val <= TARGET_POS_MAX) && (val > _limit_extension[n])) {
        _limit_flexion[n] = val;
    }
}

void Motor::set_limit_extension(uint8_t val, uint8_t n) {
    if ((val >= TARGET_POS_MIN) && (val <= TARGET_POS_MAX) && (val < _limit_flexion[n])) {
        _limit_extension[n] = val;
    }
}

void Motor::set_dt_wait_flex(uint32_t val) {
    _dt_wait_flex = val;
}

void Motor::set_dt_wait_ext(uint32_t val) {
    _dt_wait_ext = val;
}

uint8_t Motor::get_limit_flexion(uint8_t n) {
    return _limit_flexion[n];
}

uint8_t Motor::get_limit_extension(uint8_t n) {
    return _limit_extension[n];
}

uint32_t Motor::get_dt_wait_flex() {
    return _dt_wait_flex;
}

uint32_t Motor::get_dt_wait_ext() {
    return _dt_wait_ext;
}

void Motor::read_motor_current() {
    _cur_adc[0] = _ptr_adc->read(MCP3208::Channel::SINGLE_1);
    _cur_adc[1] = _ptr_adc->read(MCP3208::Channel::SINGLE_2);
    _cur_adc[2] = _ptr_adc->read(MCP3208::Channel::SINGLE_3);
    _cur_adc[3] = _ptr_adc->read(MCP3208::Channel::SINGLE_4);
    _cur_adc[4] = _ptr_adc->read(MCP3208::Channel::SINGLE_5);

    for (uint8_t i = 0; i < N_FINGERS; i++) {
        _cur_val[i].update(_ptr_adc->toAnalog(_cur_adc[i]));
        set_has_current(_cur_val[i].set_mean() > THRESHOLD_MOT_CUR, i);
    }
}

void Motor::update_pos() {
    for (uint8_t i = 0; i < N_FINGERS; i++) {
        if (is_moving[i]) {
            if (is_flexing[i]) {
                pos[i] += _dt * 100.0 / get_dt_full_rom();
            }
            else {
                pos[i] -= _dt * 100.0 / get_dt_full_rom();
            }
        }
    }
}
