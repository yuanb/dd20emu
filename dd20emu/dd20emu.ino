/*
    DD-20 emulator
    Copyright (C) 2020-2023 https://github.com/yuanb/dd20emu

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/* Libraries used:
 *  SDFat : Author : Bill Greiman, Tested: 2.2.0, https://github.com/greiman/SdFat
 *  PinChangeInterrupt : Tested 1.2.9, https://github.com/NicoHood/PinChangeInterrupt
 *
 *  Libraries can be installed from Arduino IDE menu Tools\Manage Libraries
 */

/*
 * TODO: 
 * 1. CLI interface
 *        https://www.norwegiancreations.com/2018/02/creating-a-command-line-interface-in-arduinos-serial-monitor/
 *        
 * 2. Write support
 *        
 * 3. Write Protect
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
//TODO : "GHOST2" Disk I/O Error, Invalid IDAM on TR 39
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

  serial_log(PSTR("\r\nVTech DD20 emulator, v0.0.9, 01/22/2023\r\n"));
  serial_log(PSTR("\r\nSector size: %d bytes"), sizeof(sector_t));
  serial_log(PSTR("\r\nSector header size: %d bytes\r\n"), sizeof(sec_hdr_t));

  // put your setup code here, to run once:
  // set the digital pin as output:
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(enDrvPin, INPUT_PULLUP);
  pinMode(rdDataPin, OUTPUT);
  pinMode(wrReqPin, INPUT_PULLUP);
  pinMode(wrDataPin,INPUT_PULLUP);

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
  print_enter_msg();
}

void loop() {
  handle_datastream();

  int i = Serial.read();
  if (i==0x0d || i==0x0a) {
    handle_shell();
  }
}
