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

//Tested values
//FM encoded 0 : ___n__________n___ : 1us + 31.2us + 1us (next bit)
//FM encoded 1 : ___n___n______n___ : 1us + 11 us + 1us + 19.2 us + 1us (next bit)

#include "utilities.h"

#define _delayNanoseconds(__ns)     __builtin_avr_delay_cycles( (double)(F_CPU)*((double)__ns)/1.0e9 /*+ 0.5 */)

#define delay_1us     _delayNanoseconds(1000)
#define delay_11us    _delayNanoseconds(11000)
#define delay_19_2us  _delayNanoseconds(19200)
#define delay_31_2us  _delayNanoseconds(31200)
#define pulse_1us   RD_HIGH; delay_1us; RD_LOW

#define RD_DATA_MASK   (1<<RD_DATA_BIT)
#define RD_HIGH   PORT_RDDATA |= RD_DATA_MASK;
#define RD_LOW    PORT_RDDATA &= ~RD_DATA_MASK;

#define FM_BIT_1  delay_11us; pulse_1us; delay_19_2us;
#define FM_BIT_0  delay_31_2us;

#define FM_ENCODE_BIT(v,m) {  if (v & m) { FM_BIT_1; } else { FM_BIT_0; } }
#define FM_OUTPUT_BIT(v,m) { pulse_1us; FM_ENCODE_BIT(v,m); }

extern uint8_t fdc_sector[SECSIZE_VZ];
extern vzdisk* vzdsk;

const uint8_t bitmask[8] = {
  0b10000000,
  0b01000000,
  0b00100000,
  0b00010000,
  0b00001000,
  0b00000100,
  0b00000010,
  0b00000001
};

uint8_t current_sector = 0;
extern uint16_t buf_idx;
extern uint16_t wr_buf16[];

bool handle_datastream() {
  static bool led = true;

  if (!drv_enabled || write_request || !vzdsk)
    return false;

  uint8_t tr = vtech1_track_x2/2;

#ifdef  PULSETIME
  //if wrbuf is not empty
  if (buf_idx !=0) {
    serial_log(PSTR("WRDATA-TR-%d-SEC-%d-LEN-%d\r\n"), tr, current_sector, buf_idx);
    print_buf16(wr_buf16, buf_idx, true);
    buf_idx =0;
  }
#endif
  
  if (!vzdsk->get_sector(tr, current_sector)) {
    serial_log(PSTR("failed on reading sector TR: %d, :SEC: %d"), tr, current_sector);
    return false;
  }
    
  digitalWrite(LED_BUILTIN, led);
  led = !led;

  for(uint8_t i=0; i< SECSIZE_VZ; i++) {

    for(uint8_t j=0; j<8; j++) {
      if (write_request) {
        return false;
      }
      FM_OUTPUT_BIT(fdc_sector[i], bitmask[j]);


//      TODO: Faster code to stop RDDATA output when /wrreq
//      if (fdc_sector[i] & bitmask[j]) {
//        //1
//        pulse_1us;
//        //delay_11us;
//        for(uint8_t k=0; k<7; k++) {
//          if (write_request) {
//            //tag /WR request
//            digitalWrite(9, HIGH);
//            digitalWrite(9,LOW);
//            return false;
//          }      
//          delay_1us;
//        }
//        
//        pulse_1us;
//        
//        //delay_20us;
//        for(uint8_t k=0; k<13; k++) {
//          if (write_request) {
//            //tag /WR request
//            digitalWrite(9, HIGH);
//            digitalWrite(9,LOW);
//            return false;
//          }      
//          delay_1us;
//        }
//      }
//      else {
//        //0
//        pulse_1us;
//        for(uint8_t k=0; k<20; k++) {
//          if (write_request) {
//            //tag /WR request
//            digitalWrite(9, HIGH);
//            digitalWrite(9,LOW);
//            return false;
//          }
//          delay_1us;
//        }
//      }
    }
  }
  if (++current_sector >= SEC_NUM)
    current_sector = 0;

  return true;
}
