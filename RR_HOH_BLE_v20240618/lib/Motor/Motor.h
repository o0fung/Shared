#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "Mcp320x.h"
#include "System.h"
#include "Data.h"
#include "Memory.h"

#define MIN_RESPONSE_TIME           1000        // just set a placeholder value
#define RESET_TIME                  3000000     // motor will reset to extreme position after moving for 3s
#define THRESHOLD_DT_FULL_ROM       1000000     // motor calibration will only measure current output after 0.5s
#define THRESHOLD_RESET_MIN_TIME    500000
#define DEFAULT_DT_FULL_ROM         2340000     
#define DEFAULT_CPM_WAIT_TIME       1000000
#define DT_SEQUENTIAL_FINGERS       500000

#define N_FINGERS                   5           // there are five fingers in HOH
#define THRESHOLD_MOT_CUR           10          // motor calibration detect finger stopped movement at extreme for value of 10
#define BUFFER_SIZE_MOT             3

#define TARGET_POS_MIN              0
#define TARGET_POS_MAX              100
#define DEFAULT_LIMIT_FLEXION       95
#define DEFAULT_LIMIT_EXTENSION     5
#define THRESHOLD_MIN_POS_DIFF      1

#define MEM_DT_FULL_ROM             "DT_FULL_ROM"

class Motor {
    // Control motor operations
    private:
        uint8_t _pins_fingers_ctrl[N_FINGERS][2];
        uint8_t _pin_spi_gpio;
        
        MCP3208 *_ptr_adc;
        System *_ptr_sys;
        Memory *_ptr_mem;
        
        uint16_t _cur_adc[N_FINGERS];
        Data <uint16_t, BUFFER_SIZE_MOT> _cur_val[N_FINGERS];

        bool _has_current[N_FINGERS];
        bool _skip[N_FINGERS];

        uint8_t _target_pos[N_FINGERS];
        int8_t _rep_to_go;

        uint8_t _limit_flexion[N_FINGERS];
        uint8_t _limit_extension[N_FINGERS];

        uint32_t _dt;
        uint32_t _timer;
        uint32_t _timer2;
        uint32_t _countdown;

        uint32_t _dt_full_rom = DEFAULT_DT_FULL_ROM;
        uint32_t _dt_wait_flex = DEFAULT_CPM_WAIT_TIME;
        uint32_t _dt_wait_ext = DEFAULT_CPM_WAIT_TIME;

        bool _is_cpm_sequential;
        bool _is_cpm_from_thumb;

    public:
        Motor(System *ptr_sys, Memory *ptr_memory);

        enum State {
            IDLE = 0,
            RESET_FLEX,
            RESET_EXT,
            CALIBRATE,
            FREE,
            CPM,
            CPM_SEQ,
            CPM_SEQ_REV,
            CPM_ONCE
        } state;

        enum State_Calibrate {
            CAL_END = 0,
            CAL_START,
            CAL_RESET_EXT,
            CAL_RESET_FLEX,
            CAL_TEST_FLEX,
            CAL_TEST_EXT
        } state_calibrate;

        enum State_CPM {
            CPM_END = 0,
            CPM_START,
            CPM_RESET_EXT,
            CPM_PAUSE,
            CPM_WAIT_FLEX,
            CPM_LOOP_FLEX,
            CPM_WAIT_EXT,
            CPM_LOOP_EXT,
            CPM_SEQ_FLEX,
            CPM_SEQ_EXT
        } state_cpm, state_cpm_prev;
        
        float pos[N_FINGERS];
        bool is_moving[N_FINGERS];
        bool is_flexing[N_FINGERS];

        void init_adc(MCP3208 *adc);
        void init_mot_ctrl(uint8_t pins_fingers_ctrl[N_FINGERS][2]);
        void init_cur_read(uint8_t pin_spi_gpio);
        void init_control();
        void reset_states();

        bool check_time();
        uint32_t get_time_diff();
        uint32_t get_time_diff2();
        void set_zero_timer();
        void set_zero_timer2();
        void to_default_timer();

        void set_countdown(uint32_t val);
        uint32_t get_countdown();

        void set_state(Motor::State val);
        Motor::State get_state();

        void set_state_calibrate(Motor::State_Calibrate val);
        Motor::State_Calibrate get_state_calibrate();

        void set_state_cpm(Motor::State_CPM val);
        Motor::State_CPM get_state_cpm();

        void set_state_cpm_prev(Motor::State_CPM val);
        Motor::State_CPM get_state_cpm_prev();

        void set_dt_full_rom(uint32_t val);
        uint32_t get_dt_full_rom();

        void set_skip(uint8_t n, bool val);
        void reset_skips();

        void set_rep_to_go(int8_t n);
        void minus_rep_to_go();
        uint8_t get_rep_to_go();

        void flexion(uint8_t n);
        void extension(uint8_t n);
        void stop(uint8_t n);
        
        void reset_to_flexion();
        void reset_to_extension();
        void loop_reset(bool is_zero);
        bool check_early_release();
        
        void calibration();
        void loop_calibration();

        void mass_flexion(bool is_limited);
        void mass_extension(bool is_limited);
        void mass_move(uint8_t target_percent[N_FINGERS], bool is_limited[N_FINGERS]);
        void mass_stop();
        void loop_free();

        void cpm_mass();
        void cpm_mass(uint8_t n);
        void cpm_mass(bool is_sequential, bool is_from_thumb);
        void loop_cpm();

        void set_has_current(bool val, uint8_t n);
        void set_limit_flexion(uint8_t val, uint8_t n);
        void set_limit_extension(uint8_t val, uint8_t n);
        void set_dt_wait_flex(uint32_t val);
        void set_dt_wait_ext(uint32_t val);
        bool get_has_current(uint8_t n);
        uint8_t get_limit_flexion(uint8_t n);
        uint8_t get_limit_extension(uint8_t n);
        uint32_t get_dt_wait_flex();
        uint32_t get_dt_wait_ext();
        
        void read_motor_current();
        void update_pos();
};

#endif
