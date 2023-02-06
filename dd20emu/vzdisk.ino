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

/**
 * Disk image formats:
 * Type 1. file size 98,560 bytes, 40 tracks x 16 sectors x 154 bytes
 * Type 2. file size 99184 bytes (missing last padding)
 * Type 3. file size is 99200 bytes
**/


#include "vzdisk.h"

uint8_t fdc_sector[SECSIZE_VZ];
const uint8_t sector_interleave[SEC_NUM] PROGMEM = { 0, 11, 6, 1, 12, 7, 2, 13, 8, 3, 14, 9, 4, 15, 10, 5 };
const uint8_t inversed_sec_interleave[SEC_NUM] PROGMEM = {0, 3, 6, 9, 12, 15, 2, 5, 8, 11, 14, 1, 4, 7, 10, 13};
//sector header template
const uint8_t _sector_header[sizeof(sec_hdr_t)] PROGMEM = {
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00,
    0xFE, 0xE7, 0x18, 0xC3,
    0xFF, 0xFF, 0xFF,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x00,
    0xC3, 0x18, 0xE7, 0xFE
};

vzdisk::vzdisk()
{
  if (!SD.begin(SD_CS_PIN))
  {
    serial_log(PSTR("Failed to begin on SD"));
    sdInitialized = false;
  }
  else {
    sdInitialized = true;
  }

  current_offset = 0;
  current_track = 0;
  current_cur = 0;
  padding = 0;
  dsk_mounted = false;
}

vzdisk::~vzdisk()
{
  if (file) {
    file.close();
    serial_log(PSTR("DSK file is closed.\r\n"));
  }
}

int vzdisk::Open(char *filename)
{
  int result = -1;
  if (sdInitialized)
  {
    if (SD.exists(filename))
    {
      file = SD.open(filename, FILE_READ);
      if (file)
      {
        result = 0;
        set_track_padding();
        dsk_mounted = true;
        serial_log(PSTR("Mounted disk image: '%s'.\r\n"), filename);
      }
      else {
        serial_log(PSTR("DSK File is not opened\r\n"));
      }
    }
    else
    {
      serial_log(PSTR("Can't find %s\r\n"), filename);
    }
  }

  return result;
}

SdFat* vzdisk::get_sd()
{
  return &SD;
}

bool vzdisk::get_mounted()
{
  return dsk_mounted;
}

void vzdisk::set_track_padding()
{
  unsigned long fsize = file.size();
  
  if (fsize == 98560) {
    serial_log(PSTR("Type 1 image, no padding.\r\n"));
    padding = 0;
  }
  else //if (fsize == 99184 || fsize == 99200)
  {
    if (file.seek(TRKSIZE_VZ) == false) {
      serial_log(PSTR("Error when trying to identify dsk image format"));
      return;
    }
    
    uint8_t track_padding[16];
    file.read(track_padding, 16);
    bool padding_found = true;
    for(int i=0; i<16; i++) {
      if (track_padding[i]!=0) {
        padding_found = false;
        break;
      }
    }
    if (padding_found) {
      padding = 16;
      serial_log(PSTR("Type 2/3 image, padding = %d bytes\r\n"), padding);
    }
  }  
}

uint8_t vzdisk::get_track_padding()
{
  return padding;
}

unsigned long vzdisk::get_track_offset(uint8_t TR)
{
  unsigned long expected_track_offset = (unsigned long)TR*(SEC_NUM*sizeof(sector_t)+padding);
  return expected_track_offset;
}

bool vzdisk::get_sector(uint8_t TR, uint8_t SEC, bool v)
{
  bool result = false;
  uint8_t buf[SECSIZE_VZ+5];

  if (TR != current_track) {
    current_offset = get_track_offset(TR);
    current_track = TR;
    current_cur = 0;
  }

  while(true) {
    //TODO: break if have retried(scanned) the entire track for 3x times, this would help to slide the GAP1 window to sync'd position
    
    //if at the end of a track or the file, go back to beginning of the track
    if (0==file.available() || current_cur >= get_track_offset(1)) {
      current_offset = get_track_offset(current_track);
      current_cur = 0;
    }
 
    if (v) {
      serial_log(PSTR("\rcurrent_offset=%08lX, current_cur=%d, seek_to=%08lX"), current_offset, current_cur, current_offset+current_cur);
    }
    file.seek(current_offset + current_cur);
    file.read(buf, sizeof(buf));
            
    uint8_t cur = 0;
    int ngap1 = 0;

    //Sync to 0x80 0x80.... 0x00
    while(buf[cur] != 0x00) {
      if (buf[cur] == 0x80)
        ngap1++;
      cur++;
      if (cur>=SECSIZE_VZ)
        break;
    }

    //TODO:If drift is too much, the remaining in buffer wont have the whole sector, slide the window, read data from new pos
    if (cur>=SECSIZE_VZ) {
      serial_log(PSTR("Err1, seeded to :%08lX\r\n"), (current_offset+current_cur));
    }
    
    if (ngap1 >= 5 && buf[cur] == 0x00 && buf[cur+1] == 0xFE && buf[cur+2] == 0xE7 && buf[cur+3] == 0x18 && buf[cur+4] == 0xC3) {
        unsigned long idam = current_offset+current_cur+cur+1;
        cur += 5;
  
        uint8_t tr = buf[cur++];
        uint8_t sec = buf[cur++];
        uint8_t ts_sum = buf[cur++];
        assert( ts_sum==tr+sec );

        int ngap2 = 0;
        while(buf[cur] != 0x00) {
          if (buf[cur] == 0x80)
            ngap2++;
          cur++;
        }
        assert(ngap2 >= 4 && buf[cur] == 0x00 && buf[cur+1] == 0xC3 && buf[cur+2] == 0x18 && buf[cur+3] == 0xE7 && buf[cur+4] == 0xFE);
        cur+=5;

        if (tr == TR && pgm_read_byte_near(&inversed_sec_interleave[sec]) == SEC) {
          if (v) {
            serial_log(PSTR("\r\nfound idam -> TR: %d, SEC:%d, T+S:%d @ %08lX\r\n"),tr, sec, ts_sum, idam);
          }
          
          //data part
          uint16_t checksum = 0;
          for(int i=0; i<SEC_DATA_SIZE+2 /*TODO:remove when next tr, sec removed from sector struct*/; i++) {
            uint8_t value = buf[cur++];
            checksum += value;
            fdc_sector[sizeof(sec_hdr_t)+i] = value;
          }
          uint16_t checksum_exp = buf[cur++] + 256*buf[cur++];
          if (checksum != checksum_exp) {
            serial_log(PSTR("\r\nCalculated checksum:%04X, expected: %04X\r\n"), checksum, checksum_exp);
          }
          //real real checksum
          assert(checksum == checksum_exp);
          current_cur += cur; //TODO: should add '(sizeof(sec_hdr_t) + SEC_DATA_SIZE + 2 + 2', let's wait till memcpy is done

          //Copy sector to target fdc_sector
          memcpy_PF(fdc_sector, pgm_get_far_address(_sector_header), sizeof(sec_hdr_t));
          sec_hdr_t* sec_hdr = (sec_hdr_t *)fdc_sector;
          
          sec_hdr->TR = tr;
          sec_hdr->SC = sec;
          sec_hdr->TS_sum = ts_sum;
          ((sector_t *)fdc_sector)->checksum = checksum_exp;
          
          /*TODO: ideally, we would only copy the sector data to target from here, but it maybe more efficient to copy each byte int he checksum verification loop
          memcpy(&fdc_sector+sizeof(sec_hdr_t), buf[data_pos], SEC_DATA_SIZE+2+2); */
          result = true;
          break;
        }
        else {
          /*Maybe a little conservertive? not moving forware so many?*/
          current_cur += (sizeof(sec_hdr_t) + SEC_DATA_SIZE + 2/*TODO: next tr, sec*/ + 2/*of 2 bytes checksum */);
          continue;
        }
     }
    current_cur ++; //TODO: maybe + nsync is better
  }

  return result;
}
