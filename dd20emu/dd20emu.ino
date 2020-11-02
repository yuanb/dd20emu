
/*
   DD-20 emulator
   Author: Bill Yow

*/

#include <SPI.h>
#include <SD.h>
/*
 * /Users/billyuan/Library/Arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7/bin/avr-objdump -S dd20emu.ino.elf > /Users/billyuan/Documents/Arduino/dd20emu/dd20emu.lst
 * 
 */

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
   Port 10h     Latch(write-only)
        11h     DATA(read-only)
        12h     Pooling(read-only)
        13h     Write protection status(read-only)
*/

#define TRK_NUM     40
#define SEC_NUM     16
#define SECSIZE_VZ  154
#define TRKSIZE_VZ  SECSIZE_VZ * SEC_NUM    //2464
#define TRKSIZE_VZ_PADDED TRKSIZE_VZ + 16

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

//Disk image format 1, FLOPPY1.DSK and FLOPPY2.DSK
//Penguin wont load, D1B and VZCAVE wont run. The rest are ok
//char filename[] = "FLOPPY1.DSK";

//Disk image format 2 (formatted from vzemu), fsize = 99185
char filename[] = "HELLO.DSK";

//Disk image format 2(created from empty file from vzemu)
//char filename[] = "20201016.DSK";

//Disk image format 2?
//char filename[] ="extbasic.dsk";

File f;

extern bool drv_enabled;
extern bool write_request;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  serial_log(PSTR("\r\n\r\nVTech DD20 emulator, v0.0.3, 11/1/2020\r\n"));

  if (!SD.begin())
  {
    serial_log(PSTR("Failed to begin on SD"));
  }

  if (!SD.exists(filename))
  {
    serial_log(PSTR("Can't find FLOPPY1.DSK"));
  }

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

  f = SD.open(filename, FILE_READ);
  if (f == false)
  {
    serial_log(PSTR("DSK File is not opened"));
    return -1;
  }

  set_track_padding(f);
  build_sector_lut(f);

  attachInterrupt(digitalPinToInterrupt(wrReqPin), writeRequest, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enDrvPin), driveEnabled, CHANGE);
  attachInterrupt(digitalPinToInterrupt(stepPin0), handle_steps, CHANGE);
  attachInterrupt(digitalPinToInterrupt(stepPin1), handle_steps, CHANGE);
  attachInterrupt(digitalPinToInterrupt(stepPin2), handle_steps, CHANGE);
  attachInterrupt(digitalPinToInterrupt(stepPin3), handle_steps, CHANGE);

  serial_log(PSTR("Begin DD-20 emulation"));
}

void loop() {
  handle_wr();
}
