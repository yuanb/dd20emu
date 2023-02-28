/*
    DD-20 emulator
    Copyright (C) 2020,2023 https://github.com/yuanb/

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

// handle diagnostic informations given by assertion and abort program execution:
void __assert(const char *__func, const char *__file, int __lineno, const char *__sexp) {
    // transmit diagnostic informations through serial link. 
    Serial.println(__func);
    Serial.println(__file);
    Serial.println(__lineno, DEC);
    Serial.println(__sexp);
    Serial.flush();
    // abort program execution.
    abort();
}

void print_buf8(uint8_t* buf, size_t buf_size)
{
  for(size_t i=0; i<buf_size; i+=16)
  {
    size_t finish = i+16 < buf_size ? i+16 : buf_size;

    for(size_t j=i; j<i+16; j++)
    {
      if (j<finish) {
        serial_log(PSTR("%02X "), buf[j]);
      }
      else {
        serial_log(PSTR("   "));
      }
    }

    for(size_t j=i; j<finish; j++)
    {
      if (buf[j]>0x20 && buf[j]<0x7f) {
        serial_log(PSTR("%c"), buf[j]);
      }
      else {
        serial_log(PSTR("."));
      }
    }

    serial_log(PSTR("\r\n"));      
  }
}

void print_buf16(uint16_t* buf, size_t buf_size, bool decimal)
{
  for(size_t i=0; i < buf_size; i+=16)
  {
    size_t finish = i+16 < buf_size ? i+16 : buf_size;

    for(size_t j=i; j<finish; j++)
    {
      if (decimal) {
        serial_log(PSTR("%d"), buf[j]);
      }
      else {
        serial_log(PSTR("%04X"), buf[j]);
      }

      if (j<(finish-1))
        serial_log(PSTR(","));
    }

    serial_log(PSTR("\r\n"));
  }
}
