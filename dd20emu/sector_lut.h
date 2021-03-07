#ifndef __SECTOR_LUT__
#define __SECTOR_LUT__

class sector_lut {
  public:
    sector_lut();
    int sync_gap1(uint8_t* buf, uint8_t &TR, uint8_t &SEC); 
};

#endif  //__SECTOR_LUT__
