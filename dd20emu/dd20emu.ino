
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
#include "dd20emu_acl.h"
#include "vzdisk.h"

/*
 * Laser 310 I/O port
   Port 10h     Latch(write-only)
        11h     DATA(read-only)
        12h     Pooling(read-only)
        13h     Write protection status(read-only)
*/

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

  serial_log(PSTR("\r\nVTech DD20 emulator, v0.0.8, 02/20/2021\r\n"));

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
