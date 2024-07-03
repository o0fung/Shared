#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
//please download these 2 libs mentioned above from arduino lib manager

#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>

#include <EEPROM.h>

/*
  06/01/2016
  Author: Makerbro
  Platforms: ESP8266
  Language: C++
  File: HelloOLED.ino
  ------------------------------------------------------------------------
  Description: 
  Demo for OLED display showcasing writing text to the screen.
  ------------------------------------------------------------------------
  Please consider buying products from ACROBOTIC to help fund future
  Open-Source projects like this! We'll always put our best effort in every
  project, and release all our design files and code for you to use. 
  https://acrobotic.com/
  ------------------------------------------------------------------------
  License:
  Released under the MIT license. Please check LICENSE.txt for more
  information.  All text above must be included in any redistribution. 
*/
#include <Wire.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

const int LbuttonPin = 8;
const int RbuttonPin = 6;
const int ResetbuttonPin = 9;
const int MidbuttonPin = 7;
const int RLED =  13;
const int GLED =  13;

int LRaddress=0x01;
int deviceAddress = 0x51;
int RecordAddress = 0x00;
int TargetAddress = 0x1B;
byte RecordValue;

void eeprom_i2c_write(byte address, byte from_addr, byte data) {
  Wire.beginTransmission(address);
  Wire.write(from_addr);
  Wire.write(data);
  Wire.endTransmission();
  
  delay(10);
}

byte eeprom_i2c_read(int address, int from_addr) {
  Wire.beginTransmission(address);
  Wire.write(from_addr);
  Wire.endTransmission();

  Wire.requestFrom(address, 1);
  if(Wire.available())
    return Wire.read();
  else
    return 0xFF;
}

void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
asm volatile ("  jmp 0");  
} 

void setup()
{
  Serial.begin(115200);

  Wire.begin();  
  
  
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);                     // Initialze SSD1306 OLED display
  delay(10); 
  display.display();
  delay(10); 
  display.clearDisplay();              // Clear screen
  display.setTextSize(1);
  display.setTextColor(WHITE);
  /*  
  oled.setTextXY(0,0);              // Set cursor position, start of line 0
  oled.putString("ACROBOTIC");
  oled.setTextXY(1,0);              // Set cursor position, start of line 1
  oled.putString("industries");
  oled.setTextXY(2,0);              // Set cursor position, start of line 2
  oled.putString("Pasadena,");
  oled.setTextXY(2,10);             // Set cursor position, line 2 10th character
  oled.putString("CA");
  */
  pinMode(LbuttonPin, INPUT);
  pinMode(RbuttonPin, INPUT);
  pinMode(ResetbuttonPin, INPUT);
  pinMode(MidbuttonPin, INPUT);
  
  //EEPROM.write(RecordAddress, 0x07);
  //EEPROM.write(RecordAddress+1, 0x29);
}



void loop()
{
  byte record1,record2,LR;
  byte error, address;
  Serial.println("Scanning...");
  Wire.beginTransmission(0x50);
  error = Wire.endTransmission(); 
  if (error == 0)
  {
        display.setCursor(2,0);              // Set cursor position, start of line 0
        display.println("Found a Brace");

        LR=eeprom_i2c_read(0x50, 0);
        record1 = EEPROM.read(RecordAddress);
        record2 = EEPROM.read(RecordAddress+1);

        record2++;
        if(record2==0)
          record1++;

        EEPROM.write(RecordAddress, record1);
        delay(10);      
         EEPROM.write(RecordAddress+1, record2);
        delay(10);  
        //EEPROM.commit();
        char dataString[50] = {0};
        
        display.setCursor(2,8);              // Set cursor position, start of line 0
        if(LR==1)
          display.println("Left Brace");
        else
          display.println("Right Brace");
          
        display.setCursor(2,16);              // Set cursor position, start of line 0
        sprintf(dataString,"ID: 0x%02x%02x",record1,record2);
        display.println(dataString);
        display.display();
        ///////////////////////////////////////////////
        eeprom_i2c_write(0x50, TargetAddress, record1);
        Serial.println(eeprom_i2c_read(0x50, TargetAddress));
        eeprom_i2c_write(0x50, TargetAddress+1, record2);
        Serial.println(eeprom_i2c_read(0x50, TargetAddress+1));
        /////////////////////////////////////////////////

        while(1)
        {
            int LbuttonState = 1;
            int RbuttonState = 1;
            int ResetbuttonState = 1;

            
            LbuttonState = digitalRead(LbuttonPin);
            RbuttonState = digitalRead(RbuttonPin);
            ResetbuttonState = digitalRead(ResetbuttonPin);
            delay(0);
            sprintf(dataString,"ID: 0x%02x%02x",record1,record2);
            display.setCursor(2,8);   
            if (LbuttonState == LOW) {
              eeprom_i2c_write(0x50, 0, 1);
              display.clearDisplay();
              display.println("Left Brace Proged");
              display.println(dataString);
              display.display();
            }
            else if (RbuttonState == LOW) {
              eeprom_i2c_write(0x50, 0, 2);
              display.clearDisplay();
              display.println("Right Brace Proged");
              
              display.println(dataString);
              display.display();
            }

           int x = analogRead(A6);
           int y = analogRead(A7);
              analogWrite(10, x);
               analogWrite(11, y);
             
        }
  }
  else
  {
        
        display.setCursor(2,0);              // Set cursor position, start of line 0
        display.println("Looking for a brace");
        display.display();
        delay(1000);  
        display.clearDisplay();              // Clear screen        
        delay(400); 
         
  }
   

}
