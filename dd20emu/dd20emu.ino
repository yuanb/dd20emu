
/*
   DD-20 emulator
   Author: Bill Yow
   https://github.com/yuanb/dd20emu

*/

/* Libraries used:
 *  SDFat : Author : Bill Greiman, Tested: 2.0.4, https://github.com/greiman/SdFat
 *  PinChangeInterrupt : Tested 1.2.8, https://github.com/NicoHood/PinChangeInterrupt
 */

#include <SPI.h>
#include "SdFat.h"
#include "PinChangeInterrupt.h"
#include "vzdisk.h"


//#define DD20_UNO
#define DD20_MEGA2560

#ifdef DD20_UNO
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
#else //DD20_MEGA2560
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

/* Port and bit for RDDATA */
#define PORT_RDDATA   PORTG   //rddata.ino
#define RD_DATA_BIT 5
const byte rdDataPin = 4;

/* Port and bits for /EnDrv and /WRReq */
#define PORT_CTL      PINE    //isr.ino
#define PIN_EN_BIT      5
#define PIN_WRREQ_BIT   4
const byte enDrvPin  = 3;
const byte wrReqPin  = 2;

/* Port and bits for Step 0 ~ Step 3 */
#define PORT_STEP     PINK    //isr_steps.ino
#define PIN_STEP3_BIT   3
#define PIN_STEP2_BIT   2
#define PIN_STEP1_BIT   1
#define PIN_STEP0_BIT   0
const byte stepPin3  = A8;
const byte stepPin2  = A9;
const byte stepPin1  = A10;
const byte stepPin0  = A11;
#endif

/*
 * Laser 310 I/O port
   Port 10h     Latch(write-only)
        11h     DATA(read-only)
        12h     Pooling(read-only)
        13h     Write protection status(read-only)
*/


//Emulator variables, active high: TRUE
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

  serial_log(PSTR("\r\n\r\nVTech DD20 emulator, v0.0.6, 02/19/2021\r\n"));

  // put your setup code here, to run once:
  // set the digital pin as output:
  pinMode(LED_BUILTIN, OUTPUT);

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
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(stepPin0), handle_steps, CHANGE);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(stepPin1), handle_steps, CHANGE);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(stepPin2), handle_steps, CHANGE);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(stepPin3), handle_steps, CHANGE); 

  serial_log(PSTR("Begin DD-20 emulation, Free memory: %d bytes\r\n"), freeMemory());
}

void loop() {
  handle_wr();
}
