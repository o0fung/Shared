#ifndef __IMU_H__
#define __IMU_H__

#include "ICM_20948.h"
#include "Data.h"

#define CUTOFF_FREQ         3.0
#define SAMPLE_TIME         20000.0
#define ONE_SECOND          1000000.0
#define BUFFER_SIZE         3
#define RADIAN_TO_DEGREE    180.0 / PI
#define DEGREE_TO_RADIAN    PI / 180.0
#define GRAVITY_TO_ACC      9.81 / 1000.0
#define FILTER_WEIGHT       0.98
#define SIZE_INTEGER        2
#define OFFSET_ACC_COUNT    50
// #define MIN_ACCEL_TO_MOVE   0.5
#define MIN_ACCEL_TO_MOVE   0
// #define MIN_VEL_TO_MOVE     0.5
#define MIN_VEL_TO_MOVE     0

class IMU {
    private:
        uint8_t _addr;
        uint8_t _ad0;
        ICM_20948_I2C *_icm;
        bool _is_working;
        bool _has_offset_acc;
        uint8_t _offset_acc_count;

        struct Offset {
            float x;
            float y;
            float z;
        } _accel_offset, _accel_offset_sum;

    public:
        IMU(ICM_20948_I2C *icm);
        
        typedef union {
            int16_t integer;
            uint8_t binary[SIZE_INTEGER];
        } binaryint;

        struct Sensor {
            Data <float, BUFFER_SIZE> x;
            Data <float, BUFFER_SIZE> y;
            Data <float, BUFFER_SIZE> z;
        } acc, gyr, mag;

        struct Tilt {
            Data <float, BUFFER_SIZE> x;
            Data <float, BUFFER_SIZE> y;
            Data <float, BUFFER_SIZE> z;
        } ang;

        struct Acceleration {
            Data <float, BUFFER_SIZE> x;
            Data <float, BUFFER_SIZE> y;
            Data <float, BUFFER_SIZE> z;
        } accel, accel_filt;

        struct Velocity {
            Data <float, BUFFER_SIZE> x;
            Data <float, BUFFER_SIZE> y;
            Data <float, BUFFER_SIZE> z;
        } vel, vel_filt;

        struct Displacement {
            Data <float, BUFFER_SIZE> x;
            Data <float, BUFFER_SIZE> y;
            Data <float, BUFFER_SIZE> z;
        } pos, pos_filt;
        
        void init(uint8_t addr, uint8_t ad0);
        void reset();
        
        void set_working(bool val);
        bool get_working();
        
        void set_has_offset_acc(bool val);
        bool get_has_offset_acc();

        template <typename any>
        void set_filters(any *sensor);
        void set_filters_all();

        template <typename any>
        void clear(any *data);
        void clear_all();
        
        void update();
        void acquire();
        void lowpass_filter();
        void compute_tilt_angles();
        void compute_head_angle();
        void compute_linear_acc();
        void compute_linear_vel();
        void compute_linear_pos();
        
        void offset_accel();
        void set_zero_pos();
};

#endif
