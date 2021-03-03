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

#include "vzdisk.h"

uint8_t vtech1_fdc_latch = 0;

#define PARKED_TRACK 0   //80
uint8_t vtech1_track_x2 = PARKED_TRACK;

//Mega 2560 only, stepmotor phase interrupt register
#define PIN_STEP_REG    PORT_STEP  // interrupt 0 is on AVR pin PD2

//Used to calculate latch PHI
#define PHI0(n) (((n)>>PIN_STEP0_BIT)&1)
#define PHI1(n) (((n)>>PIN_STEP1_BIT)&1)
#define PHI2(n) (((n)>>PIN_STEP2_BIT)&1)
#define PHI3(n) (((n)>>PIN_STEP3_BIT)&1)

#define STEP3 (1<<PIN_STEP3_BIT)
#define STEP2 (1<<PIN_STEP2_BIT)
#define STEP1 (1<<PIN_STEP1_BIT)
#define STEP0 (1<<PIN_STEP0_BIT)

void handle_steps() {
  if (!drv_enabled)
    return;

  uint8_t data = PIN_STEP_REG & 0x0F;

  //Track--, if current bit is shifted to right by 1, wrapped by 4 bits
  if (data == STEP0 && PHI1(vtech1_fdc_latch) ||
      data == STEP1 && PHI2(vtech1_fdc_latch) ||
      data == STEP2 && PHI3(vtech1_fdc_latch) ||
      data == STEP3 && PHI0(vtech1_fdc_latch))
  /*if ( (PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI1(vtech1_fdc_latch)) ||
       (PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI2(vtech1_fdc_latch)) ||
       (PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI3(vtech1_fdc_latch)) ||
       (PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI0(vtech1_fdc_latch)) )*/
  {
    //serial_log(PSTR("Trk-- : %d%d%d%d Latch: %d%d%d%d\r\n"),PHI0(data),PHI1(data),PHI2(data),PHI3(data),    PHI0(vtech1_fdc_latch), PHI1(vtech1_fdc_latch), PHI2(vtech1_fdc_latch), PHI3(vtech1_fdc_latch));
    if (vtech1_track_x2 > 0)
      vtech1_track_x2--;
  }
  //Track++, if current bit is shifted to left by 1, wrapped by 4 bits
  else if (
    data == STEP0 && PHI3(vtech1_fdc_latch) ||
    data == STEP1 && PHI0(vtech1_fdc_latch) ||
    data == STEP2 && PHI1(vtech1_fdc_latch) ||
    data == STEP3 && PHI2(vtech1_fdc_latch))
  /*else if ( (PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI3(vtech1_fdc_latch)) ||
            (PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI0(vtech1_fdc_latch)) ||
            (PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI1(vtech1_fdc_latch)) ||
            (PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI2(vtech1_fdc_latch)) )*/    
  {
    //serial_log(PSTR("Trk++ : %d%d%d%d Latch: %d%d%d%d\r\n"),PHI0(data),PHI1(data),PHI2(data),PHI3(data),    PHI0(vtech1_fdc_latch), PHI1(vtech1_fdc_latch), PHI2(vtech1_fdc_latch), PHI3(vtech1_fdc_latch));
    if ( vtech1_track_x2 < 2 * TRK_NUM )
      vtech1_track_x2++;
  }
  
  vtech1_fdc_latch = data;
  //serial_log(PSTR("TRK: %d\r\n"), vtech1_track_x2);  
}
