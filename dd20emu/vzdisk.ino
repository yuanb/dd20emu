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
const uint8_t inversed_sec_interleave[SEC_NUM] PROGMEM = {0, 3, 6, 9, 12, 15, 2, 5, 8, 11, 14, 1, 4, 7, 10, 13};

#include "vzdisk.h"

vzdisk::vzdisk(sector_lut* lut)
{
  if (!SD.begin(SD_CS_PIN))
  {
    serial_log(PSTR("Failed to begin on SD"));
    return;
  }

  if (lut == NULL)
  {
    serial_log(PSTR("lut is NULL"));
    return;
  }

  this->lut = lut;
  sdInitialized = true;
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
      serial_log(PSTR("Can't find %s\r\n"), fname);
    }
  }

  return result;
}

void vzdisk::set_track_padding()
{
  unsigned long fsize = file.size();
  
  if (fsize == 98560) {
    padding = 0;    
    serial_log(PSTR("%s is type 1 image, padding= %d bytes\r\n"), this->filename, padding);
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

int vzdisk::build_sector_lut()
{
  uint8_t buf[13] = {0}, TR, SEC;
  unsigned long offset = 0;
  uint16_t sectors = 0;
  uint16_t elapsed = 0;
  int8_t gap1_size = -1;

  serial_log(PSTR("Start building sector LUT.\r\n"));
  elapsed = millis();

  while(offset <(file.size()-13)) {
    file.seek(offset);
    file.read(buf,13);

    /* 0x80 6 times, then 0x00 */
    /**
     * We should use : 
     *    int delta = offset + 0 - expected_offset;
     *      then
     *    offset += (13 + 141);
     * in case of 6 bytes 0x80, otherwise the lut table will also be '1' for every sector
     * I belive there is a bug : I have to put
     *    int delta = offset + 1 - expected_offset;
     *      and
     *    offset += (12 + 141);
     *  
     *  If the current sector has spec defined sync bytes, '0' is registered in the sec_lut, otherwise '1' is put in lut to mark a 
     *  sector with short 5 bytes + 00h sync words
     */
    gap1_size = lut->sync_gap1(buf, TR, SEC);
    if (gap1_size != -1)
    {
      uint8_t SEC_IDX= pgm_read_byte_near(&inversed_sec_interleave[SEC]);
      unsigned long expected_offset = (unsigned long)TR*(SEC_NUM*sizeof(sector_t)+padding) + (unsigned long)SEC_IDX*sizeof(sector_t);
      //TODO: Correct value : change 1 to 0 for 7 bytes header
      int delta = offset + (gap1_size==7? 1 : 0) - expected_offset;
      uint8_t value = delta - TR*16 - SEC_IDX;     

      if (SEC_IDX%2==0) {
        //high half
        sec_lut[TR][SEC_IDX/2] = value << 4;
      } else {
        //low half
        sec_lut[TR][SEC_IDX/2] = sec_lut[TR][SEC_IDX/2] | value;
      }
      
      //7 bytes GAP1, 4 bytes IDAM leading, 1 byte TR, 1 byte SEC
      //TODO: Correct value : change 0 to 1 for 7 bytes header
      offset += (gap1_size + 4 + 1 + (gap1_size==7 ? 0 : 1) + SEC_REMAINING);
      sectors++;
      
      //if this is the last sector of  
      if (SEC_IDX==15) {
        offset += padding;
      }      
    }
    else {
      offset++;
    }
  }

#if 1 //Dump Sector LUT
  serial_log(PSTR("\r\nsec_lut: '0' means 6x80h+1x00h; '1' mean 5x80h+1x00h \r\n"));
  for(int i=0; i<TRK_NUM; i++)
  {
    serial_log(PSTR("TR:%02d  "),i);
    for(int j=0; j<SEC_NUM/2; j++)
    {
      serial_log(PSTR("%02x "), sec_lut[i][j]);
    }
    serial_log(PSTR("\r\n"));
  }
#endif

  serial_log(PSTR("Found %d sectors in %d ms.\r\n"), sectors, millis()-elapsed);
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

            if (lut->sync_gap1(buf, TR, SEC) == -1)
            {
              serial_log(PSTR("check_gap1 on offset %04lX failed at [%d][%d]\r\n"), offset, i,j);
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

/**
 * n : Requested Track number
 * s : Requested Sector number
 */
int vzdisk::get_sector(uint8_t n, uint8_t s)
{
  int result = -1;
  if (n<TRK_NUM && s<SEC_NUM)
  {  
    unsigned long expected_offset = (unsigned long)n*(SEC_NUM*sizeof(sector_t)+padding) + (unsigned long)s*sizeof(sector_t);
    uint8_t value = s%2==0 ? (sec_lut[n][s] >>4) : (sec_lut[n][s] & 0x0F);

    //Adjust offset!!!!!!!!!!!!!!
    unsigned long calculated_offset = expected_offset + (value + n*16 + s);

    if (file.seek(calculated_offset) != false && (result= file.read(fdc_sector, SECSIZE_VZ)) != -1)
    { 
      uint8_t TR, SEC;
      int gap1_size = lut->sync_gap1(fdc_sector, TR, SEC);
      if (gap1_size != -1 && TR == n && pgm_read_byte_near(&sector_interleave[s])==SEC) {
        return result;
      }
      else {
        serial_log(PSTR("Validate sector header error on TR:%d, SEC:%d\r\n"), n,s);
      }
    }
    else {
      serial_log(PSTR("Failed to seek/read to T:%d, S%d\r\n"), n, s);
    }
  }
  else {
      serial_log(PSTR("Invalid sector T:%d, S%d requested\r\n"), n, s);
  }

  return result;
}
