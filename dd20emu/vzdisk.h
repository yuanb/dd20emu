#ifndef	_VZDISK_H_
#define	_VZDISK_H_


#define TRK_NUM     40
#define SEC_NUM     16
#define SECSIZE_VZ  154
#define TRKSIZE_VZ  SECSIZE_VZ * SEC_NUM    //2464
#define TRKSIZE_VZ_PADDED TRKSIZE_VZ + 16

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
