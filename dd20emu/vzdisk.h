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

typedef struct SectorHeader {
  uint8_t   GAP1[7];    //0x80 6 times, then 0x00
  uint8_t   IDAM_leading[4];  //FE, E7, 18. C3
  uint8_t   TR;
  uint8_t   SC;
  uint8_t   TS_sum;
  uint8_t   GAP2[6];    //0x80 5 times, 0x00
  uint8_t   IDAM_closing[4];  //C3, 18, E7, FE
} sec_hdr_t;

#define SECSIZE_VZ  154   //SPEC size
#define SEC_DATA_SIZE 126
typedef struct Sector {
  sec_hdr_t   header;
  uint8_t     data_bytes[SEC_DATA_SIZE];
  uint8_t     next_track;
  uint8_t     next_sector;
  uint16_t    checksum;
} sector_t;

#define TRKSIZE_VZ  sizeof(sector_t) * SEC_NUM    //2464
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

    /*SEC: sector natural order in a track, check sector_interleave when SEC=5, will return sector 7 from the track*/
    bool get_sector(uint8_t TR, uint8_t SEC, bool v=false);
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
};

#endif	//_VZDISK_H_
