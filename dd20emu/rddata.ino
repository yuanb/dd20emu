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


#define delay_1us   __builtin_avr_delay_cycles (15)
#define delay_11us  __builtin_avr_delay_cycles (170)
#define delay_20us  __builtin_avr_delay_cycles (294)
#define delay_31_2us  __builtin_avr_delay_cycles (482)
#define pulse_1us   RD_HIGH; delay_1us; RD_LOW

#define RD_DATA_MASK   (1<<RD_DATA_BIT)
#define RD_HIGH   PORT_RDDATA |= RD_DATA_MASK;
#define RD_LOW    PORT_RDDATA &= ~RD_DATA_MASK;

#define WR_DATA_MASK   (1<<WR_DATA_BIT)
#define WR_HIGH    PORT_WRDATA & WR_DATA_BIT


#define FM_BIT_1  delay_11us; pulse_1us; delay_20us;
#define FM_BIT_0  delay_31_2us;

#define FM_ENCODE_BIT(v,m) {  if (v & m) { FM_BIT_1; } else { FM_BIT_0; } }

#define FM_OUTPUT_BIT(v,m) { pulse_1us; FM_ENCODE_BIT(v,m); }

uint8_t bitmask[8] = {
  0b10000000,
  0b01000000,
  0b00100000,
  0b00010000,
  0b00001000,
  0b00000100,
  0b00000010,
  0b00000001
};

/*
 * Soft SPI, Most Significant Bit (MSB) first
 */
inline uint8_t put_byte(uint8_t v)
{
  uint8_t i;
  for(i=0; i<8 & !write_request; i++) {
    FM_OUTPUT_BIT(v, bitmask[i]);
  }

  return i;
}

void handle_datastream() {
  static uint8_t current_sector = 0;
  static bool led = true;
  
  if (drv_enabled && !write_request) {

    digitalWrite(LED_BUILTIN, led);
    led = !led;
    
    put_sector((uint8_t)vtech1_track_x2/2, current_sector);
    if (++current_sector >= SEC_NUM)
      current_sector = 0;      
  }
}

extern uint8_t fdc_sector[SECSIZE_VZ];
extern vzdisk* vzdsk;
inline void put_sector(uint8_t n, uint8_t s)
{
  if (vzdsk && vzdsk->get_sector(n, s))
  {
    //If current track has changed
    if (n != vtech1_track_x2/2)
      return;

    bool closing_bit = true;
    for(int j=0; j < SECSIZE_VZ; j++)
    {
      if (put_byte(fdc_sector[j]) != 8) {
        closing_bit = false;
        break;
      }
    }

    if (closing_bit) {
      //last byte of the sector, closing pulse
      pulse_1us;
    }
  }
}
