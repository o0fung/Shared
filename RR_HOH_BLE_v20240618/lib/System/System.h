#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#define FREQ_I2C_FOR_ICM        400000

class System {
    // update system operation
    private:
        uint32_t _dt;
        uint8_t _pin_i2c_dat;
        uint8_t _pin_i2c_clk;
        
        bool _enable_send_development;
        bool _enable_send_version;
        bool _enable_send_setting;
        bool _enable_send_status;
        bool _enable_send_data;

        uint32_t _count;
        
        char *_version;
        char *_development;

    public:
        System();
        
        uint8_t status;
        
        void init(uint8_t pin_i2c_dat, uint8_t pin_i2c_clk);
        bool check_time(uint32_t *timer, uint32_t period);
        uint32_t get_time_diff(uint32_t *timer);

        uint32_t count_up();
        uint32_t get_count();

        uint32_t get_dt();

        void set_version(char *ver);
        char* get_version();

        void set_development(char *ver);
        char* get_development();

        void set_enable_development(bool val);
        void set_enable_version(bool val);
        void set_enable_setting(bool val);
        void set_enable_status(bool val);
        void set_enable_data(bool val);
        bool get_enable_development();
        bool get_enable_version();
        bool get_enable_setting();
        bool get_enable_status();
        bool get_enable_data();
};

class Cable {
    // check cable connectivity
    private:
        bool _is_connected_cable;
        bool _current_cable_conn;
        uint8_t _pin_cable_conn;

    public:
        Cable();
        void init(uint8_t pin_cable_conn);
        bool check(uint8_t *status);
        bool get_cable_conn();
};

class Brace {
    // check brace connectivity and operating side
    private:
        bool _is_connected_brace;
        bool _current_brace_conn;
        uint8_t _side;
        uint8_t _current_side;
        uint8_t _addr;
        uint8_t _err;

    public:
        Brace();
        void init(uint8_t addr);
        bool check_conn(uint8_t *status);
        bool check_side(uint8_t *status);
        bool get_brace_conn();
        bool get_side_lr();
};

#endif
