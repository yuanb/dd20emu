#include "vzdisk.h"

uint8_t vtech1_fdc_latch = 0;

#define PARKED_TRACK 0   //80
uint8_t vtech1_track_x2 = PARKED_TRACK;

//Mega 2560 only, stepmotor phase interrupt register
#define PIN_STEP_REG    PORT_STEP  // interrupt 0 is on AVR pin PD2

#define PHI0(n) (((n)>>PIN_STEP0_BIT)&1)
#define PHI1(n) (((n)>>PIN_STEP1_BIT)&1)
#define PHI2(n) (((n)>>PIN_STEP2_BIT)&1)
#define PHI3(n) (((n)>>PIN_STEP3_BIT)&1)

void handle_steps() {
  if (!drv_enabled)
    return;

  uint8_t data = PIN_STEP_REG & 0x0F;

  //Track--, if current bit is shifted to right by 1, wrapped by 4 bits
  if (data == 0x01 && PHI1(vtech1_fdc_latch) ||
      data == 0x02 && PHI2(vtech1_fdc_latch) ||
      data == 0x04 && PHI3(vtech1_fdc_latch) ||
      data == 0x08 && PHI0(vtech1_fdc_latch))
  /*if ( (PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI1(vtech1_fdc_latch)) ||
       (PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI2(vtech1_fdc_latch)) ||
       (PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI3(vtech1_fdc_latch)) ||
       (PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI0(vtech1_fdc_latch)) )*/
  {
    //serial_log(PSTR("Trk-- : %d%d%d%d Latch: %d%d%d%d"),PHI0(data),PHI1(data),PHI2(data),PHI3(data),    PHI0(vtech1_fdc_latch), PHI1(vtech1_fdc_latch), PHI2(vtech1_fdc_latch), PHI3(vtech1_fdc_latch));
    if (vtech1_track_x2 > 0)
      vtech1_track_x2--;
  }
  //Track++, if current bit is shifted to left by 1, wrapped by 4 bits
  else if (
    data == 0x01 && PHI3(vtech1_fdc_latch) ||
    data == 0x02 && PHI0(vtech1_fdc_latch) ||
    data == 0x04 && PHI1(vtech1_fdc_latch) ||
    data == 0x08 && PHI2(vtech1_fdc_latch))
  /*else if ( (PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI3(vtech1_fdc_latch)) ||
            (PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI0(vtech1_fdc_latch)) ||
            (PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI1(vtech1_fdc_latch)) ||
            (PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI2(vtech1_fdc_latch)) )*/    
  {
    //serial_log(PSTR("Trk++ : %d%d%d%d Latch: %d%d%d%d"),PHI0(data),PHI1(data),PHI2(data),PHI3(data),    PHI0(vtech1_fdc_latch), PHI1(vtech1_fdc_latch), PHI2(vtech1_fdc_latch), PHI3(vtech1_fdc_latch));
    if ( vtech1_track_x2 < 2 * TRK_NUM )
      vtech1_track_x2++;
  }
  vtech1_fdc_latch = data;
}
