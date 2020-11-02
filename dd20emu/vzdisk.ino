//#define DEBUG_TRACK 1

/**
 * Disk image formats:
 * Type 1. file size 98,560 bytes, 40 tracks x 16 sectors x 154 bytes
 * Type 2. file size 99184 bytes (missing last padding)
 * Type 3. file size is 99200 bytes
 */

 #define  NORMALIZEZD_SECTOR_HDR   1
 

typedef struct SectorHeader {
#ifdef   NORMALIZEZD_SECTOR_HDR
  uint8_t   GAP1[6];    //0x80 5 times, then 0x00
#else
  uint8_t   GAP1[7];    //0x80 6 times, then 0x00
#endif
  uint8_t   IDAM_leading[4];  //FE, E7, 18. C3
  uint8_t   TR;
  uint8_t   SC;
  uint8_t   TS_sum;
  uint8_t   GAP2[6];    //0x80 5 times, 0x00
  uint8_t   IDAM_closing[4];  //C3, 18, E7, FE
} sec_hdr_t;

typedef struct Sector {
  sec_hdr_t   header;
  uint8_t     data_bytes[126];
  uint8_t     next_track;
  uint8_t     next_sector;
  uint16_t    checksum;
} sector_t;

typedef struct Track {
  sector_t    sector[16];
} track_t;

//uint8_t   fdc_data[TRKSIZE_VZ];
uint8_t fdc_sector[SECSIZE_VZ];

//#define TRKSIZE_FM  3172
//uint8_t   fm_track_data[TRKSIZE_FM];
uint8_t padding = 0;

int sector_interleave[SEC_NUM] = { 0, 11, 6, 1, 12, 7, 2, 13, 8, 3, 14, 9, 4, 15, 10, 5 };
int inversed_sec_interleave[SEC_NUM] = {0, 3, 6, 9, 12, 15, 2, 5, 8, 11, 14, 1, 4, 7, 10, 13};

void set_track_padding(File f)
{
  unsigned long fsize = f.size();
  
  if (fsize == 98560) {
    serial_log(PSTR("%s is type 1 image, no padding.\r\n"), filename);
    padding = 0;
  }
  else //if (fsize == 99184 || fsize == 99200)
  {
    if (f.seek(TRKSIZE_VZ) == false) {
      serial_log(PSTR("Error when trying to identify dsk image format"));
      return;
    }
    
    uint8_t track_padding[16];
    f.read(track_padding, 16);
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

unsigned long sector_lut[40][16] = { 0 };

void build_sector_lut(File f)
{
  uint8_t buf[13] = {0};
  uint8_t n[1] = {0};
  unsigned long offset = 0;
  uint16_t sectors = 0;
  uint16_t elapsed = 0;

  serial_log(PSTR("Start building sector LUT.\r\n"));
  elapsed = millis();

  while(offset <(f.size()-13)) {
    f.seek(offset);
    f.read(buf,13);

    if (buf[0] == 0x80 && buf[1] == 0x80 && buf[2] == 0x80 && buf[3] == 0x80 && buf[4] == 0x80 && buf[5] == 0x80 &&
        buf[6] == 0x00 && buf[7] == 0xFE && buf[8] == 0xE7 && buf[9] == 0x18 && buf[10]== 0xC3)
    {
      //Exceptional sector size: 155, trim the first byte
      sector_lut[buf[11]][inversed_sec_interleave[buf[12]]] = offset+1;

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
      sector_lut[buf[10]][inversed_sec_interleave[buf[11]]] = offset;
      
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

  serial_log(PSTR("Finished, used %d ms.\r\n"), millis()-elapsed);
  serial_log(PSTR("Sectors found: %d\r\n"), sectors);
}

int get_sector(File f, uint8_t n, uint8_t s)
{
  int result = -1;
  if (n<TRK_NUM && s<SEC_NUM)
  {
    if (f.seek(sector_lut[n][s]) != false)
    { 
      result= f.read(fdc_sector, SECSIZE_VZ);

#ifdef   NORMALIZEZD_SECTOR_HDR      
      if (result != -1)
      {
        sec_hdr_t *sec_hdr = (sec_hdr_t *)fdc_sector;
        //FE, E7, 18. C3
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

/*
int get_track(File f, int n)
{
  int size = TRKSIZE_VZ;
  unsigned long offset = (unsigned long)TRKSIZE_VZ * n;

  if (n < 0 || n > 39)
  {
    serial_log(PSTR("Invalid track number %d\r\n"), offset);
    return -1;
  }

  if (f.seek(offset) == false)
  {
    serial_log(PSTR("Failed seek file to %ul\r\n"), offset);
    return -1;
  }

  //Read track
  serial_log(PSTR("Read track #%d\r\n"), n);

  if (f.read(fdc_data, size) == -1)
  {
    serial_log(PSTR("Failed to read track %d\r\n"), n);
    return -1;
  }

  serial_log(PSTR("Trk %d is ready\r\n"), n);

#ifdef  DEBUG_TRACK
  for (int i = 0; i < SEC_NUM; i++)
  {
    sector_t *sec = (sector_t *)&fdc_data[i * sizeof(sector_t)];
    sec_hdr_t *hdr = &sec->header;

    serial_log(PSTR("\r\nSector # %02x"), i);

    serial_log(PSTR("GAP1: \r\n"));
    for (int j = 0; j < sizeof(hdr->GAP1); j++)
    {
      serial_log(PSTR("%02X "), hdr->GAP1[j]);
    }

    serial_log(PSTR("\r\nIDAM_leading: \r\n"));
    for (int j = 0; j < sizeof(hdr->IDAM_leading); j++)
    {
      serial_log(PSTR("%02X "), hdr->IDAM_leading[j]);
    }

    serial_log(PSTR("\r\nTR: %02X, SC: %02X, T+S: %02X"), hdr->TR, hdr->SC, hdr->TS_sum);

    serial_log(PSTR("GAP2: "));
    for (int j = 0; j < sizeof(hdr->GAP2); j++)
    {
      serial_log(PSTR("%02X "), hdr->GAP2[j]);
    }

    serial_log(PSTR("\r\nIDAM_closing: "));
    for (int j = 0; j < sizeof(hdr->IDAM_closing); j++)
    {
      serial_log(PSTR("%02X "), hdr->IDAM_closing[j]);
    }

    serial_log(PSTR("\r\nNext TR: %02X, Next SC: %02X"), sec->next_track, sec->next_sector);
  }
#endif

  return 0;
}
*/
