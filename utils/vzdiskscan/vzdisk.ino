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

/**
 * Disk image formats:
 * Type 1. file size 98,560 bytes, 40 tracks x 16 sectors x 154 bytes
 * Type 2. file size 99184 bytes (missing last padding)
 * Type 3. file size is 99200 bytes
**/

uint8_t fdc_sector[SECSIZE_VZ];

//used in get_sector()
const uint8_t sector_interleave[SEC_NUM] PROGMEM = { 0, 11, 6, 1, 12, 7, 2, 13, 8, 3, 14, 9, 4, 15, 10, 5 };

//used in build_sector_lut()
const uint8_t reversed_index_sec_interleave[SEC_NUM] PROGMEM = {0, 3, 6, 9, 12, 15, 2, 5, 8, 11, 14, 1, 4, 7, 10, 13};

/*40 tracks, 8 bytes/track packed*/
uint8_t sec_lut[TRK_NUM][SEC_NUM] = { 0 };

#include "vzdisk.h"

vzdisk::vzdisk()
{
  if (!SD.begin(SD_CS_PIN))
  {
    serial_log(PSTR("Failed to begin on SD\r\n"));
  }
  else {
    sdInitialized = true;
  }  
}

int vzdisk::Open(char *fname)
{
  int result = -1;
  if (sdInitialized)
  {
    this->filename = fname;
    if (SD.exists(this->filename))
    {
      file = SD.open(this->filename, FILE_READ);
      if (file == false)
      {
        serial_log(PSTR("DSK File is not opened"));
      }
      else {
        result = 0;
      }
    }
    else
    {
      serial_log(PSTR("Can't find FLOPPY1.DSK"));
    }
  }

  return result;
}

void vzdisk::set_track_padding()
{
  unsigned long fsize = file.size();
  
  if (fsize == 98560) {
    serial_log(PSTR("%s is type 1 image, no padding.\r\n"), this->filename);
    padding = 0;
  }
  else //if (fsize == 99184 || fsize == 99200)
  {
    if (file.seek(TRKSIZE_VZ) == false) {
      serial_log(PSTR("Error when trying to identify dsk image format"));
      return;
    }
    
    uint8_t track_padding[PADDING_SIZE];
    file.read(track_padding, sizeof(track_padding));
    bool padding_found = true;
    for(int i=0; i<sizeof(track_padding); i++) {
      if (track_padding[i]!=0) {
        padding_found = false;
        break;
      }
    }
    if (padding_found) {
      padding = PADDING_SIZE;
    }
    serial_log(PSTR("%s is type 2/3 image, padding = %d bytes\r\n"), this->filename, padding);    
  }  
}

int vzdisk::sync_gap1(uint8_t* buf, uint8_t* TR, uint8_t* SEC, int debug=0)
{
    int gap1_size = -1;

    //Sync bytes
    if (*buf++ == 0x80 && *buf++ == 0x80 && *buf++ == 0x80 && *buf++ == 0x80 && *buf++ == 0x80)
    {
        if (*buf == 0x80) {
            buf++;
            if (*buf++ == 0x00) {
                gap1_size = 7;
            }
        } else if (*buf++ == 0x00) {
            gap1_size = 6;
        }
        //IDAM
        if (*buf++ == 0xFE && *buf++ == 0xE7 && *buf++ == 0x18 && *buf++ == 0xC3) {
            *TR = *buf++;
            *SEC = *buf;
        }
    }

    if (debug == 1) {
      for(int i=0; i<LUT_SCAN_STEP; i++) {
        serial_log(PSTR("%02X "), buf[i]);
      }
      serial_log(PSTR("\r\n"));
    }
    return gap1_size;
}

int vzdisk::build_sector_lut()
{
    uint8_t buf[LUT_SCAN_STEP] = {0};
    uint32_t offset = 0;
    uint16_t sectors = 0;
    uint16_t elapsed = millis();
    uint8_t TR, SEC;
    int debug = 0;    

    serial_log(PSTR("Start building sector LUT.\r\n"));
    uint32_t filesize = file.size();

    while (offset < filesize-LUT_SCAN_STEP) {
        if (file.seek(offset) == false) {
          serial_log(PSTR("Failed seek to: %04lX\r\n\r\n\r\n\r\n"), offset);
          return 0;
        }
        file.read(buf,sizeof(buf));
    
        int gap1_size = sync_gap1(buf, &TR, &SEC, debug);
        if (gap1_size != -1) {
            uint8_t SEC_IDX = pgm_read_byte_near(&reversed_index_sec_interleave[SEC]);
            sec_lut[TR][SEC_IDX] = offset - TR*(TRKSIZE_VZ+padding) - SEC_IDX*SECSIZE_VZ;
            //serial_log(PSTR("TR:%d, SEC_IDX:%d, value:%d\t"), TR, SEC_IDX, sec_lut[TR][SEC_IDX]);

            //7 bytes GAP1, 4 bytes IDAM leading, 1 byte TR, 1 byte SEC
            //6 bytes GAP1, 4 bytes IDAM leading, 1 byte TR, 1 byte SEC   
            //serial_log(PSTR("offset=%04lX, "), offset);
            offset += (gap1_size + 4 + 1 + 1 + SEC_REMAINING);
            sectors++;
            //serial_log(PSTR("%d bytes sync : TR: %d, SEC: %d, \tnext offset to read:%04lX\r\n"), gap1_size, TR, SEC, offset);

            //if this is the last sector of a track
            if (SEC == sector_interleave[SEC_NUM-1] && padding!=0) {
                offset += padding;
                //printf("This is the last sector of TR:%d, moving to next track with padding=%d\r\n\r\n", TR, padding);
            }            
        }
        else {
            //serial_log(PSTR("............Searching sync bytes, skipped 1 byte[%02X] at %04lX, "), buf[0], offset);
            offset++;
            //serial_log(PSTR("\t\t\tnext offset to read:%04lX\r\n"), offset);
        }
    }

#if 0 //Dump Sector LUT
  serial_log(PSTR("\r\nsec_lut: '0' means 6x80h+1x00h; '1' mean 5x80h+1x00h \r\n"));
  for(int i=0; i<TRK_NUM; i++)
  {
    serial_log(PSTR("TR:%02d  "),i);
    for(int j=0; j<SEC_NUM; j++)
    {
      serial_log(PSTR("%d "), sec_lut[i][j]);
    }
    serial_log(PSTR("\r\n"));
  }
#endif

  serial_log(PSTR("Found %d sectors in %d ms.\r\n"), sectors, millis()-elapsed);
  return sectors;  
}

int vzdisk::validate_sector_lut()
{
    int result = -1;
    uint8_t TR, SEC;
    uint8_t buf[LUT_SCAN_STEP] = {0};

    for(int i=0; i<TRK_NUM; i++)
    {
        for(int j=0; j<SEC_NUM; j++)
        {
            uint32_t offset = sec_lut[i][j] + (uint32_t)i*(TRKSIZE_VZ+padding) + (uint32_t)j*SECSIZE_VZ;
            file.seek(offset);
            file.read(buf, sizeof(buf));

            int gap1_size = sync_gap1(buf, &TR, &SEC);
            if (sync_gap1(buf, &TR, &SEC) ==-1)
            {
              serial_log(PSTR("check_gap1 on offset %04lX failed at [%d][%d]\r\n"), offset, i,j);
              sync_gap1(buf, &TR, &SEC, 1);
              return result;
            }

            if (TR!=i || SEC !=pgm_read_byte_near(&sector_interleave[j]))
            {
                serial_log(PSTR("scan_sctor_lut failed at TR=%d, SEC=%d\r\n"), TR, SEC);
                return result;
            }
        }
    }
    result = 1;

    serial_log(PSTR("All sector offsets in lut are validated.\r\n"));
    return result;
}
