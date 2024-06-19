#ifndef __COMM_H__
#define __COMM_H__

#include "Motor.h"
#include "System.h"
#include "IMU.h"

#define DATA_HEADER         0xFF
#define STATUS_DATA_LENGTH  11
#define SETTING_LENGTH      25
#define IMU_DATA_LENGTH     23
#define SIZE_FLOATPT        4

class COMM {
    private:
        uint8_t _byte_status_1;
        uint8_t _byte_status_2;
        uint8_t _byte_status_3;
        uint8_t _byte_motor[5];

        uint8_t _length;
        
        Motor *_ptr_motor;
        System *_ptr_sys;
        IMU *_ptr_imu;

    public:
        COMM(Motor *ptr_motor, System *ptr_sys, IMU *ptr_imu);

        typedef union {
            float floatpt;
            uint32_t integer;
            uint8_t binary[SIZE_FLOATPT];
        } binaryfloat;

        void decode(const uint8_t *buffer, uint16_t length);
        void encode_status(uint8_t *buffer, uint8_t *size);
        void encode_data(uint8_t *buffer, uint8_t *size);
        void encode_development(uint8_t *buffer, uint8_t *size);
        void encode_version(uint8_t *buffer, uint8_t *size);
        void encode_setting(uint8_t *buffer, uint8_t *size);
};

#endif
