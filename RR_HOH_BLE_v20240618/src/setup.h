#ifndef __SETUP_H__
#define __SETUP_H__

#define DEVICE_NAME     (char*) "RR_HOH_BLE"
#define VERSION         (char*) "V1.0.0"
#define DEVELOPMENT     (char*) "v20240531"
#define BAUDRATE        115200

#define LOOP_TIME       20000
// #define LOOP_TIME       100000
#define LOOP_ONE_SEC    1000000

#define SERVICE         (char*) "484f4801-4200-4c00-4500-000032303233"
#define CHAR_RX         (char*) "484f4802-4200-4c00-4500-000032303233"
#define CHAR_TX         (char*) "484f4803-4200-4c00-4500-000032303233"

#define IMU_ADRR        0x68
#define HB_ADRR         0x50

#define CABLE_DETECT    17

#define I2C_DATA        21
#define I2C_CLOCK       22
#define AD0_VAL         0

#define SPI_MOSI        19
#define SPI_MISO        23
#define SPI_CLK         30
#define SPI_SS1         5           // SPI CS for MSP3208
#define SPI_SS2         32          // SPI CS for IMU

#define ADC_VREF        3300        // ADC 2.3V Vref
#define ADC_CLK         1600000     // SPI Clock 1.6MHz
#define MSBFIRST        1
#define SPI_MODE0       0

#define M1_IN1          33      
#define M2_IN1          26      
#define M3_IN1          14      
#define M4_IN1          13      
#define M5_IN1          4       

#define M1_IN2          25      
#define M2_IN2          27      
#define M3_IN2          12      
#define M4_IN2          15      
#define M5_IN2          16   

uint8_t M_IN[5][2] = {
    {M1_IN1, M1_IN2},
    {M2_IN1, M2_IN2},
    {M3_IN1, M3_IN2},
    {M4_IN1, M4_IN2},
    {M5_IN1, M5_IN2},
};

#endif