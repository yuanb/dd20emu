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

#ifndef	_VZDISK_H_
#define	_VZDISK_H_

//These numbers are spec numbers

#define TRK_NUM         40
#define SEC_NUM         16
#define SECSIZE_VZ      154
#define TRKSIZE_VZ  SECSIZE_VZ * SEC_NUM        //2464
#define PADDING_SIZE    16
//define    TRKSIZE_VZ_PADDED   TRKSIZE_VZ + 16

typedef struct SectorHeader {

  /* spec GAP1 */
  uint8_t     GAP1[7];        //0x80 6 times, then 0x00
  uint8_t     IDAM_leading[4];//FE, E7, 18, C3
  uint8_t     TR;
  uint8_t     SC;
  uint8_t     TS_sum;
  uint8_t     GAP2[6];        //0x80 5 times, 0x00
  uint8_t     IDAM_closing[4];//C3, 18, E7, FE
} sec_hdr_t;

#define LUT_SCAN_STEP   sizeof((sec_hdr_t*)0)->GAP1 + sizeof((sec_hdr_t*)0)->IDAM_leading + sizeof((sec_hdr_t*)0)->TR + sizeof((sec_hdr_t*)0)->SC

//defines the remaining # of bytes after SC field in sector header
#define SEC_HDR_REMAINING   sizeof((sec_hdr_t*)0)->TS_sum + sizeof((sec_hdr_t*)0)->GAP2 + sizeof((sec_hdr_t*)0)->IDAM_closing

typedef struct Sector {
  sec_hdr_t   header;
  uint8_t     data_bytes[126];
  uint8_t     next_track;
  uint8_t     next_sector;
  uint16_t    checksum;
} sector_t;

//defines the remaining # of bytes after sec_hdr_t.SC field in a sector
#define SEC_REMAINING       SEC_HDR_REMAINING + sizeof((sector_t*)0)->data_bytes + sizeof((sector_t*)0)->next_track + \
                            sizeof((sector_t*)0)->next_sector + sizeof((sector_t*)0)->checksum

typedef struct Track {
  sector_t    sector[SEC_NUM];
} track_t;

class vzdisk {
  public:
    vzdisk();
    int Open(char *filename);
    void set_track_padding();
    int sync_gap1(uint8_t* buf, uint8_t* TR, uint8_t* SEC, int debug =0);
    int build_sector_lut();
    int validate_sector_lut();
    int get_sector(uint8_t n, uint8_t s);    

  protected:
    int get_track(int n);

  private:
    bool sdInitialized = false;
#define  SDFAT    
#ifdef  SDFAT    
    SdFat SD;
#endif
    File file;
    uint8_t padding = 0; 
    char* filename;
};

#endif	//_VZDISK_H_
