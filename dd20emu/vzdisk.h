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

#ifndef	_VZDISK_H_
#define	_VZDISK_H_


#define TRK_NUM     40
#define SEC_NUM     16
#define SECSIZE_VZ  154   //SPEC size
#define TRKSIZE_VZ  SECSIZE_VZ * SEC_NUM    //2464
#define SEC_DATA_SIZE 126
//#define TRKSIZE_VZ_PADDED TRKSIZE_VZ + 16

//Since most of the sectors in various formats of disk images have short sync words ( 5x80h, 00h ), we will use normailized sector header
//#define  NORMALIZED_SECTOR_HDR   1
 
typedef struct SectorHeader {
  /*Normalized means, if the GAP1 is 7 bytes, the first byte is skipped, only 6 bytes in output*/  
#ifdef   NORMALIZED_SECTOR_HDR 
  //some disk images have 6 bytes GAP1
  uint8_t   GAP1[6];    //0x80 5 times, then 0x00
#else
  /*spec GAP1*/
  uint8_t   GAP1[7];    //0x80 6 times, then 0x00
#endif
  uint8_t   IDAM_leading[4];  //FE, E7, 18. C3
  uint8_t   TR;
  uint8_t   SC;
  uint8_t   TS_sum;
  uint8_t   GAP2[6];    //0x80 5 times, 0x00
  uint8_t   IDAM_closing[4];  //C3, 18, E7, FE
} sec_hdr_t;

//defines the remaining # of bytes after SC field in sector header
#define SEC_HDR_REMAINING   sizeof((sec_hdr_t*)0)->TS_sum + sizeof((sec_hdr_t*)0)->GAP2 + sizeof((sec_hdr_t*)0)->IDAM_closing

typedef struct Sector {
  sec_hdr_t   header;
  uint8_t     data_bytes[SEC_DATA_SIZE];
  uint8_t     next_track;
  uint8_t     next_sector;
  uint16_t    checksum;
} sector_t;

//defines the remaing # of bytes after sec_hdr_t.SC field in a sector
#define SEC_REMAINING       SEC_HDR_REMAINING + sizeof((sector_t*)0)->data_bytes + sizeof((sector_t*)0)->next_track + \
                            sizeof((sector_t*)0)->next_sector + sizeof((sector_t*)0)->checksum

typedef struct Track {
  sector_t    sector[16];
} track_t;

class vzdisk {
  public:
    vzdisk();
    ~vzdisk();

    int Open(char *filename);
    void set_track_padding();
    uint8_t get_track_padding();
    void build_sector_lut();
    int get_sector(uint8_t n, uint8_t s);
    bool get_sector1(uint8_t n, uint8_t s, bool b=false);
    unsigned long get_track_offset(uint8_t TR);

    SdFat* get_sd();
    bool get_mounted();

  protected:
    int get_track(int n);

  private:
    bool sdInitialized;
    bool dsk_mounted;
    
    uint8_t current_track;
    unsigned long current_offset;
    uint16_t current_cur;
    
    SdFat SD;
    File file;
    uint8_t padding = 0; 

    int seekto_sector(uint8_t TR, uint8_t SEC);
    void reconstruct_sector();
};

#endif	//_VZDISK_H_
