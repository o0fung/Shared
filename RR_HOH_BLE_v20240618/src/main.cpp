
#include <Arduino.h>
#include <Preferences.h>
#include <Wire.h>
#include <SPI.h>
#include "setup.h"
#include "Data.h"
#include "NimBLEDevice.h"
#include "ICM_20948.h"
#include "Mcp320x.h"
#include "COMM.h"
#include "BLE.h"
#include "IMU.h"
#include "Motor.h"
#include "System.h"
#include "Memory.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

Memory memory;
MCP3208 adc(ADC_VREF, SPI_SS1);
ICM_20948_I2C icm;
System sys;
Motor motor(&sys, &memory);
static ServerCallback server_callback;
static CharacteristicCallback characteristic_callback;
Cable cable;
Brace brace;
IMU imu(&icm);
COMM comm(&motor, &sys, &imu);
BLE ble(&motor, &comm);

uint32_t timer1 = micros();     // LOOPTIME
uint32_t timer2 = micros();     // LOOP_ONE_SEC
uint16_t counter = 0;

void setup() {
    // to disable brownout detector
    // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    
    Serial.begin(BAUDRATE);

    memory.init(DEVICE_NAME);

    motor.init_adc(&adc);
    motor.init_mot_ctrl(M_IN);
    motor.init_cur_read(SPI_SS1);
    motor.init_control();
    motor.reset_to_flexion();  // to hide the early finger movement at reboot
    
    SPISettings settings(ADC_CLK, MSBFIRST, SPI_MODE0);
    SPI.begin();
    SPI.beginTransaction(settings);

    NimBLEDevice::init(DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    ble.init_uuid(DEVICE_NAME, SERVICE, CHAR_TX, CHAR_RX);
    ble.init_callback(&server_callback, &characteristic_callback);
    
    server_callback.init(&ble, &motor);
    characteristic_callback.init(&comm);
    
    sys.init(I2C_DATA, I2C_CLOCK);
    sys.set_version(VERSION);
    sys.set_development(DEVELOPMENT);
    sys.set_enable_development(false);
    sys.set_enable_version(false);
    sys.set_enable_setting(false);
    sys.set_enable_status(true);
    sys.set_enable_data(true);

    cable.init(CABLE_DETECT);
    brace.init(HB_ADRR);

    imu.init(IMU_ADRR, AD0_VAL);

    Serial.print(">> ");
    Serial.print(DEVICE_NAME);
    Serial.println(" Started...");

    motor.reset_to_extension();  // to reset all fingers to extended position at start
}

void loop() {

    // tasks that performed frequently
    if (sys.check_time(&timer1, LOOP_TIME)) {
        sys.count_up();

        switch (motor.get_state()) {

            case Motor::RESET_FLEX:
                motor.loop_reset(false);
                break;

            case Motor::RESET_EXT:
                motor.loop_reset(true);
                break;

            case Motor::CALIBRATE:
                motor.loop_calibration();
                break;

            case Motor::FREE:
                motor.loop_free();
                break;

            case Motor::CPM:
            case Motor::CPM_SEQ:
            case Motor::CPM_SEQ_REV:
            case Motor::CPM_ONCE:
                motor.loop_cpm();
                break;

            case Motor::IDLE:
            default:
                break;
        }

        // send motor state and finger status
        if (sys.get_enable_status()) {
            ble.send_status();
        }

        // send motion sensor data
        if (sys.get_enable_data()) {
            imu.update();
            if (imu.get_working()) {
                sys.get_time_diff(&timer1);
                ble.send_data();
            }
        }
    }

    // tasks that performed once every second
    if (sys.check_time(&timer2, LOOP_ONE_SEC)) {

        // update connectivity status
        cable.check(&sys.status);
        if (brace.check_conn(&sys.status)) {
            brace.check_side(&sys.status);
        }

        // send version or setting info if requested
        // only send once per request
        if (sys.get_enable_version()) {
            ble.send_version();
            sys.set_enable_version(false);
        }

        if (sys.get_enable_development()) {
            ble.send_development();
            sys.set_enable_development(false);
        }

        if (sys.get_enable_setting()) {
            ble.send_setting();
            sys.set_enable_setting(false);
        }
    }
}
