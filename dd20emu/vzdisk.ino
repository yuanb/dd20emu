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

uint8_t fdc_sector[155/*SECSIZE_VZ*/];

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
    if (file.seek(offset) == false || file.read(buf, 13) == -1) {
      serial_log(PSTR("Failed on building sector lut table.\r\n"));
      break;
    }

    gap1_size = lut->sync_gap1(buf, TR, SEC);
    if (gap1_size != -1)
    {
      uint8_t SEC_IDX= pgm_read_byte_near(&inversed_sec_interleave[SEC]);
      //TODO: Correct valvalueue : change 1 to 0 for 7 bytes header
      //1 : adjust 7 bytes GAP1 to 6 bytes for lut value
      //0 : keep 7 bytes spec GAP1
      uint8_t lut = offset + /*(gap1_size==7? 1 : 0)*/0 - TR*(TRKSIZE_VZ+padding) - SEC_IDX*SECSIZE_VZ;;     

      if (SEC_IDX%2==0) {
        //high nibble
        sec_lut[TR][SEC_IDX/2] = lut << 4;
      } else {
        //low nibble
        sec_lut[TR][SEC_IDX/2] = sec_lut[TR][SEC_IDX/2] | lut;
      }
      
      //n bytes GAP1, 4 bytes IDAM leading, 1 byte TR, 1 byte SEC
      //TODO: Correct value : change 0 to 1 for 7 bytes header
      //1 : use adjusted GAP1(6 bytes), slide sector by 1 bytes
      //0 : use spec GAP1, no adjustment for offset
      offset += (gap1_size + 4 + 1 + /*(gap1_size==7 ? 0 : 1)*/1 + SEC_REMAINING);
      sectors++;
      
      //if this is the last sector of  
      if (SEC_IDX==SEC_NUM-1) {
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
            uint8_t value = j%2==0 ? (sec_lut[i][j/2] >>4) : (sec_lut[i][j/2] & 0x0F);
            uint32_t offset = value + (uint32_t)i*(TRKSIZE_VZ+padding) + (uint32_t)j*SECSIZE_VZ;
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

/*
 * n : Requested Track number
 * s : Requested Sector(inidex) number
 */
int vzdisk::get_lut(uint8_t n, uint8_t s)
{
  return s%2==0 ? (sec_lut[n][s/2] >>4) : (sec_lut[n][s/2] & 0x0F); 
}

uint32_t vzdisk::get_offset(int lut, uint8_t n, uint8_t s)
{
  return lut + (uint32_t)n*(TRKSIZE_VZ+padding) + (uint32_t)s*SECSIZE_VZ;
}

/*
 * n : Requested Track number
 * s : Requested Sector(inidex) number
 */
uint32_t vzdisk::get_next_sector_offset(uint8_t n, uint8_t s)
{
  uint32_t result = 0;

  int next_tr, next_sec;
  if (s != 15)
  {
    next_tr = n;
    next_sec = s+1;
  }
  else
  {
    next_sec = 0;    
    if (n != 39)
    {
      next_tr = n+1;
    }
    else
    {
      //n==39, s==15, this is the last sector of last track;
      next_tr = -1;
    }
  }

  if (next_tr != -1)
  {
    int value = get_lut(next_tr, next_sec);
    result = get_offset(value, next_tr, next_sec);
    if (s == 15)
    {
      //HACK : hello.dsk, tr:3, sec:15 has 154 bytes, the padding has 15 bytes, this will fix the hello.dsk/ghost2' loading problem
      //There need some fix to detect and adjust
      //With this hack, this files still have problem
      //FLOPPY2.dsk 'LUNAR' 
      //FLOPPY1.dsk 'PENGUIN'

      //This problem mean, at current stage, the sector adjustment, lut are good, just occasionally, there are some sector are cut 
      //off at the end by 1 or 2 bytes.
      result -= (padding-1);
    }
    //serial_log(PSTR("\r\n\r\nnext_tr=%d, next_sec=%d, TR:%d, SEC:%d, next_offset=%04lX, result=%04lX"), next_tr, next_sec, n, s, 
    //            get_offset(value, next_tr, next_sec), result);    
  }

  return result;
}

/*
if lut value is 0, the sector size could be 154 or 155
    155 : TR0 to TR6 are used in hello1.dsk, SEC#0 of 0~6 track of hello1.dsk, 
            this is caused by 7 bytes(80 80 80 80 80 80 00) GAP1 and 7 bytes(80 80 80 80 80 80 00) of GAP2 leading
        CORRECT GAP1, WRONG GAP2(+1)
        
    154 : Each un-used sector which lut is 0, the GAP1 is always 7 bytes(80 80 80 80 80 80 00), GAP2 leading is 
            always 6 bytes(80 80 80 80 80 00)
        CORRECT GAP1, CORRECT GAP2

Algorithm : Output everything; report actual size
    use read buffer size 155, 
    read [current_offset, next_offset),
    return next_offset-current_offset to get_sector() caller

if lut value is 1, the sector size could be 154 or 153
    154 : TR:39, SEC#1 of hello,dsk(starting 0x17B9F) is a 154 bytes sector.
            this is caused by 6 bytes(80 80 80 80 80 00) of GAP1, 7 bytes(80 80 80 80 80 80 00) of GAP2 leading.
        WRONG GAP1(-1), WRONG GAP2(+1)

    153 : TR:39, SEC#11 of hello.dsk(starting 0x17A6B) is a 153 bytes sector. 
            6 bytes(80 80 80 80 80 00) of GAP1, 6 bytes(80 80 80 80 80 00) of GAP2 leading.
        WRONG GAP1(-1), CORRECT GAP2

Algorithm : add 0x80 to the front, report actual size + 1
    use read buffer size 155
    *buf_ptr++ = 0x80;
    read [current_offset, next_offset) to buf_ptr,
    return next_offset-current_offset+1 to get_sector() caller

*/
/**
 * n : Requested Track number
 * s : Requested Sector(inidex) number
 */
int vzdisk::get_sector(uint8_t n, uint8_t s)
{
  int result = -1;
  int value = get_lut(n,s);  
  uint32_t offset = get_offset(value, n,s);

  //serial_log(PSTR("\r\nn=%d, s=%d, sec_lut[n][s/2]=%02X, value=%d, offset=%04lX"), n,s, sec_lut[n][s/2], value, offset);
  if (file.seek(offset) == false)
  {
    serial_log(PSTR("Failed on seek to TR:%d, SEC:%d\r\n"), n,s);
    return result;
  }

  int read_size = SECSIZE_VZ;

  if (value == 0)
  {
      //value == 0, the read_size could either be 155 or 154
      uint32_t next_offset = get_next_sector_offset(n,s);
      if (next_offset != 0) {
        read_size = (next_offset - offset);
      }
      else {
        //Note: get_next_sector_offset returned 0, the (n,s) is TR:39, SEC:15
        read_size = file.size() - offset;
      }
      
      assert(read_size == 155 || read_size == 154);
      file.read(fdc_sector, read_size);
      result = read_size;
      //serial_log(PSTR("\r\n'0' -> n:%d, s:%d, offset=%04lX, result=%d\r\n"), n, s, offset, result);      
  }
  else
  {
    //value ==1, the read_size could either be 154 or 153
    fdc_sector[0] = 0x80;
    
    uint32_t next_offset = get_next_sector_offset(n,s);
    if (next_offset != 0) {
      read_size = (next_offset - offset);
    }
    else {
      //Note: get_next_sector_offset returned 0, the (n,s) is TR:39, SEC:15
      read_size = file.size() - offset;
    }

    assert(read_size == 154 || read_size == 153);
    file.read(fdc_sector, read_size);
    //result = read_size+1;
    result = read_size;
    //serial_log(PSTR("\r\n'1' -> n:%d, s:%d, offset=%04lX, result=%d\r\n"),n, s, offset, result);    
  }
  
//  if (result != -1)
//  { 
//    uint8_t TR, SEC;
//    int gap1_size = lut->sync_gap1(fdc_sector, TR, SEC);
//    if (gap1_size != -1 && TR == n && pgm_read_byte_near(&sector_interleave[s])==SEC) {
//      return result;
//    }
//    else {
//      result = -1;
//      serial_log(PSTR("Validate sector header error on TR:%d, SEC:%d\r\n"), n,s);
//    }
//  }
//  else {
//    serial_log(PSTR("Failed to seek/read to T:%d, S%d\r\n"), n, s);
//  }

  return result;
}
