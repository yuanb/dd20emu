/*
    DD-20 emulator
    Copyright (C) 2020,2021 https://github.com/yuanb/

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

#ifndef _DD20EMU_ACL_H_
#define _DD20EMU_ACL_H_

#ifdef ARDUINO_AVR_UNO
/*
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
*/

// SD chip select pin.  Be sure to disable any other SPI devices such as Enet.
#define SD_CS_PIN 4

/*
 * Pin definitions, Arduino Uno
 * GND
 * 4,  PD4,    RD Data
 * A5, PC5,    /WRReq
 * A4, PC4,    /EnDrv
 * A3, PC3,    Step3
 * A2, PC2,    Step2
 * A1, PC1,    Step1
 * A0, PC0,    Step0
*/
/* Port and bit for RDDATA */
#define PORT_RDDATA   PORTD   //rddata.ino
#define RD_DATA_BIT 4
const byte rdDataPin = 4;

/* Port and bits for /EnDrv and /WRReq */
#define PORT_CTL      PIND    //isr.ino
#define PIN_EN_BIT      3
#define PIN_WRREQ_BIT   2
const byte enDrvPin  = 3;
const byte wrReqPin  = 2;

/* Port and bits for Step 0 ~ Step 3 */
#define PORT_STEP     PINC    //isr_steps.ino
#define PIN_STEP3_BIT   3
#define PIN_STEP2_BIT   2
#define PIN_STEP1_BIT   1
#define PIN_STEP0_BIT   0
const byte stepPin3  = A3;
const byte stepPin2  = A2;
const byte stepPin1  = A1;
const byte stepPin0  = A0;

#elif ARDUINO_AVR_MEGA2560

// SD chip select pin.  Be sure to disable any other SPI devices such as Enet.
#define SD_CS_PIN SS

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
const byte stepPin3  = A11;
const byte stepPin2  = A10;
const byte stepPin1  = A9;
const byte stepPin0  = A8;
#endif

#else
#error "Unsupported architecture"

#endif  //_DD20EMU_ACL_H_
