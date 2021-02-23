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

#include "utilities.h"

/**
 * LED visual when Drive is enabled
 * Low is enabled, power led when main unit is turned off
 */
bool old_drv_enabled = -1; 
inline void handle_drive_enable() {
  bool isEnabled = drv_enabled;
  if (isEnabled != old_drv_enabled) {
    digitalWrite(LED_BUILTIN, isEnabled);
    old_drv_enabled = isEnabled;
  }
}

/**
 * Serial log on R/W request
 */
bool old_wr_req = -1;
void handle_wr_request() {
  if (!drv_enabled)
    return;

  bool wrRequest = write_request;
  if (wrRequest != old_wr_req) {
    //serial_log(PSTR("Trk: %d, %s"), vtech1_track_x2 / 2, wrRequest ? "Write" : "Read");
    old_wr_req = wrRequest;
  }
}

void serial_log( const char * format, ... )
{
  char buffer[256];
  va_list args;
  va_start (args, format);
#ifdef __AVR__
    vsnprintf_P(buffer, sizeof(buffer), (const char *)format, args);     // progmem for AVR
#else
    vsnprintf(buffer, sizeof(buffer), (const char *)format, args);     // for the rest of the world
#endif  
//  vsprintf (buffer, format, args);
  Serial.print(buffer);
  va_end (args);
}

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}
