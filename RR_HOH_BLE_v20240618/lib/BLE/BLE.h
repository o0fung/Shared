#ifndef __BLE_H__
#define __BLE_H__

#include "NimBLEDevice.h"
#include "Motor.h"
#include "COMM.h"

#define CHARACTERISTIC_STRING_SIZE      37
#define DATA_BUFFER_SIZE                50

class BLE;

class ServerCallback: public BLEServerCallbacks {
    // Configure the callback function
    // when BLE is connected or disconnected
    private:
        BLE *_ptr_ble;
        Motor *_ptr_motor;

    public:
        void init(BLE *ptr_ble, Motor *ptr_mot);
        void onConnect(BLEServer *ptr_Server);
        void onDisconnect(BLEServer *ptr_Server);
};

class CharacteristicCallback: public BLECharacteristicCallbacks {
    // Configure the call back function
    // when BLE received data at the rx characteristics 
    private:
        COMM *_ptr_comm;

    public:
        void init(COMM *comm);
        void onWrite(NimBLECharacteristic* ptr_Characteristic);
};

class BLE {
    // Control the BLE operations
    private:
        NimBLEServer* _ptr_server;
        NimBLEService* _ptr_service;
        NimBLECharacteristic* _ptr_tx_characteristic;
        NimBLECharacteristic* _ptr_rx_characteristic;
        Motor *_ptr_motor;
        COMM *_ptr_comm;

        uint8_t _buffer_data[DATA_BUFFER_SIZE];
        uint8_t _buffer_size;

        bool _is_connected_ble;

    public:
        BLE(Motor *ptr_motor, COMM *ptr_comm);
        void init_uuid(char *name, char *service, char *char_tx, char *char_rx);
        void init_callback(ServerCallback *server_callback, CharacteristicCallback * characteristic_callback);
        void set_connected(bool val);
        void send();
        void send_status();
        void send_data();
        void send_development();
        void send_version();
        void send_setting();
        void clear_buffer();
};

#endif
