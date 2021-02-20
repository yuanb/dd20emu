
/*
   DD-20 emulator
   Author: Bill Yow
   https://github.com/yuanb/dd20emu

*/

#include <SPI.h>
#include "SdFat.h"
#include "vzdisk.h"

/*
 * Pin definitions, Arduino Uno
 * GND
 * 7,  PD7,    RD Data
 * A5, PC5,    /WRReq
 * A4, PC4,    /EnDrv
 * A3, PC3,    Step3
 * A2, PC2,    Step2
 * A1, PC1,    Step1
 * A0, PC0,    Step0
 * 
 * Arduino Uno pinout 
 * https://upload.wikimedia.org/wikipedia/commons/c/c9/Pinout_of_ARDUINO_Board_and_ATMega328PU.svg
 * 
 * Ethernet shield datasheet
 * https://www.mouser.com/catalog/specsheets/a000056_datasheet.pdf
 * Ethernet shield schametic , 
 * Arduino Uno Pin 4 is used as SD-CS, can not be used as GPIO
 * Arduino Uno Pin 10 is used as W5100 CS, can not be used as GPIO
 * 11,12,13 on ICSP header are for SPI
 * https://www.arduino.cc/en/uploads/Main/arduino-ethernet-shield-05-schematic.pdf
 * 
*/

/*
   Pin definitions, Arduino Mega2560
   GND
   4 ,  PG5,    RD Data
   3,   PE5,    /EnDrv
   2,   PE4,    /WRReq
   18,  PD3,    Step3
   19,  PD2,    Step2
   20,  PD1,    Step1
   21,  PD0,    Step0  
*/
#define PORT_RDDATA   PORTG   //rddata.ino
#define RD_DATA_BIT 5

#define PORT_CTL      PINE    //isr.ino
#define PIN_EN_BIT      5
#define PIN_WRREQ_BIT   4

#define PORT_STEP     PIND    //isr_steps.ino
#define PIN_STEP3_BIT   3
#define PIN_STEP2_BIT   2
#define PIN_STEP1_BIT   1
#define PIN_STEP0_BIT   0

//pinMode(), use pin name
const byte rdDataPin = 4;
const byte enDrvPin  = 3;
const byte wrReqPin  = 2;
const byte stepPin3  = 18;
const byte stepPin2  = 19;
const byte stepPin1  = 20;
const byte stepPin0  = 21;

/*
 * Laser 310 I/O port
   Port 10h     Latch(write-only)
        11h     DATA(read-only)
        12h     Pooling(read-only)
        13h     Write protection status(read-only)
*/


//Emulator variables
const int ledPin =  LED_BUILTIN;// the number of the LED pin Arduino
//const int ledPin = 2;         //Ethernet shield
uint8_t ledState = LOW;         // ledState used to set the LED

extern bool drv_enabled;
extern bool write_request;

/*****************/
/*  SD FILE      */
/*****************/

//Disk image format 1, FLOPPY1.DSK and FLOPPY2.DSK
//Penguin wont load, D1B and VZCAVE wont run. The rest are ok
//char filename[] = "FLOPPY1.DSK";

//Disk image format 2 (formatted from vzemu), fsize = 99185
char filename[] = "HELLO.DSK";

//Disk image format 2(created from empty file from vzemu)
//char filename[] = "20201016.DSK";

//Disk image format 2?
//char filename[] ="extbasic.dsk";

vzdisk *vzdsk = NULL;
  
void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  serial_log(PSTR("\r\n\r\nVTech DD20 emulator, v0.0.5, 12/05/2020\r\n"));

  // put your setup code here, to run once:
  // set the digital pin as output:
  pinMode(ledPin, OUTPUT);

  pinMode(enDrvPin, INPUT_PULLUP);
  pinMode(rdDataPin, OUTPUT);
  pinMode(wrReqPin, INPUT_PULLUP);

  pinMode(stepPin0, INPUT_PULLUP);
  pinMode(stepPin1, INPUT_PULLUP);
  pinMode(stepPin2, INPUT_PULLUP);
  pinMode(stepPin3, INPUT_PULLUP);

  vzdsk = new vzdisk();
  vzdsk->Open(filename);
  vzdsk->set_track_padding();  
  vzdsk->build_sector_lut();

  attachInterrupt(digitalPinToInterrupt(wrReqPin), writeRequest, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enDrvPin), driveEnabled, CHANGE);
  attachInterrupt(digitalPinToInterrupt(stepPin0), handle_steps, CHANGE);
  attachInterrupt(digitalPinToInterrupt(stepPin1), handle_steps, CHANGE);
  attachInterrupt(digitalPinToInterrupt(stepPin2), handle_steps, CHANGE);
  attachInterrupt(digitalPinToInterrupt(stepPin3), handle_steps, CHANGE);

  serial_log(PSTR("Begin DD-20 emulation, Free memory: %d bytes"), freeMemory());
}

void loop() {
  handle_wr();
}
