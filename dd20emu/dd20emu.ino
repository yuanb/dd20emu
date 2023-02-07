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
 * 1. Write support
 * 2. Save configurations to EEPROM
 */
#define __ASSERT_USE_STDERR
#include <assert.h>

#include <SPI.h>
#include "SdFat.h"
#include "PinChangeInterrupt.h"
#include "dd20emu_acl.h"
#include "vzdisk.h"

#define FILENAME_MAX  80

/*
 * Laser 310 I/O port
   Port 10h     Latch(write-only)
        11h     DATA(read-only)
        12h     Pooling(read-only)
        13h     Write protection status(read-only)
*/

char diskimage[FILENAME_MAX] = "HELLO.DSK";
vzdisk *vzdsk = NULL;
  
void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  serial_log(PSTR("Begin DD-20 emulation\r\n"));
  print_status();

  // put your setup code here, to run once:
  // set the digital pin as output:
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(wrProtPin, OUTPUT);
  digitalWrite(wrProtPin, HIGH);

  pinMode(enDrvPin, INPUT_PULLUP);
  pinMode(rdDataPin, OUTPUT);
  pinMode(wrReqPin, INPUT_PULLUP);
  pinMode(wrDataPin,INPUT_PULLUP);

  pinMode(stepPin0, INPUT_PULLUP);
  pinMode(stepPin1, INPUT_PULLUP);
  pinMode(stepPin2, INPUT_PULLUP);
  pinMode(stepPin3, INPUT_PULLUP);

  vzdsk = new vzdisk();
  vzdsk->Open(diskimage); 

  attachInterrupt(digitalPinToInterrupt(wrReqPin), writeRequest, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enDrvPin), driveEnabled, CHANGE);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(stepPin0), handle_steps, CHANGE);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(stepPin1), handle_steps, CHANGE);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(stepPin2), handle_steps, CHANGE);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(stepPin3), handle_steps, CHANGE); 

  print_enter_msg();
}

void loop() {
  handle_datastream();

  int i = Serial.read();
  if (i==0x0d || i==0x0a) {
    handle_shell();
  }
}
