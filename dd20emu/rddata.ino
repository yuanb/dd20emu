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

//Tested values

//FM encoded 0 : ___n__________n___ : 1.125us + 31.75us
//FM encoded 1 : ___n___n______n___ : 1.125us + 11 us + 1.125us + 20 us


#define delay_1us   __builtin_avr_delay_cycles (16)
#define delay_11us  __builtin_avr_delay_cycles (170)
#define delay_20us  __builtin_avr_delay_cycles (300)
#define delay_31_2us  __builtin_avr_delay_cycles (488)

#define RD_DATA_MASK   (1<<RD_DATA_BIT)
#define RD_HIGH   PORT_RDDATA |= RD_DATA_MASK;
#define RD_LOW    PORT_RDDATA &= ~RD_DATA_MASK;


#define FM_BIT_1  delay_11us; RD_HIGH;  delay_1us;  RD_LOW; delay_20us;
#define FM_BIT_0  delay_31_2us;

#define FM_ENCODE_BIT(v,m) {  if (v & m) { FM_BIT_1; } else { FM_BIT_0; } }

#define FM_OUTPUT_BIT(v,m) { RD_HIGH; delay_1us; RD_LOW;  FM_ENCODE_BIT(v,m); }

/*
 * Soft SPI, Most Significant Bit (MSB) first
 */
inline void put_byte(byte v)
{
  FM_OUTPUT_BIT(v,0b10000000);
  FM_OUTPUT_BIT(v,0b01000000);
  FM_OUTPUT_BIT(v,0b00100000);
  FM_OUTPUT_BIT(v,0b00010000);
  FM_OUTPUT_BIT(v,0b00001000);
  FM_OUTPUT_BIT(v,0b00000100);
  FM_OUTPUT_BIT(v,0b00000010);
  FM_OUTPUT_BIT(v,0b00000001);
}

uint8_t current_sector = 0;
void handle_wr() {
  if (drv_enabled && !write_request) {
    put_sector((uint8_t)vtech1_track_x2/2, current_sector);
    if (++current_sector >= SEC_NUM)
      current_sector = 0;      
  }
}

extern uint8_t fdc_sector[SECSIZE_VZ];
extern vzdisk* vzdsk;
inline void put_sector(uint8_t n, uint8_t s)
{
  if (vzdsk->get_sector(n, s) != -1)
  {
    //In theory, this should speed up disk read a bit
    //Track has changed while reading sector from SD, skip playing back
    if (n != vtech1_track_x2/2)
      return;

    for(int j=0; j < SECSIZE_VZ; j++)
    {
      byte v = (byte *)fdc_sector[j];
      put_byte(v);
    }
  }
}
