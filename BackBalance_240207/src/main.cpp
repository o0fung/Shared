// Common library
#include <Arduino.h>
#include <EEPROM.h>
// #include <Wire.h>
#include "../lib/PIN_USE/pin_use.h"
#include "../lib/PARAMETER/parameter.h"
#include "../lib/SIGNAL/Signal.h"
#include "../lib/CONTROL/control.h"
#include "../lib/MPU6050/MPU6050.h"
#include "../lib/MPU6050/I2Cdev.h"

// BLE library
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

pin_used  pu;
Signal    data;
Control   ctrl;

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        rxLen = rxValue.length();
        for(int i=0; i<rxValue.length(); i++){
          rxArray[i] += rxValue[i];
        }  
      }
    }
};

void setup(){
  // - - - - - C O M M O N   S E T T I N G - - - - - - - - - - - // -------------------------------------- //
  Serial.begin(COM);                                             // initialize COM Port
  pu.pinsetup();
  EEPROM.begin(EERPOM_SIZE);

  // - - - - - I M U   S E T T I N G - - - - - - - - - - - - - - // -------------------------------------- //
  data.init_imu(SAMPLRT_DIV, DLPF_CONF);                         // initialize MPU6050 device
  data.butterworth(CUTOFF_FREQ, SAMPLE_TIME);                    // initialize LPF
  // data.getData();                                                // get the IMU initialize data    
  // data.setZero();                                                // set the IMU data to zero
  // delay(100);

  // - - - - - B L E   S E T T I N G - - - - - - - - - - - - - - //
  BLEDevice::init(DeviceName);                                   // Create the BLE Device
  pServer = BLEDevice::createServer();                           // Create the BLE Server
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE);        // Create the BLE Service

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
										CHAR_TX,
										BLECharacteristic::PROPERTY_NOTIFY
									);
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
											 CHAR_RX,
											BLECharacteristic::PROPERTY_WRITE
										);

  pRxCharacteristic-> setCallbacks(new MyCallbacks());
  pService         -> start(); 

  // Start advertising
  pServer->getAdvertising()->start();

  data.setZero();  
  data.setZero();  
  
  delay(100);

  // - - - - - T I M E R  S T A R T - - - - - - - - - - - - - -  //
  timer = micros();                                              // initialize timer
}

// int ble_count = 0;
// int cosole_c = 0;

void loop(){
  delta_t = micros() - timer;
  if(delta_t > SAMPLE_TIME){
    timer = micros();

    data.getData();
    ctrl.save2_Tx_Buffer();
    ctrl.check_balance();

    // Check Button
    ctrl.check_button();
    if(btn_count>10){
      data.setZero();
    }

    // Read Battery Voltage
    ctrl.read_bt_volt();
    ctrl.read_bt_charge();

    // Console output
    if(tx_buff_fg && !deviceConnected){
      ctrl.console_log();
      tx_buff_fg = false;
    }

    // Device connected
    if(deviceConnected){
      // Transmit BLE Data
      if(tx_buff_fg){
      //   pTxCharacteristic->setValue(txArray, 41);               //
      //   pTxCharacteristic->notify();
        ctrl.console_log();
        tx_buff_fg = false;
      }

      // Receive BLE Data
      if(rxLen>=2){

        // test code
        // Serial.printf("BLE data %d received\n", rxLen);
        // for(int i=0; i<rxLen; i++){
        //   if(rxArray[i]< 0x0A){
        //     Serial.printf("0x0%x | ", rxArray[i]);
        //   }else{
        //     Serial.printf("0x%x | ", rxArray[i]);
        //   }
        // }

        uint8_t Hdr = rxArray[0];
        uint8_t Cmd = rxArray[1];

        if(Hdr == H1){                                           // 1st byte rx 0xA0
          if(Cmd == H11){                                        // Calibate tile x, tile z value to zero
            data.setZero();
          }else if(Cmd == H12){                                  
            tile_x_cal = 0;                                      // Reset tile x, tile z value to orginal reading
            tile_z_cal = 0;
          }else if(Cmd == H13){
            for(int i=0; i<4; i++){                              // Reset angle threshold to default angle
              angleLmt[i] = defualt_angle;
            }
          }
        }
        
        if(Hdr == H2){                                           // 1st byte Receive 0xA1
          if(Cmd == R){                                          // Read all threshold value
            txStatus[0] = H2;
            txStatus[1] = R;
            txStatus[2] = H25;
            txStatus[3] = angleLmt[0];                           // Tile XF (Front) threshold value
            // txStatus[4] = angleLmt[1];                        // Tile XF (Back) threshold value
            txStatus[4] = angleLmt[2];                           // Tile ZL (Left) threshold value
            txStatus[5] = angleLmt[3];                           // Tile ZR (Right) threshold value
            
            txLen = 6;                                           // 
            
            if(rxArray[2] == 0x00){
              Serial.printf("X: %d, ZL: %d, ZR: %d\n",           // Show all angle threshold on console
              angleLmt[0], angleLmt[2], angleLmt[3]);
            }
          }
          
          if(Cmd == W){
            if(rxArray[2] == H21){                               // Set Front pos angle threshold
              angleLmt[0] = rxArray[3]; 
            }else if(rxArray[2] == H22){                         // Set Back pos angle threshold
              angleLmt[1] = rxArray[3];
            }else if(rxArray[2] == H23){                         // Set Left pos angle threshold
              angleLmt[2] = rxArray[3];
            }else if(rxArray[2] == H24){                         // Set Right pos angle threshold
              angleLmt[3] = rxArray[3];
            }else if(rxArray[2] == H25){                         // Set All pos angle threshold
              angleLmt[0] = rxArray[3];
              angleLmt[2] = rxArray[4];
              angleLmt[3] = rxArray[5];
            }
          }
        }

        if(Hdr == H3){
          if(Cmd == R){
            Serial.printf("The battery voltage is: 4.2V\n");
          }
        }

        if(Hdr == H4){
          if(Cmd == R){
            Serial.printf("The battery charging\n");
          }
        }
          
        if(Hdr == H5){
          if(Cmd == R){
            Serial.printf("Firmware ver: %s\n", FirmwareVer);
          }else if(Cmd == W){

          }
        }

        if(Hdr == H6){
          uint8_t data_len = rxArray[2];
          if(Cmd == R){
            for(int i=0; i<data_len+1; i++){
              Serial.printf("%c ", EEPROM.read(i));
            }
            Serial.printf("\n");
            delay(5000);

          }else if(Cmd == W){
            if(data_len != 0xFF){                                // if data len != 0xFF, write data from BLE to EERPOM
              for(int i=0; i<data_len+1; i++){
                EEPROM.write(i, rxArray[i+2]);
                EEPROM.commit();
              }
            }else if(data_len == 0xFF){
              for(int i=0; i<EERPOM_SIZE; i++){                  // if data len == 0x00, clear EEPROM data
                EEPROM.write(i, 0x00);
                EEPROM.commit();
              }
            }else if(data_len == 0xF0){                          // testing code
              uint8_t d = 0x41;
              for(int i=0; i<EERPOM_SIZE+1; i++){
                EEPROM.write(i, d);
                EEPROM.commit();
                delay(1);
                d++;
              }
            }
          }
        }

        pTxCharacteristic->setValue(txStatus, txLen);               //
        pTxCharacteristic->notify();

        ctrl.clear_Tx_Para_Buff();
        ctrl.clear_Rx_Buffer();
      }


      // if(rxLen>0){
      //   if(rxArray[0] == RX_H1){                                // Set tile x, tile z value to zero
      //     data.setZero();
      //   }else if(rxArray[0] == RX_H2){                          // Calibate Position Limit
      //     if(rxArray[1] != 0x00){                               // For change one position only
      //       uint8_t mask  = 0xC0;
      //       for(int i=0; i<4; i++){
      //         if(rxArray[1] == mask){
      //           angleLmt[i] = rxArray[2];
      //         }
      //         mask = mask >> 2;
      //       }
      //     }else{                                                // For change more than one position
      //       for(int i=0; i< rxLen -2; i++){
      //         angleLmt[i]= rxArray[i + 2];
      //       }
      //     }
      //   }else if(rxArray[0] == RX_H3){                          // Reset tile x, tile z value
      //     tile_x_cal = 0;
      //     tile_z_cal = 0;
      //   }
      //   ctrl.clear_Rx_Buffer();
      // }
    }
    
    // Device disconnected
    if (!deviceConnected && oldDeviceConnected) {
      pServer->startAdvertising();                             // restart advertising
    }

    // Device just connected
    if (deviceConnected && !oldDeviceConnected) {
      oldDeviceConnected = deviceConnected;
      pServer -> getAdvertising() -> stop();
    }
  }
}