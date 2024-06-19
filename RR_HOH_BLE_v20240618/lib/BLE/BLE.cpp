#include <Arduino.h>
#include "BLE.h"
#include "NimBLEDevice.h"
#include "Motor.h"

BLE::BLE(Motor *ptr_motor, COMM *ptr_comm) {
    _ptr_motor = ptr_motor;
    _ptr_comm = ptr_comm;
}

void BLE::init_uuid(char *name, char *service, char *char_tx, char *char_rx) {
    // set up the server
    _ptr_server = NimBLEDevice::createServer();
    // set up the service
    _ptr_service = _ptr_server->createService(service);
    // set up the tx/rx characteristics
    _ptr_tx_characteristic = _ptr_service->createCharacteristic(char_tx, NIMBLE_PROPERTY::NOTIFY);
    _ptr_rx_characteristic = _ptr_service->createCharacteristic(char_rx, NIMBLE_PROPERTY::WRITE);
    // start the service and advertisment
    _ptr_service->start();
    _ptr_server->getAdvertising()->start();
}

void BLE::init_callback(ServerCallback *server_callback, CharacteristicCallback *characteristic_callback) {
    // apply the customized callback function
    _ptr_server->setCallbacks(server_callback);
    _ptr_rx_characteristic->setCallbacks(characteristic_callback);
}

void BLE::set_connected(bool val) {
    _is_connected_ble = val;
    if (_is_connected_ble) {
        // not allow external device to find the BLE anymore
        // if the device has already connected to one device
        Serial.println(">> BLE Connected...");
        _ptr_server->getAdvertising()->stop();
        clear_buffer();
    }
    else {
        // allow external devices to find the BLE if disconnected
        // stop the motor output if BLE disconnected
        Serial.println(">> BLE Disconnected...");
        _ptr_server->startAdvertising();
        _ptr_motor->mass_stop();
    }
}

void BLE::send() {
    _ptr_tx_characteristic->setValue(_buffer_data, _buffer_size);
    _ptr_tx_characteristic->notify();
    clear_buffer();
}

void BLE::send_status() {
    _ptr_comm->encode_status(_buffer_data, &_buffer_size);
    send();
}

void BLE::send_data() {
    _ptr_comm->encode_data(_buffer_data, &_buffer_size);
    send();
}

void BLE::send_version() {
    _ptr_comm->encode_version(_buffer_data, &_buffer_size);
    send();
}

void BLE::send_development() {
    _ptr_comm->encode_development(_buffer_data, &_buffer_size);
    send();
}

void BLE::send_setting() {
    _ptr_comm->encode_setting(_buffer_data, &_buffer_size);
    send();
}

void BLE::clear_buffer() {
    for (uint8_t i = 0; i < DATA_BUFFER_SIZE; i++) {
        _buffer_data[i] = 0;
    }
    _buffer_size = 0;
}

void ServerCallback::init(BLE *ble, Motor *mot) {
    _ptr_ble = ble;
    _ptr_motor = mot;
}

void ServerCallback::onConnect(BLEServer *ptr_Server) {
    _ptr_ble->set_connected(true);
    _ptr_motor->cpm_mass(1);
}

void ServerCallback::onDisconnect(BLEServer *ptr_Server) {
    _ptr_ble->set_connected(false);
    _ptr_motor->reset_states();
    _ptr_motor->reset_skips();
}

void CharacteristicCallback::init(COMM *comm) {
    _ptr_comm = comm;
}

void CharacteristicCallback::onWrite(NimBLECharacteristic* ptr_Characteristic) {
    _ptr_comm->decode(ptr_Characteristic->getValue().data(), ptr_Characteristic->getValue().length());
}
