#include <SPI.h>
#include <SdFat.h>
#include "vzdisk.h"

#define SD_CS_PIN 4

//char filename[] = "HELLO.DSK";
//char filename[] = "empty.dsk";
//char filename[] = "floppy1.dsk";
//char filename[] = "floppy2.dsk";
char filename[] = "extbasic.dsk";

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

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  serial_log(PSTR("\r\nVTech DD20 Image scanner, v0.0.8, 02/21/2021\r\n"));

  vzdisk reader = vzdisk();
  reader.Open(filename);
  reader.set_track_padding();  
  reader.build_sector_lut();
  reader.validate_sector_lut();
}

void loop() {
  // put your main code here, to run repeatedly:

}
