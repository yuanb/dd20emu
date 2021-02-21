//#define DEBUG_TRACK 1

/**
 * Disk image formats:
 * Type 1. file size 98,560 bytes, 40 tracks x 16 sectors x 154 bytes
 * Type 2. file size 99184 bytes (missing last padding)
 * Type 3. file size is 99200 bytes
**/

uint8_t fdc_sector[SECSIZE_VZ];

const uint8_t sector_interleave[SEC_NUM] = { 0, 11, 6, 1, 12, 7, 2, 13, 8, 3, 14, 9, 4, 15, 10, 5 };
const uint8_t inversed_sec_interleave[SEC_NUM] = {0, 3, 6, 9, 12, 15, 2, 5, 8, 11, 14, 1, 4, 7, 10, 13};

/*40 tracks, 8 bytes/track */
int8_t sec_lut[TRK_NUM][SEC_NUM/2] = { 0 };

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
  int8_t oldTR = -1;

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
     */
    if (buf[0] == 0x80 && buf[1] == 0x80 && buf[2] == 0x80 && buf[3] == 0x80 && buf[4] == 0x80 && buf[5] == 0x80 &&
        buf[6] == 0x00 && buf[7] == 0xFE && buf[8] == 0xE7 && buf[9] == 0x18 && buf[10]== 0xC3)
    {
      uint8_t TR = buf[11];
      uint8_t SEC= inversed_sec_interleave[buf[12]];
      
      unsigned long expected_offset = (unsigned long)TR*(16*sizeof(sector_t)+padding) + (unsigned long)SEC*sizeof(sector_t);
      int delta = offset + 1 - expected_offset;
#if 0
      sec_lut[TR][SEC] = delta;
#else
      uint8_t value = delta - TR*16 - SEC;
      if (SEC%2==0) {
        //high half
        sec_lut[TR][SEC/2] = value << 4;
      } else {
        //low half
        sec_lut[TR][SEC/2] = sec_lut[TR][SEC/2] | value;
      }

      if (oldTR!=TR) {
#if 0        
        serial_log(PSTR("\r\n"));
#endif
        oldTR = TR;
      }
#if 0        
      serial_log(PSTR("0"));
#endif
#endif

      //spec sector size: 154, trim the first byte
      offset += (12 + 0 + 141);
      sectors++;
      
      //if this is the last sector of  
      if (buf[12] == 0x05 && padding!=0) {
        offset += 15; //+15 or 16
      }
    }
    /* 0x80 5 times, then 0x00 */
    else if (buf[0] == 0x80 && buf[1] == 0x80 && buf[2] == 0x80 && buf[3] == 0x80 && buf[4] == 0x80 && buf[5] == 0x00 &&
        buf[6] == 0xFE && buf[7] == 0xE7 && buf[8] == 0x18 && buf[9] == 0xC3)
    {
      uint8_t TR = buf[10];
      uint8_t SEC = inversed_sec_interleave[buf[11]];
      
      unsigned long expected_offset = (unsigned long)TR*(16*sizeof(sector_t)+padding) + (unsigned long)SEC*sizeof(sector_t);
      int delta = offset - expected_offset;
#if 0
      sec_lut[TR][SEC] = delta;
#else
      uint8_t value = delta - TR*16 - SEC;   
      if (SEC%2==0) {
        //high half
        sec_lut[TR][SEC/2] = value << 4;
      } else {
        //low half      
        sec_lut[TR][SEC/2] = sec_lut[TR][SEC/2] | value;   
      }    
      
      if (oldTR!=TR) {
#if 0        
        serial_log(PSTR("\r\n"));
#endif
        oldTR = TR;
      }
#if 0         
      serial_log(PSTR("1"));
#endif
#endif

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

#if 1 //Dump Sector LUT
  //2021-02-07 BUG: The sector_lut is always 1 to 16 on every track for some reason, these must be a calculation error,
  //I suspect Laser310 DI-40 fall out of sync every once a while... there are some retries.
  serial_log(PSTR("\r\nsec_lut:\r\n"));
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

/**
 * n : Requested Track number
 * s : Requested Sector number
 */
int vzdisk::get_sector(uint8_t n, uint8_t s)
{
  int result = -1;
  if (n<TRK_NUM && s<SEC_NUM)
  {
#if 0
    unsigned long expected_offset = (unsigned long)n*(16*sizeof(sector_t)+padding) + (unsigned long)s*sizeof(sector_t);
    unsigned long calculated_offset = expected_offset + sec_lut[n][s];  
#else    
    unsigned long expected_offset = (unsigned long)n*(SEC_NUM*sizeof(sector_t)+padding) + (unsigned long)s*sizeof(sector_t);
    uint8_t value = s%2==0 ? (sec_lut[n][s] >>4): (sec_lut[n][s] & 0x0F);
    unsigned long calculated_offset = expected_offset + (value + n*16 + s);
#endif

    if (file.seek(calculated_offset) != false)
    { 
      result= file.read(fdc_sector, SECSIZE_VZ);

#ifdef   NORMALIZEZD_SECTOR_HDR      
      if (result != -1)
      {
#if 1      
        sec_hdr_t *sec_hdr = (sec_hdr_t *)fdc_sector;
#else
        sec_hdr_t *sec_hdr = (sec_hdr_t *)fdc_sector + sec_lut[n][s];
#endif
        
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
