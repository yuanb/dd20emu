
/*
   DD-20 emulator
   Author: Bill Yow
   https://github.com/yuanb/dd20emu

*/

#include <SPI.h>
#include "SdFat.h"
#include "vzdisk.h"

/*
   Pin definitions, Arduino Mega2560
   GND
   13,  PB7,    RD Data
   2,   PE4,    /WRReq
   3,   PE5,    /EnDrv

   18,  PD3,    Step3
   19,  PD2,    Step2
   20,  PD1,    Step1
   21,  PD0,    Step0
*/

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

const byte rdDataPin = 13;

const byte enDrvPin     = 3;
const byte wrReqPin     = 2;

const byte stepPin0 = 21;
const byte stepPin1 = 20;
const byte stepPin2 = 19;
const byte stepPin3 = 18;

extern bool drv_enabled;
extern bool write_request;

/*****************/
/*  SD FILE      */
/*****************/

//Disk image format 1, FLOPPY1.DSK and FLOPPY2.DSK
//Penguin wont load, D1B and VZCAVE wont run. The rest are ok
char filename[] = "FLOPPY1.DSK";

//Disk image format 2 (formatted from vzemu), fsize = 99185
//char filename[] = "HELLO.DSK";

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

  serial_log(PSTR("\r\n\r\nVTech DD20 emulator, v0.0.4, 11/6/2020\r\n"));

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
