#ifndef	_VZDISK_H_
#define	_VZDISK_H_


#define TRK_NUM     40
#define SEC_NUM     16
#define SECSIZE_VZ  154
#define TRKSIZE_VZ  SECSIZE_VZ * SEC_NUM    //2464
//#define TRKSIZE_VZ_PADDED TRKSIZE_VZ + 16

#define  NORMALIZED_SECTOR_HDR   1
 
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

class vzdisk {
  public:
    vzdisk();
    int Open(char *filename);
    void set_track_padding();
    void build_sector_lut();
    int get_sector(uint8_t n, uint8_t s);    

  protected:
    int get_track(int n);

  private:
    bool sdInitialized = false;
    SdFat SD;
    File file;
    uint8_t padding = 0; 
};

#endif	//_VZDISK_H_
