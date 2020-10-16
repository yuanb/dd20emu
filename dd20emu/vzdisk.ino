//#define DEBUG_TRACK 1

/**
 * Disk image formats:
    1. file size 98,560 bytes, 40 tracks x 16 sectors x 154 bytes
 *  2. file size 99184 bytes
 *  3. file size is 99200 bytes
 */

typedef struct SectorHeader {
  uint8_t   GAP1[7];    //0x80 6 times, 0x00
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

void set_track_padding(File f)
{
  unsigned long fsize = f.size();
  if (fsize == 98560) {
    serial_log("%s is format 1 image", filename);
    padding = 0;
  }
  else if (fsize == 99184 || fsize == 99200)
  {
    if (f.seek(TRKSIZE_VZ) == false) {
      serial_log("Error when trying to identify dsk image format");
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
      serial_log("%s is format 2/3 image, padding = %d", filename, padding);
    }
  }  
}

int get_sector(File f, uint8_t n, uint8_t s)
{
  int result = -1;
  int sector_interleave[SEC_NUM] = { 0, 11, 6, 1, 12, 7, 2, 13, 8, 3, 14, 9, 4, 15, 10, 5 };
  unsigned long sector_offset[40][16] = {
    { 0x0, 0x9b, 0x135, 0x1cf, 0x269, 0x303, 0x39d, 0x437, 0x4d1, 
  }
  if (n<TRK_NUM && s<SEC_NUM)
  {
    unsigned long offset = track_offset[n] + sec_offset[s];

    if (f.seek(offset) != false)
    { 
      result= f.read(fdc_sector, SECSIZE_VZ);
      if (fdc_sector[0] == 0x80 && fdc_sector[1] == 0x80 && fdc_sector[2] == 0x80 &&
        fdc_sector[3] == 0x80 && fdc_sector[4] == 0x80 && fdc_sector[5] == 0x80 &&
        fdc_sector[6] == 0x00 && fdc_sector[7] == 0xFE && fdc_sector[8] == 0xE7 &&
        fdc_sector[9] == 0x18 && fdc_sector[10]== 0xC3 && 
        fdc_sector[11]== n    && fdc_sector[12]== sector_interleave[s]) {
      } else {
        serial_log("Not a hdr at 0x%04x, requested T:%02x, S:%02x", offset, n, sector_interleave[s]);
      }      
    }
    else {
      serial_log("Failed to seek 0x%04x for T:%02x, S:%02x", offset, n, s);      
    }
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
    serial_log("Invalid track number %d", offset);
    return -1;
  }

  if (f.seek(offset) == false)
  {
    serial_log("Failed seek file to %ul", offset);
    return -1;
  }

  //Read track
  //serial_log("Read track #%d", n);

  if (f.read(fdc_data, size) == -1)
  {
    serial_log("Failed to read track %d", n);
    return -1;
  }

  serial_log("Trk %d, Sector %d is ready", n,s);

#ifdef  DEBUG_TRACK
  for (int i = 0; i < SEC_NUM; i++)
  {
    sector_t *sec = (sector_t *)&fdc_data[i * sizeof(sector_t)];
    sec_hdr_t *hdr = &sec->header;

    serial_log("\r\nSector # %02x", i);

    Serial.print("GAP1: ");
    for (int j = 0; j < sizeof(hdr->GAP1); j++)
    {
      serial_log("%02X ", hdr->GAP1[j]);
    }

    Serial.print("\r\nIDAM_leading: ");
    for (int j = 0; j < sizeof(hdr->IDAM_leading); j++)
    {
      serial_log("%02X ", hdr->IDAM_leading[j]);
    }

    serial_log("\r\nTR: %02X, SC: %02X, T+S: %02X", hdr->TR, hdr->SC, hdr->TS_sum);

    Serial.print("GAP2: ");
    for (int j = 0; j < sizeof(hdr->GAP2); j++)
    {
      serial_log("%02X ", hdr->GAP2[j]);
    }

    Serial.print("\r\nIDAM_closing: ");
    for (int j = 0; j < sizeof(hdr->IDAM_closing); j++)
    {
      serial_log("%02X ", hdr->IDAM_closing[j]);
    }

    serial_log("\r\nNext TR: %02X, Next SC: %02X", sec->next_track, sec->next_sector);
  }
#endif

  return 0;
}
*/
