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

#define PIN_EN_REG      PORT_CTL
#define PIN_WR_REG      PORT_CTL
#define PIN_EN_MASK     (1<<PIN_EN_BIT)
#define PIN_WRREQ_MASK  (1<<PIN_WRREQ_BIT)

#define PIN_EMUEN_MASK  (1<<PIN_EMUEN_BIT)

bool drv_enabled = false;
bool write_request = false;

void driveEnabled() {
  drv_enabled = !(PIN_EN_REG & PIN_EN_MASK);

  //ext 5v from physical cable or mainboard jumper
  bool emuEn = PORT_EMUEN & PIN_EMUEN_MASK;  //digitalRead(emuEnPin);
  drv_enabled = drv_enabled & emuEn;  

  handle_drive_enable();
}

void writeRequest() {
  write_request = !(PIN_WR_REG & PIN_WRREQ_MASK);

  //Toggle WR led
  PORT_WRLED = (PORT_WRLED & ~(1 << WRLED_DATA_BIT)) | (write_request << WRLED_DATA_BIT);
}
