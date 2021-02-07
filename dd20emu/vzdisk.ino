//#define DEBUG_TRACK 1

// SD chip select pin.  Be sure to disable any other SPI devices such as Enet.
#define SD_CS_PIN SS

/**
 * Disk image formats:
 * Type 1. file size 98,560 bytes, 40 tracks x 16 sectors x 154 bytes
 * Type 2. file size 99184 bytes (missing last padding)
 * Type 3. file size is 99200 bytes
**/

uint8_t fdc_sector[SECSIZE_VZ];

const int sector_interleave[SEC_NUM] = { 0, 11, 6, 1, 12, 7, 2, 13, 8, 3, 14, 9, 4, 15, 10, 5 };
const int inversed_sec_interleave[SEC_NUM] = {0, 3, 6, 9, 12, 15, 2, 5, 8, 11, 14, 1, 4, 7, 10, 13};

int8_t sec_lut[40][16] = { 0 };

#include "vzdisk.h"

vzdisk::vzdisk()
{
  if (!SD.begin(SD_CS_PIN))
  {
    serial_log(PSTR("Failed to begin on SD"));
  }
  else {
    sdInitialized = true;
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
    serial_log(PSTR("%s is type 1 image, no padding.\r\n"), filename);
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
      serial_log(PSTR("%s is type 2/3 image, padding = %d bytes\r\n"), filename, padding);
    }
  }  
}

void vzdisk::build_sector_lut()
{
  uint8_t buf[13] = {0};
  uint8_t n[1] = {0};
  unsigned long offset = 0;
  uint16_t sectors = 0;
  uint16_t elapsed = 0;

  serial_log(PSTR("Start building sector LUT.\r\n"));
  elapsed = millis();

  while(offset <(file.size()-13)) {
    file.seek(offset);
    file.read(buf,13);

    if (buf[0] == 0x80 && buf[1] == 0x80 && buf[2] == 0x80 && buf[3] == 0x80 && buf[4] == 0x80 && buf[5] == 0x80 &&
        buf[6] == 0x00 && buf[7] == 0xFE && buf[8] == 0xE7 && buf[9] == 0x18 && buf[10]== 0xC3)
    {
      uint8_t TR = buf[11];
      uint8_t SEC= inversed_sec_interleave[buf[12]];
      
      unsigned long expected_offset = (unsigned long)TR*(16*sizeof(sector_t)+padding) + (unsigned long)SEC*sizeof(sector_t);
      int delta = offset + 1 - expected_offset;
      sec_lut[TR][SEC] = delta - TR*16 /*SEC_NUM*/;

      //spec sector size: 154, trim the first byte      
      offset += (13+ 141);
      sectors++;
      
      //if this is the last sector of  
      if (buf[12] == 0x05 && padding!=0) {
        offset += 15; //+15 or 16
      }
    }
    else if (buf[0] == 0x80 && buf[1] == 0x80 && buf[2] == 0x80 && buf[3] == 0x80 && buf[4] == 0x80 && buf[5] == 0x00 &&
        buf[6] == 0xFE && buf[7] == 0xE7 && buf[8] == 0x18 && buf[9] == 0xC3)
    {
      uint8_t TR = buf[10];
      uint8_t SEC = inversed_sec_interleave[buf[11]];
      
      unsigned long expected_offset = (unsigned long)TR*(16*sizeof(sector_t)+padding) + (unsigned long)SEC*sizeof(sector_t);
      int delta = offset - expected_offset;
      sec_lut[TR][SEC] = delta - TR*16 /*SEC_NUM*/;

      //Exceptional sector size: 153      
      offset += (12 + 141);
      sectors++;
       
      //if this is the last sector of  
      if (buf[11] == 0x05 && padding!=0) {
        offset += 15;
      }
    }
    else {
      offset++;
    }
  }

#if 0 //Dump Sector LUT
  //2021-02-07 BUG: The sector_lut is always 1 to 16 on every track for some reason, these must be a calculation error,
  //I suspect Laser310 DI-40 fall out of sync every once a while... there are some retries.
  for(int i=0; i<TRK_NUM; i++)
  {
    serial_log(PSTR("TR: %d\r\n"),i);
    for(int j=0; j<SEC_NUM; j++)
    {
      serial_log(PSTR("%d "), sec_lut[i][j]);
    }
    serial_log(PSTR("\r\n"));
  }
#endif

  serial_log(PSTR("Finished, used %d ms.\r\n"), millis()-elapsed);
  serial_log(PSTR("Sectors found: %d\r\n"), sectors);
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
    unsigned long calculated_offset = expected_offset + (sec_lut[n][s] + n*16 /*SEC_NUM*/);

    if (file.seek(calculated_offset) != false)
    { 
      result= file.read(fdc_sector, SECSIZE_VZ);

#ifdef   NORMALIZEZD_SECTOR_HDR      
      if (result != -1)
      {
        sec_hdr_t *sec_hdr = (sec_hdr_t *)fdc_sector;
        
        //FE, E7, 18, C3
        if (sec_hdr->IDAM_leading[0] == 0xFE && sec_hdr->IDAM_leading[1] == 0xE7 && sec_hdr->IDAM_leading[2] == 0x18 && sec_hdr->IDAM_leading[3] == 0xC3) {
          if (n == sec_hdr->TR && sector_interleave[s] == sec_hdr->SC) {
            return result;
          }
          else {
            serial_log(PSTR("Expecting T:%d, S%d, but got T:%d, S:%d\r\n"), n, s, sec_hdr->TR, sec_hdr->SC);
          }
        }
        else {
          serial_log(PSTR("Invalid IDAM T:%d, S%d\r\n"), n, s);
        }
      }
      else {
        serial_log(PSTR("Failed to read T:%d, S%d\r\n"), n, s);
      }
#endif

    }
    else {
      serial_log(PSTR("Failed to seek to T:%d, S%d\r\n"), n, s);
    }
  }
  else {
      serial_log(PSTR("Invalid sector T:%d, S%d requested\r\n"), n, s);
  }

  return result;
}
