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

bool drv_enabled = false;
bool write_request = false;

//Arduino interruption on pin change, nice Arduino interrupt tutorial
//https://arduino.stackexchange.com/questions/8758/arduino-interruption-on-pin-change
void driveEnabled() {
  drv_enabled = !(PIN_EN_REG & PIN_EN_MASK);
  handle_drive_enable();
}

void writeRequest() {
  write_request = !(PIN_WR_REG & PIN_WRREQ_MASK);
}
